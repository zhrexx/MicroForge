x.set_max_jobs(x.get_cpu_info().cores - 1) 

if (args.count > 0 and args[1] == "clean") then 
else 
print("XProject Build")
print("---------------------------------------------------------------------")
print("Platform informations: ")
print(string.format("- Platform: %s", x.get_platform()))
print(string.format("- OS: %s\n", x.get_os_name()))

local cpu = x.get_cpu_info()
if cpu then
    print("- CPU")
    print(string.format("-- Model: %s", cpu.model))
    print(string.format("-- Cores: %d", cpu.cores))
    print(string.format("-- Arch: %s\n", cpu.arch))
end

print(string.format("- Available memory: %d", x.get_available_memory()))

print("---------------------------------------------------------------------")

local function check_library(name, required, target)
    required = required or false
    
    io.write(string.format("Checking for library '%s'... ", name))
    local lib_path
    if target ~= nil then 
        lib_path = x.find_library(target, name)
    else 
        lib_path = x.find_library(name)
    end 
        
    if lib_path then
        print("found")
        return {found = true, path = lib_path}
    else
        print("not found")
        if required then
            print(string.format("ERROR: Required library '%s' not found. Build cannot continue.", name))
            os.exit(1)
        end
        return {found = false, path = nil}
    end
end


xbuild = x.create_target("xbuild", TARGET_EXECUTABLE)
x.add_source(xbuild, "xbuild.c")
x.add_library_path(xbuild, "./dependencies");
lm = check_library("m", true, nil);
if lm.found then 
    x.add_link_library(xbuild, "m");
end
llua = check_library("lua", true, xbuild)
if llua.found then 
    x.add_link_library(xbuild, "lua");
end
x.set_output_directory("./");
x.add_target_flag(xbuild, "-g -ggdb")
x.compile_target_parallel(xbuild)

print("---------------------------------------------------------------------")
end
