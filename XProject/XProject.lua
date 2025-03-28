platform = x.get_platform()
print("Building on " .. platform)

target = x.create_target("xproject", TARGET_EXECUTABLE)
x.add_source(target, "xproject.c")
x.add_target_flag(target, "-L./dependencies/");
x.add_library_path(target, "./dependencies");
x.add_link_library(target, "m");
x.add_link_library(target, "lua");
x.compile_target(target)

