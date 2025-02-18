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
#include <sys/statvfs.h>
#include <utmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

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

void get_disk_usage() {
    struct statvfs stat;
    if (statvfs("/", &stat) != 0) {
        perror("statvfs");
        return;
    }

    unsigned long block_size = stat.f_frsize;
    unsigned long total_blocks = stat.f_blocks;
    unsigned long free_blocks = stat.f_bfree;

    unsigned long total_space = total_blocks * block_size / (1024 * 1024);
    unsigned long free_space = free_blocks * block_size / (1024 * 1024);

    printf("Disk Usage:\n");
    printf("  Total Space   : %lu MB\n", total_space);
    printf("  Free Space    : %lu MB\n", free_space);
}

void get_load_average() {
    double loadavg[3];
    if (getloadavg(loadavg, 3) == -1) {
        perror("getloadavg");
        return;
    }

    printf("Load Average:\n");
    printf("  1 Minute      : %.2f\n", loadavg[0]);
    printf("  5 Minutes     : %.2f\n", loadavg[1]);
    printf("  15 Minutes    : %.2f\n", loadavg[2]);
}

void get_logged_in_users() {
    struct utmp *entry;
    setutent();
    printf("Logged In Users:\n");
    while ((entry = getutent()) != NULL) {
        if (entry->ut_type == USER_PROCESS) {
            printf("  %s\n", entry->ut_user);
        }
    }
    endutent();
}

void get_battery_status() {
    FILE *battery_info = fopen("/sys/class/power_supply/BAT0/uevent", "r");
    if (battery_info == NULL) {
        printf("Battery information not available.\n");
        return;
    }

    char line[256];
    printf("Battery Status:\n");
    while (fgets(line, sizeof(line), battery_info)) {
        if (strncmp(line, "POWER_SUPPLY_CAPACITY", 21) == 0) {
            printf("  Battery Level  : %s", line);
        } else if (strncmp(line, "POWER_SUPPLY_STATUS", 19) == 0) {
            printf("  Battery Status : %s", line);
        }
    }
    fclose(battery_info);
}

void get_gpu_info() {
    FILE *gpu_info = popen("lspci | grep VGA", "r");
    if (gpu_info == NULL) {
        printf("GPU information not available.\n");
        return;
    }

    char line[256];
    printf("GPU Information:\n");
    while (fgets(line, sizeof(line), gpu_info)) {
        printf("  %s", line);
    }
}

void get_temperature() {
    FILE *temp_file = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (temp_file == NULL) {
        printf("Temperature information not available.\n");
        return;
    }

    int temp;
    fscanf(temp_file, "%d", &temp);
    fclose(temp_file);

    printf("CPU Temperature: %.1f°C\n", temp / 1000.0);
}

void get_boot_time() {
    FILE *stat_file = fopen("/proc/stat", "r");
    if (stat_file == NULL) {
        perror("fopen /proc/stat");
        return;
    }

    char line[256];
    fgets(line, sizeof(line), stat_file); // Ignore the first line
    fgets(line, sizeof(line), stat_file); // Get the second line (boot time)

    long long boot_time;
    sscanf(line, "btime %lld", &boot_time);
    fclose(stat_file);

    time_t boot_time_t = (time_t)boot_time;
    struct tm *tm_info = localtime(&boot_time_t);
    
    char buffer[80];
    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", tm_info);

    printf("Boot Time: %s\n", buffer);
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
    get_disk_usage();
    get_load_average();
    get_logged_in_users();
    get_battery_status();
    get_gpu_info();
    get_temperature();
    get_boot_time();

    printf("\nEnvironment Variables:\n");
    printf("----------------------\n");
    printf("PATH         : %s\n", getenv("PATH"));
    printf("LANG         : %s\n", getenv("LANG"));
    printf("TERM         : %s\n", getenv("TERM"));

    return 0;
}

