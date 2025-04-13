#!/usr/bin/env lua

local xam = {}
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

function xam:OS()
    return package.config:sub(1,1) == "\\" and "win" or "unix"
end

local target_win = "win"
local target_linux = "linux"
local target_linux_object = "linux_object"

local config = {
    inputFile = "input.xam",
    outputFile = "output.fasm",
    entryPoint = "__entry",
    target = platform and platform or (xam:OS() == target_win) and target_win or target_linux,
    verbose = false,
    optimize = false,
    auto_compile = true,
}

local function errorExit(msg, line_number, file_name, line_content)
    io.stderr:write(colors.red, "ERROR: ", msg, colors.reset, "\n")
    
    if line_number and file_name then
        io.stderr:write(colors.yellow, "File: ", file_name, " Line: ", line_number, colors.reset, "\n")
    end
    
    if line_content then
        io.stderr:write(colors.yellow, "Content: ", line_content, colors.reset, "\n")
    end
    
    io.stderr:write(colors.red, "Compilation aborted.", colors.reset, "\n")
    os.exit(1)
end

local current_line_number = 0
local current_file_name = ""

local i = 1
while arg and i <= #arg do
    if arg[i] == "-i" or arg[i] == "--input" then
        if not arg[i+1] then
            errorExit("Missing input file parameter after " .. arg[i])
        end
        config.inputFile = arg[i+1]
        i = i + 2
    elseif arg[i] == "-o" or arg[i] == "--output" then
        if not arg[i+1] then
            errorExit("Missing output file parameter after " .. arg[i])
        end
        config.outputFile = arg[i+1]
        i = i + 2
    elseif arg[i] == "-e" or arg[i] == "--entry" then
        if not arg[i+1] then
            errorExit("Missing entry point parameter after " .. arg[i])
        end
        config.entryPoint = arg[i+1]
        i = i + 2
    elseif arg[i] == "-v" or arg[i] == "--verbose" then
        config.verbose = true
        i = i + 1
    elseif arg[i] == "--optimize" then
        config.optimize = true
        i = i + 1
    elseif arg[i] == "--target" then
        if not arg[i+1] then
            errorExit("Missing target parameter after " .. arg[i])
        end
        local valid_targets = {[target_win]=true, [target_linux]=true, [target_linux_object]=true}
        if not valid_targets[arg[i+1]] then
            errorExit("Invalid target: " .. arg[i+1] .. ". Valid targets are: win, linux, linux_object")
        end
        config.target = arg[i+1]
        i = i + 2
    elseif arg[i] == "--no-auto-compile" then
        config.auto_compile = false
        i = i + 1
    elseif arg[i] == "-h" or arg[i] == "--help" then
        printBanner()
        print("Usage: xam [options]")
        print("Options:")
        print("  -i, --input FILE     Input assembly file (default: input.xam)")
        print("  -o, --output FILE    Output FASM file (default: output.fasm)")
        print("  -e, --entry LABEL    Entry point label (default: __entry)")
        print("  -v, --verbose        Enable verbose output")
        print("  --optimize           Enable basic optimizations")
        print("  --target TARGET_OS   Set a target os (win, linux, linux_object)")
        print("  -h, --help           Show this help message")
        print("  --no-auto-compile    Disable auto compilation")
        os.exit(0)
    else
        if i == 1 then config.inputFile = arg[i] end
        if i == 2 then config.outputFile = arg[i] end
        i = i + 1
    end
end

local function generateHeader()
    if config.target == target_win then
        return string.format([[
format PE64 console
entry %s

section '.text' code readable executable

]], config.entryPoint)
    elseif config.target == target_linux then
        return string.format([[
format ELF64 executable 3
entry %s

segment readable executable

]], config.entryPoint)
    elseif config.target == target_linux_object then
        return string.format([[
format ELF64

public _start
section '.text' executable
_start:
    call %s
]], config.entryPoint)
    end
end

local header = generateHeader()

local function log(msg, color)
    if config.verbose then
        color = color or colors.green
        io.write(color, msg, colors.reset, "\n")
    end
end

local function parseInputFile(filename)
    local file, err = io.open(filename, "r")
    if not file then
        errorExit("Could not open input file: " .. (err or filename))
    end

    current_file_name = filename
    log("Reading input file: " .. filename)
    local content = file:read("*all")
    file:close()

    return content
end

local preprocessor = {
    defines = {},
    includePaths = {"./"},
    conditionalStack = {},
    currentCondition = true
}

local function resolveIncludePath(filename)
    for _, path in ipairs(preprocessor.includePaths) do
        local fullPath = path .. "/" .. filename
        local file = io.open(fullPath, "r")
        if file then
            local content = file:read("*all")
            file:close()
            log("Included file: " .. fullPath, colors.yellow)
            return content, fullPath
        end
    end
    errorExit("Could not find include file: " .. filename, current_line_number, current_file_name)
    return nil
end

local function expandDefines(line)
    for name, value in pairs(preprocessor.defines) do
        line = line:gsub("%$" .. name .. "%f[^%w_]", tostring(value))
    end
    return line
end

local function appendDefine(name, value)
    preprocessor.defines[name] = value
end

local function handlePreprocessorDirective(line)
    local directive, operands = line:match("^!([%w_]+)%s*(.*)$")
    if not directive then return line end

    directive = directive:lower()
    log("Preprocessing directive: !" .. directive .. " " .. (operands or ""), colors.yellow)

    if not preprocessor.currentCondition then
        if directive == "endif" then
            table.remove(preprocessor.conditionalStack)
            preprocessor.currentCondition =
                #preprocessor.conditionalStack == 0 or
                preprocessor.conditionalStack[#preprocessor.conditionalStack]
        elseif directive == "else" and #preprocessor.conditionalStack > 0 then
            preprocessor.currentCondition =
                not preprocessor.conditionalStack[#preprocessor.conditionalStack]
        end
        return ""
    end

    if directive == "define" then
        local name, value = operands:match("^([%w_]+)%s*(.*)$")
        if name then
            preprocessor.defines[name] = value or ""
            log("Defined " .. name .. " = '" .. (value or "") .. "'", colors.yellow)
        else
            errorExit("Invalid !define syntax", current_line_number, current_file_name, line)
        end
        return ""

    elseif directive == "undef" then
        local name = operands:match("^([%w_]+)%s*$")
        if name then
            if preprocessor.defines[name] == nil then
                log("Warning: Attempting to undef '" .. name .. "' which is not defined", colors.yellow)
            end
            preprocessor.defines[name] = nil
            log("Undefined " .. name, colors.yellow)
        else
            errorExit("Invalid !undef syntax", current_line_number, current_file_name, line)
        end
        return ""

    elseif directive == "ifdef" or directive == "ifndef" then
        local name = operands:match("^([%w_]+)%s*$")
        if name then
            local isDefined = preprocessor.defines[name] ~= nil
            if directive == "ifndef" then isDefined = not isDefined end

            table.insert(preprocessor.conditionalStack, isDefined)
            preprocessor.currentCondition = isDefined

            log("Conditional " .. directive .. " " .. name .. ": " .. tostring(isDefined), colors.yellow)
        else
            errorExit("Invalid " .. directive .. " syntax", current_line_number, current_file_name, line)
        end
        return ""

    elseif directive == "else" then
        if #preprocessor.conditionalStack > 0 then
            local currentTop = preprocessor.conditionalStack[#preprocessor.conditionalStack]
            preprocessor.currentCondition = not currentTop
            preprocessor.conditionalStack[#preprocessor.conditionalStack] = not currentTop

            log("Conditional else: " .. tostring(preprocessor.currentCondition), colors.yellow)
        else
            errorExit("!else without matching !ifdef/!ifndef", current_line_number, current_file_name, line)
        end
        return ""

    elseif directive == "endif" then
        if #preprocessor.conditionalStack > 0 then
            table.remove(preprocessor.conditionalStack)
            preprocessor.currentCondition =
                #preprocessor.conditionalStack == 0 or
                preprocessor.conditionalStack[#preprocessor.conditionalStack]

            log("Conditional endif", colors.yellow)
        else
            errorExit("!endif without matching !ifdef/!ifndef", current_line_number, current_file_name, line)
        end
        return ""

    elseif directive == "include" then
        local filename = operands:match("^\"([^\"]+)\"$") or
                        operands:match("^<([^>]+)>$")

        if filename then
            local content, includedFile = resolveIncludePath(filename)
            local prevFile = current_file_name
            current_file_name = includedFile
            local result = content
            current_file_name = prevFile
            return result
        else
            errorExit("Invalid !include syntax", current_line_number, current_file_name, line)
        end

    elseif directive == "includepath" then
        local path = operands:match("^\"([^\"]+)\"$")
        if path then
            table.insert(preprocessor.includePaths, path)
            log("Added include path: " .. path, colors.yellow)
        else
            errorExit("Invalid !includepath syntax", current_line_number, current_file_name, line)
        end
        return ""

    elseif directive == "entry" then
        local entryLabel = operands:match("^([%w_]+)%s*$")
        if entryLabel then
            config.entryPoint = entryLabel
            log("Entry point set to: " .. entryLabel, colors.yellow)
        else
            errorExit("Invalid !entry syntax", current_line_number, current_file_name, line)
        end
        return ""
    else
        errorExit("Unknown preprocessor directive: !" .. directive, current_line_number, current_file_name, line)
    end
end

local function preprocessContent(content)
    local lines = {}
    local result = {}

    preprocessor.defines = {}
    preprocessor.conditionalStack = {}
    preprocessor.currentCondition = true
    preprocessor.includePaths = {"./"}

    for line in content:gmatch("[^\r\n]+") do
        table.insert(lines, line)
    end

    log("Preprocessing " .. #lines .. " lines")

    for i, line in ipairs(lines) do
        current_line_number = i
        local trimmed = line:match("^%s*(.-)%s*$")

        if trimmed:match("^!") then
            local processedLine = handlePreprocessorDirective(trimmed)
            if processedLine and #processedLine > 0 then
                for subline in processedLine:gmatch("[^\r\n]+") do
                    table.insert(result, subline)
                end
            end
        else
            if preprocessor.currentCondition then
                local expandedLine = expandDefines(trimmed)
                table.insert(result, expandedLine)
            end
        end
    end

    if #preprocessor.conditionalStack > 0 then
        errorExit("Unclosed conditional directive (!ifdef/!ifndef without matching !endif)", current_line_number, current_file_name)
    end

    return table.concat(result, "\n")
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

local function extractDataLabels(content)
    local data_labels = {}
    current_line_number = 0
    
    for line in content:gmatch("[^\r\n]+") do
        current_line_number = current_line_number + 1
        local trimmed = line:match("^%s*(.-)%s*$")
        trimmed = trimmed:gsub(";.*", "")
        trimmed = trimmed:match("^%s*(.-)%s*$")

        if #trimmed > 0 then
            local label, str = trimmed:match("^([%w_]+):%s*db%s+'([^']*)'")
            if not label or not str then
                label, str = trimmed:match("^([%w_]+):%s*db%s+\"([^\"]*)\"")
            end

             if not label then
                label, str = trimmed:match("^([%w_]+):%s*db%s+(%d+)")
            end

            if label and str then
                local num = tonumber(str)
                data_labels[label] = num or str
                local value_str = type(num) == "number" and str or ("'" .. str .. "'")
                log("Found data label: " .. label .. " = " .. value_str, colors.magenta)
            end
        end
    end
    return data_labels
end

local function substituteBuiltins(line, data_labels)
    for name, value in pairs(preprocessor.defines) do
        line = line:gsub("%$" .. name .. "%f[^%w_]", tostring(value))
    end

    line = line:gsub("strlen%(([%w_]+)%)", function(label)
        local str = data_labels[label]
        if str then
            return tostring(#str)
        else
            errorExit("strlen: undefined data label '" .. label .. "'", current_line_number, current_file_name, line)
        end
    end)

    line = line:gsub("__FILE__", current_file_name or config.inputFile)
    line = line:gsub("__DATE__", os.date("%Y-%m-%d"))
    line = line:gsub("__TIME__", os.date("%H:%M:%S"))
    line = line:gsub("__PLATFORM__", config.target)
    return line
end

local function processContent(content)
    local lines = {}
    local data_section = {}
    local bss_section = {}
    local code_section = {}
    local has_entry = false
    local entry_count = 0
    current_line_number = 0

    for line in content:gmatch("[^\r\n]+") do
        table.insert(lines, line)
    end

    log("Processing " .. #lines .. " lines of code")

    local data_labels = extractDataLabels(content)

    for i, line in ipairs(lines) do
        current_line_number = i
        local trimmed = line:match("^%s*(.-)%s*$")
        trimmed = trimmed:gsub(";.*", "")
        trimmed = trimmed:match("^%s*(.-)%s*$")

        if #trimmed > 0 then
            local isData = false
            local isBSS = false

            if trimmed:match(":%s*db%s+") or
               trimmed:match(":%s*dw%s+") or
               trimmed:match(":%s*dd%s+") or
               trimmed:match(":%s*dq%s+") then
                isData = true
            end

            if trimmed:match(":%s*rb%s+") or
               trimmed:match(":%s*rw%s+") or
               trimmed:match(":%s*rd%s+") or
               trimmed:match(":%s*rq%s+") then
                isBSS = true
            end

            if isData then
                local processed_line = substituteBuiltins(trimmed, data_labels)
                table.insert(data_section, processed_line)
                log("DATA: " .. trimmed, colors.magenta)
            elseif isBSS then
                local processed_line = substituteBuiltins(trimmed, data_labels)
                table.insert(bss_section, processed_line)
                log("BSS: " .. trimmed, colors.blue)
            else
                local processed_line = substituteBuiltins(trimmed, data_labels)

                if processed_line:match("^" .. config.entryPoint .. ":") then
                    has_entry = true
                    entry_count = entry_count + 1
                end

                if processed_line:sub(-1) == ':' then
                    table.insert(code_section, processed_line)
                else
                    table.insert(code_section, string.format("    %s", processed_line))
                end
                log("CODE: " .. processed_line, colors.cyan)
            end
        end
    end

    if not has_entry then
        errorExit("No '" .. config.entryPoint .. ":' label found in the input file.", nil, current_file_name)
    elseif entry_count > 1 then
        log("WARNING: Multiple entry points found, using the first one", colors.yellow)
    end

    if config.optimize then
        code_section = optimizeCode(code_section)
        data_section = optimizeCode(data_section)
        bss_section = optimizeCode(bss_section)
    end

    local output = header
    output = output .. table.concat(code_section, "\n") .. "\n\n"

    if #data_section > 0 or #bss_section > 0 then
        if config.target ~= target_linux_object then
            output = output .. "segment readable writeable\n\n"
        else
            output = output .. "section '.data' writeable\n\n"
        end

        if #data_section > 0 then
            output = output .. table.concat(data_section, "\n") .. "\n"
        end

        if config.target == target_linux_object then
            output = output .. "section '.bss' writeable\n\n"
        end
        if #bss_section > 0 then
            output = output .. table.concat(bss_section, "\n") .. "\n"
        end
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

local function auto_compile()
    log("Starting auto-compilation...", colors.yellow)
    
    local command
    local result
    
    if config.target == target_linux or config.target == target_linux_object then
        command = string.format("fasm %s", config.outputFile)
        log("Running: " .. command, colors.blue)
        result = os.execute(command)
    else
        command = string.format("fasm.exe %s", config.outputFile)
        log("Running: " .. command, colors.blue)
        result = os.execute(command)
    end
    
    if not result then
        errorExit("Auto-compilation failed with command: " .. command)
    end
    
    local remove_result = os.remove(config.outputFile)
    if not remove_result then
        log("Warning: Failed to remove temporary file: " .. config.outputFile, colors.yellow)
    end

    io.write(colors.green, "Auto Compile finished successfully", colors.reset, "\n")
end

local startTime = os.clock()

local function main()
    printBanner()

    io.write(colors.bright_yellow, "Input:  ", colors.reset, config.inputFile, "\n")
    io.write(colors.bright_yellow, "Output: ", colors.reset, config.outputFile, "\n")
    io.write(colors.bright_yellow, "Entry:  ", colors.reset, config.entryPoint, "\n")
    io.write(colors.bright_yellow, "Target: ", colors.reset, config.target, "\n")
    io.write(colors.bright_yellow, "Auto Compile: ", colors.reset, tostring(config.auto_compile), "\n")
    print("")

    current_file_name = config.inputFile
    local content = parseInputFile(config.inputFile)
    content = preprocessContent(content)
    header = generateHeader()
    local processed = processContent(content)
    writeOutputFile(config.outputFile, processed)

    local elapsed = os.clock() - startTime
    io.write(colors.green, "Assembly successful! ", colors.reset,
             "Completed in ", string.format("%.2fms", elapsed * 1000), "\n")

    if config.auto_compile then
        auto_compile()
    end
end

local status, err = pcall(main)
if not status then
    errorExit("Runtime error: " .. tostring(err))
end
