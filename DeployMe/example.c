#include "deploy.h"

int main(int argc, char **argv) {
    dm_enable_debug();
    dm_request_sudo(argc, argv);
    if (!dm_install_package("curl")) {
        dm_log(DM_ERROR, "Failed to install '%s': %s.\n", "curl", dm_strerror(errno));
    }
    
    dm_execute_command("echo \"%s\"", "Lul");
    dm_log(DM_INFO, "Hello, World %s!\n", "lul");
    return 0;
}

