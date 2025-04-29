x.set_output_directory("build")
x.set_max_jobs(x.get_cpu_info().cores - 1) 

print("XProject Build")

print("---------------------------------------------------------------------")
print("Platform informations: ")
print(string.format("- Platform: %s", x.get_platform()))
print(string.format("- OS: %s\n", x.get_os_name()))

local cpu = x.get_cpu_info()
if cpu then
    print("- CPU")
    print(string.format("-- Model: %s", cpu.model))
    print(string.format("-- Cores: %d\n", cpu.cores))
end

print(string.format("- Available memory: %d", x.get_available_memory()))

print("---------------------------------------------------------------------")

local function check_library(name, required)
    required = required or false
    
    io.write(string.format("Checking for library '%s'... ", name))
    local lib_path = x.find_library(name)
    
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

target = x.create_target("xproject", TARGET_EXECUTABLE)
x.add_source(target, "xproject.c")
x.add_library_path(target, "./dependencies");
lm = check_library("m", true);
if lm.found then 
    x.add_link_library(target, "m");
end
llua = check_library("lua", true)
if llua.found then 
    x.add_link_library(target, "lua");
end
x.set_output_directory("./");
x.compile_target_parallel(target)
print("---------------------------------------------------------------------")



