platform = x.get_platform()
print("Building on " .. platform)

target = x.create_target("myapp", x.TARGET_EXECUTABLE)
x.add_source(target, "main.c")
x.compile_target(target)
