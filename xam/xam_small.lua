#!/usr/bin/env lua

local colors = {
    reset = "\27[0m",
    red = "\27[31m",
    green = "\27[32m",
    yellow = "\27[33m",
    blue = "\27[34m",
    magenta = "\27[35m",
    cyan = "\27[36m",
    bright_cyan = "\27[96m",
    bright_yellow = "\27[93m"
}

local function printBanner()
    local banner = [[
██╗  ██╗ █████╗ ███╗   ███╗
╚██╗██╔╝██╔══██╗████╗ ████║
 ╚███╔╝ ███████║██╔████╔██║
 ██╔██╗ ██╔══██║██║╚██╔╝██║
██╔╝ ██╗██║  ██║██║ ╚═╝ ██║
╚═╝  ╚═╝╚═╝  ╚═╝╚═╝     ╚═╝ 
    ]]
    
    io.write(colors.bright_cyan, banner, colors.reset, "\n")
    io.write(colors.cyan, "Your Favorite Assembler v1.0", colors.reset, "\n\n")
end

local config = {
    inputFile = "input.asm",
    outputFile = "output.asm",
    entryPoint = "__entry",
    verbose = false,
    optimize = false
}

local i = 1
while i <= #arg do
    if arg[i] == "-i" or arg[i] == "--input" then
        config.inputFile = arg[i+1]
        i = i + 2
    elseif arg[i] == "-o" or arg[i] == "--output" then
        config.outputFile = arg[i+1]
        i = i + 2
    elseif arg[i] == "-e" or arg[i] == "--entry" then
        config.entryPoint = arg[i+1]
        i = i + 2
    elseif arg[i] == "-v" or arg[i] == "--verbose" then
        config.verbose = true
        i = i + 1
    elseif arg[i] == "--optimize" then
        config.optimize = true
        i = i + 1
    elseif arg[i] == "-h" or arg[i] == "--help" then
        printBanner()
        print("Usage: xam [options]")
        print("Options:")
        print("  -i, --input FILE     Input assembly file (default: input.asm)")
        print("  -o, --output FILE    Output FASM file (default: output.asm)")
        print("  -e, --entry LABEL    Entry point label (default: __entry)")
        print("  -v, --verbose        Enable verbose output")
        print("  --optimize           Enable basic optimizations")
        print("  -h, --help           Show this help message")
        os.exit(0)
    else
        if i == 1 then config.inputFile = arg[i] end
        if i == 2 then config.outputFile = arg[i] end
        i = i + 1
    end
end

local header = string.format([[
format ELF64 executable 3
entry %s

segment readable executable

]], config.entryPoint)

local function log(msg, color)
    if config.verbose then
        color = color or colors.green
        io.write(color, msg, colors.reset, "\n")
    end
end

local function errorExit(msg)
    io.stderr:write(colors.red, "ERROR: ", msg, colors.reset, "\n")
    os.exit(1)
end

local function parseInputFile(filename)
    local file, err = io.open(filename, "r")
    if not file then
        errorExit("Could not open input file: " .. (err or filename))
    end
    
    log("Reading input file: " .. filename)
    local content = file:read("*all")
    file:close()
    
    return content
end

local function optimizeCode(lines)
    if not config.optimize then return lines end
    
    log("Applying basic optimizations...", colors.yellow)
    local result = {}
    
    local lastWasEmpty = false
    for _, line in ipairs(lines) do
        local isEmpty = (line:match("^%s*$") ~= nil)
        if not (isEmpty and lastWasEmpty) then
            table.insert(result, line)
        end
        lastWasEmpty = isEmpty
    end
    
    return result
end

function endswith(str, suffix)
    return str:sub(-#suffix) == suffix
end

local function processContent(content)
    local lines = {}
    local data_section = {}
    local code_section = {}
    local has_entry = false
    local entry_count = 0
    
    for line in content:gmatch("[^\r\n]+") do
        table.insert(lines, line)
    end
    
    log("Processing " .. #lines .. " lines of code")
    
    for _, line in ipairs(lines) do
        local trimmed = line:match("^%s*(.-)%s*$")
        
        if #trimmed > 0 and not trimmed:match("^;") then
            local isData = false
            for _, pattern in ipairs({"db%s+", "dw%s+", "dd%s+", "dq%s+", "rb%s+", "rw%s+", "rd%s+", "rq%s+"}) do
                if trimmed:match(pattern) then
                    isData = true
                    break
                end
            end
            
            if isData then
                table.insert(data_section, trimmed)
                log("DATA: " .. trimmed, colors.magenta)
            else
                if endswith(trimmed, ':') then 
                    table.insert(code_section, trimmed)
                else 
                    table.insert(code_section, string.format("    %s", trimmed))
                end
                log("CODE: " .. trimmed, colors.cyan)
                
                if trimmed:match("^" .. config.entryPoint .. ":") then
                    has_entry = true
                    entry_count = entry_count + 1
                end
            end
        end
    end
    
    if not has_entry then
        errorExit("No '" .. config.entryPoint .. ":' label found in the input file.")
    elseif entry_count > 1 then
        log("WARNING: Multiple entry points found, using the first one", colors.yellow)
    end
    
    if config.optimize then
        code_section = optimizeCode(code_section)
        data_section = optimizeCode(data_section)
    end
    
    local output = header
    
    output = output .. table.concat(code_section, "\n") .. "\n\n"
    
    if #data_section > 0 then
        output = output .. "segment readable writeable\n\n"
        output = output .. table.concat(data_section, "\n") .. "\n"
    end
    
    log("Generated " .. #code_section .. " code lines and " .. #data_section .. " data lines")
    
    return output
end

local function writeOutputFile(filename, content)
    local file, err = io.open(filename, "w")
    if not file then
        errorExit("Could not open output file: " .. (err or filename))
    end
    
    file:write(content)
    file:close()
    
    io.write(colors.green, "Written " .. #content .. " bytes to " .. filename, "\n")
end

local startTime = os.clock()

local function main()
    printBanner()
    
    io.write(colors.bright_yellow, "Input:  ", colors.reset, config.inputFile, "\n")
    io.write(colors.bright_yellow, "Output: ", colors.reset, config.outputFile, "\n")
    io.write(colors.bright_yellow, "Entry:  ", colors.reset, config.entryPoint, "\n")
    print("")
    
    local content = parseInputFile(config.inputFile)
    local processed = processContent(content)
    writeOutputFile(config.outputFile, processed)
    
    local elapsed = os.clock() - startTime
    io.write(colors.green, "Assembly successful! ", colors.reset, 
             "Completed in ", string.format("%.2f", elapsed * 1000), "ms\n")
end

local status, err = pcall(main)
if not status then
    errorExit(err)
end
