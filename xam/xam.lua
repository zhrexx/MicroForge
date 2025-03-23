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

local function extractDataLabels(content)
    local data_labels = {}
    for line in content:gmatch("[^\r\n]+") do
        local trimmed = line:match("^%s*(.-)%s*$")
        trimmed = trimmed:gsub(";.*", "")
        trimmed = trimmed:match("^%s*(.-)%s*$")
        
        if #trimmed > 0 then
            local label, str = trimmed:match("^([%w_]+):%s*db%s+'([^']*)'")
            if not label or not str then
                label, str = trimmed:match("^([%w_]+):%s*db%s+\"([^\"]*)\"")
            end
            
            if label and str then
                data_labels[label] = str
                log("Found data label: " .. label .. " = '" .. str .. "'", colors.magenta)
            end
        end
    end
    return data_labels
end

local function substituteBuiltins(line, data_labels)
    line = line:gsub("strlen%(([%w_]+)%)", function(label)
        local str = data_labels[label]
        if str then
            return tostring(#str)
        else
            errorExit("strlen: undefined data label '" .. label .. "'")
        end
    end)
    
    line = line:gsub("strcat%(([%w_]+)%s*,%s*([%w_]+)%)", function(label1, label2)
        local str1 = data_labels[label1]
        local str2 = data_labels[label2]
        
        if not str1 then
            errorExit("strcat: undefined data label '" .. label1 .. "'")
        end
        if not str2 then
            errorExit("strcat: undefined data label '" .. label2 .. "'")
        end
        
        local combined = label1 .. "_" .. label2
        data_labels[combined] = str1 .. str2
        return combined
    end)
    
    line = line:gsub("charat%(([%w_]+)%s*,%s*(%d+)%)", function(label, index)
        local str = data_labels[label]
        index = tonumber(index)
        
        if not str then
            errorExit("charat: undefined data label '" .. label .. "'")
        end
        
        if index < 0 or index >= #str then
            errorExit("charat: index " .. index .. " out of bounds for label '" .. label .. "' (length " .. #str .. ")")
        end
        
        return tostring(string.byte(str, index + 1))
    end)
    
    line = line:gsub("strbyte%(([%w_]+)%)", function(label)
        local str = data_labels[label]
        
        if not str then
            errorExit("strbyte: undefined data label '" .. label .. "'")
        end
        
        if #str == 0 then
            errorExit("strbyte: empty string for label '" .. label .. "'")
        end
        
        return tostring(string.byte(str, 1))
    end)
    
    line = line:gsub("upper%(([%w_]+)%)", function(label)
        local str = data_labels[label]
        
        if not str then
            errorExit("upper: undefined data label '" .. label .. "'")
        end
        
        local upper_label = label .. "_upper"
        data_labels[upper_label] = string.upper(str)
        return upper_label
    end)
    
    line = line:gsub("lower%(([%w_]+)%)", function(label)
        local str = data_labels[label]
        
        if not str then
            errorExit("lower: undefined data label '" .. label .. "'")
        end
        
        local lower_label = label .. "_lower"
        data_labels[lower_label] = string.lower(str)
        return lower_label
    end)
    
    line = line:gsub("substr%(([%w_]+)%s*,%s*(%d+)%s*,%s*(%d+)%)", function(label, start, length)
        local str = data_labels[label]
        start = tonumber(start)
        length = tonumber(length)
        
        if not str then
            errorExit("substr: undefined data label '" .. label .. "'")
        end
        
        if start < 0 or start >= #str then
            errorExit("substr: start index " .. start .. " out of bounds for label '" .. label .. "' (length " .. #str .. ")")
        end
        
        local substr = string.sub(str, start + 1, start + length)
        local substr_label = label .. "_sub_" .. start .. "_" .. length
        data_labels[substr_label] = substr
        return substr_label
    end)
    
    line = line:gsub("ascii%('(.)'%)", function(char)
        return tostring(string.byte(char))
    end)
    
    line = line:gsub("sizeof%((%w+)%)", function(type)
        local sizes = {
            byte = 1,
            word = 2,
            dword = 4,
            qword = 8,
            db = 1,
            dw = 2,
            dd = 4,
            dq = 8
        }
        
        local size = sizes[string.lower(type)]
        if not size then
            errorExit("sizeof: unknown type '" .. type .. "'")
        end
        
        return tostring(size)
    end)
    
    line = line:gsub("align%((%d+)%s*,%s*(%d+)%)", function(value, alignment)
        value = tonumber(value)
        alignment = tonumber(alignment)
        
        if alignment <= 0 then
            errorExit("align: alignment must be positive, got " .. alignment)
        end
        
        local result = math.ceil(value / alignment) * alignment
        return tostring(result)
    end)
    
    line = line:gsub("max%((%d+)%s*,%s*(%d+)%)", function(a, b)
        return tostring(math.max(tonumber(a), tonumber(b)))
    end)
    
    line = line:gsub("min%((%d+)%s*,%s*(%d+)%)", function(a, b)
        return tostring(math.min(tonumber(a), tonumber(b)))
    end)
    
    line = line:gsub("hex2dec%(0x([%da-fA-F]+)%)", function(hex)
        return tostring(tonumber(hex, 16))
    end)
    
    line = line:gsub("dec2hex%((%d+)%)", function(dec)
        return string.format("0x%x", tonumber(dec))
    end)
    
    return line
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
    
    local data_labels = extractDataLabels(content)
    
    for _, line in ipairs(lines) do
        local trimmed = line:match("^%s*(.-)%s*$")
        trimmed = trimmed:gsub(";.*", "")
        trimmed = trimmed:match("^%s*(.-)%s*$")
        
        if #trimmed > 0 then
            local isData = false
            
            if trimmed:match(":%s*db%s+") or 
               trimmed:match(":%s*dw%s+") or 
               trimmed:match(":%s*dd%s+") or 
               trimmed:match(":%s*dq%s+") or
               trimmed:match(":%s*rb%s+") or
               trimmed:match(":%s*rw%s+") or
               trimmed:match(":%s*rd%s+") or
               trimmed:match(":%s*rq%s+") then
                isData = true
            end
            
            if isData then
                table.insert(data_section, trimmed)
                log("DATA: " .. trimmed, colors.magenta)
            else
                local processed_line = substituteBuiltins(trimmed, data_labels)
                
                if processed_line:match("^" .. config.entryPoint .. ":") then
                    has_entry = true
                    entry_count = entry_count + 1
                end
                
                if endswith(processed_line, ':') then
                    table.insert(code_section, processed_line)
                else
                    table.insert(code_section, string.format("    %s", processed_line))
                end
                log("CODE: " .. processed_line, colors.cyan)
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
