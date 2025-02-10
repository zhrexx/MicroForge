#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/utsname.h>
#include <limits.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void get_ip_address() {
    struct ifaddrs *ifaddr, *ifa;
    char ip[INET_ADDRSTRLEN];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return;
    }

    printf("IP Addresses:\n");
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;

        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
            inet_ntop(AF_INET, &(sa->sin_addr), ip, INET_ADDRSTRLEN);
            printf("  %s: %s\n", ifa->ifa_name, ip);
        }
    }
    freeifaddrs(ifaddr);
}

void get_cpu_info() {
    FILE *cpuinfo = fopen("/proc/cpuinfo", "r");
    if (cpuinfo == NULL) {
        perror("fopen /proc/cpuinfo");
        return;
    }

    char line[256];
    char model_name[256] = "";
    int core_count = 0;

    while (fgets(line, sizeof(line), cpuinfo)) {
        if (strncmp(line, "model name", 10) == 0 && model_name[0] == '\0') {
            sscanf(line, "model name : %[^\n]", model_name);
        }
        if (strncmp(line, "processor", 9) == 0) {
            core_count++;
        }
    }
    fclose(cpuinfo);

    printf("CPU Information:\n");
    printf("  Model Name     : %s\n", model_name);
    printf("  Logical Cores  : %d\n", core_count);
}


void get_memory_info() {
    FILE *meminfo = fopen("/proc/meminfo", "r");
    if (meminfo == NULL) {
        perror("fopen /proc/meminfo");
        return;
    }

    char line[256];
    printf("Memory Information:\n");
    while (fgets(line, sizeof(line), meminfo)) {
        if (strncmp(line, "MemTotal", 8) == 0 || strncmp(line, "MemFree", 7) == 0) {
            printf("  %s", line);
        }
    }
    fclose(meminfo);
}

void get_uptime() {
    FILE *uptime = fopen("/proc/uptime", "r");
    if (uptime == NULL) {
        perror("fopen /proc/uptime");
        return;
    }

    double up;
    fscanf(uptime, "%lf", &up);
    fclose(uptime);

    int days = (int)up / 86400;
    int hours = ((int)up % 86400) / 3600;
    int minutes = ((int)up % 3600) / 60;

    printf("Uptime: %d days, %d hours, %d minutes\n", days, hours, minutes);
}

void get_process_count() {
    struct sysinfo info;
    if (sysinfo(&info) != 0) {
        perror("sysinfo");
        return;
    }

    printf("Number of Processes: %d\n", info.procs);
}

int main() {
    uid_t uid = getuid();
    gid_t gid = getgid();
    struct passwd *pw = getpwuid(uid);
    
    if (pw == NULL) {
        perror("getpwuid");
        return 1;
    }

    const char *username = pw->pw_name;
    const char *home_dir = pw->pw_dir;
    const char *shell = pw->pw_shell;

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd");
        return 1;
    }

    char hostname[HOST_NAME_MAX];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        perror("gethostname");
        return 1;
    }

    struct utsname sys_info;
    if (uname(&sys_info) != 0) {
        perror("uname");
        return 1;
    }

    printf("User Information:\n");
    printf("-----------------\n");
    printf("Username      : %s\n", username);
    printf("User ID (UID) : %d\n", uid);
    printf("Group ID (GID): %d\n", gid);
    printf("Home Directory: %s\n", home_dir);
    printf("Shell         : %s\n", shell);
    printf("Current Dir   : %s\n", cwd);
    printf("Hostname      : %s\n\n", hostname);

    printf("System Information:\n");
    printf("-------------------\n");
    printf("OS Name       : %s\n", sys_info.sysname);
    printf("Node Name     : %s\n", sys_info.nodename);
    printf("Release       : %s\n", sys_info.release);
    printf("Version       : %s\n", sys_info.version);
    printf("Machine       : %s\n\n", sys_info.machine);

    get_ip_address();
    printf("\n");
    get_cpu_info();
    printf("\n");
    get_memory_info();
    printf("\n");
    get_uptime();
    get_process_count();

    printf("\nEnvironment Variables:\n");
    printf("----------------------\n");
    printf("PATH         : %s\n", getenv("PATH"));
    printf("LANG         : %s\n", getenv("LANG"));
    printf("TERM         : %s\n", getenv("TERM"));

    return 0;
}

