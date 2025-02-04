#ifndef DEPLOY_H
#define DEPLOY_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>
// dm_ = DeployMe

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

#define DM_DEFAULT_SIZE 1024

typedef struct {
    bool sudo;
    bool debug;
} State;
static State state = {0};

#define DME_INVALID_PERMISSIONS 1000

const char *dm_strerror(int err) {
    switch (err) {
        case DME_INVALID_PERMISSIONS:
            return "Invalid Permissions (run as program as sudo or run dm_request_sudo before dm_install_package)";

        default:
            return "Invalid Error Code";
    }    
    
}

void dm_enable_debug() {
    state.debug = true;
}

// TODO: Add more pms
#ifdef _WIN32
    #define PKG_CHECK_CMD "choco list --local-only "
#elif __APPLE__
    #define PKG_CHECK_CMD "brew list --formula "
#elif __linux__
    #if __has_include("/usr/bin/dpkg")
        #define PKG_CHECK_CMD "dpkg -l | grep "
    #elif __has_include("/usr/bin/rpm")
        #define PKG_CHECK_CMD "rpm -q "
    #elif __has_include("/usr/bin/pacman")
        #define PKG_CHECK_CMD "pacman -Q "
    #elif __has_include("/usr/bin/zypper")
        #define PKG_CHECK_CMD "zypper se --installed-only "
    #elif __has_include("/usr/bin/xbps-query")
        #define PKG_CHECK_CMD "xbps-query -l "
    #elif __has_include("/usr/bin/nix-env")
        #define PKG_CHECK_CMD "nix-env -q "
    #elif __has_include("/usr/bin/flatpak")
        #define PKG_CHECK_CMD "flatpak list "
    #elif __has_include("/usr/bin/snap")
        #define PKG_CHECK_CMD "snap list "
    #elif __has_include("/usr/bin/emerge")
        #define PKG_CHECK_CMD "equery list "
    #elif __has_include("/usr/bin/port")
        #define PKG_CHECK_CMD "port installed "
    #elif __has_include("/usr/bin/slackpkg")
        #define PKG_CHECK_CMD "slackpkg search "
    #elif __has_include("/usr/bin/guix")
        #define PKG_CHECK_CMD "guix package -I "
    #else
        #error "No supported package manager found!"
    #endif
#else
    #error "Unsupported platform!"
#endif


typedef enum {
    DM_INFO,
    DM_WARNING,
    DM_ERROR,
    DM_DEBUG
} DM_LOG_LEVEL;

void dm_log(DM_LOG_LEVEL level, const char *message, ...) {
    switch (level) {
        case DM_INFO:
            printf("[INFO] ");
            break;
        case DM_WARNING:
            printf("[WARNING] ");
            break;
        case DM_ERROR:
            printf("[ERROR] ");
            break;
        case DM_DEBUG:
            printf("[DEBUG] ");
            break;
    }

    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);
}
const char* dm_detect_package_manager() {
    char buffer[256];

#if defined(_WIN32) || defined(_WIN64)
    #define COMMAND_EXISTS(cmd) (snprintf(buffer, sizeof(buffer), "where %s >nul 2>&1", cmd), system(buffer) == 0)
#else
    #define COMMAND_EXISTS(cmd) (snprintf(buffer, sizeof(buffer), "command -v %s >/dev/null 2>&1", cmd), system(buffer) == 0)
#endif

#if defined(_WIN32) || defined(_WIN64)
    if (COMMAND_EXISTS("choco")) return "chocolatey";
    if (COMMAND_EXISTS("winget")) return "winget";
    if (COMMAND_EXISTS("scoop")) return "scoop";
#else
    if (COMMAND_EXISTS("apt")) return "apt";
    if (COMMAND_EXISTS("dnf")) return "dnf";
    if (COMMAND_EXISTS("yum")) return "yum";
    if (COMMAND_EXISTS("pacman")) return "pacman";
    if (COMMAND_EXISTS("zypper")) return "zypper";
    if (COMMAND_EXISTS("brew")) return "brew";
    if (COMMAND_EXISTS("port")) return "macports";
    if (COMMAND_EXISTS("apk")) return "apk";
    if (COMMAND_EXISTS("pkg")) return "pkg";
    if (COMMAND_EXISTS("xbps-install")) return "xbps";
    if (COMMAND_EXISTS("nix-env")) return "nix";
    if (COMMAND_EXISTS("flatpak")) return "flatpak";
    if (COMMAND_EXISTS("snap")) return "snap";
#endif

    return NULL;
}
int dm_is_package_installed(const char *package_name) {
    char command[512];
    snprintf(command, sizeof(command), PKG_CHECK_CMD "%s > /dev/null 2>&1", package_name);
    return system(command) == 0;
}

bool dm_install_package(char *package) {
    if (!state.sudo) {
        errno = DME_INVALID_PERMISSIONS;
        return false;
    }
    const char *pm = dm_detect_package_manager();
    if (pm == NULL) {
        return false;
    }

    if (dm_is_package_installed(package)) {
        if (state.debug) dm_log(DM_INFO, "Package '%s' already installed!\n", package);
        return true;
    }

    char buffer[DM_DEFAULT_SIZE];
    snprintf(buffer, sizeof(buffer), "sudo %s install %s -y", pm, package);
    system(buffer);

    return true;
}

int dm_request_sudo(int argc, char *argv[]) {
#ifdef _WIN32
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }

    if (!isAdmin) {
        char params[1024] = {0};
        for (int i = 1; i < argc; i++) {
            strcat(params, "\"");
            strcat(params, argv[i]);
            strcat(params, "\" ");
        }

        SHELLEXECUTEINFO sei = { sizeof(sei) };
        sei.lpVerb = "runas";
        sei.lpFile = argv[0];
        sei.lpParameters = params;
        sei.nShow = SW_SHOWNORMAL;

        if (!ShellExecuteEx(&sei)) {
            dm_log(DM_ERROR, "Failed to obtain administrator privileges.\n");
            return -1;
        }
        exit(0);
    }
    else {
        state.sudo = true;
    }
#else
    if (state.sudo) {
        return 0;
    }
    if (geteuid() != 0) {
        if (state.debug) dm_log(DM_INFO, "This program needs to run as root. Requesting sudo...\n");

        char **new_argv = calloc(argc + 2, sizeof(char*));
        new_argv[0] = "/usr/bin/sudo";
        new_argv[1] = argv[0];
        for (int i = 1; i < argc; i++) {
            new_argv[i + 1] = argv[i];
        }
        new_argv[argc + 1] = NULL;

        int status = execv("/usr/bin/sudo", new_argv);
        if (status == -1){ 
            dm_log(DM_ERROR, "Failed to get sudo. Run it again but with sudo.");
            free(new_argv);
            exit(1);
        }
    } else {
        //dm_log(DM_INFO, "Success Program runned as sudo.\n");
        state.sudo = true;
    }
#endif
    return 0;
}


int dm_execute_command(char *fmt, ...) {
    char *command = malloc(DM_DEFAULT_SIZE);
    va_list args;
    va_start(args, fmt);
    vsprintf(command, fmt, args);
    va_end(args);

    if (state.debug) dm_log(DM_INFO, "Executing command \"%s\"\n", command);
    int return_code = system(command);
    if (return_code < 0) {
        return return_code;
    }

    return -1;
}

#endif // DEPLOY_H
