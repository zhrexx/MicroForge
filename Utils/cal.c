#include <stdio.h>
#include <time.h>

int get_day_of_week(int year, int month) {
    if (month < 3) {
        month += 12;
        year--;
    }
    int k = year % 100;
    int j = year / 100;
    int day = (1 + 13 * (month + 1) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;
    return (day + 6) % 7;
}

int get_days_in_month(int year, int month) {
    if (month == 2) {
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            return 29;
        } else {
            return 28;
        }
    }
    if (month == 4 || month == 6 || month == 9 || month == 11) {
        return 30;
    }
    return 31;
}

void print_calendar(int year, int month) {
    printf("     %d-%02d\n", year, month);
    printf(" Su Mo Tu We Th Fr Sa\n");

    int days_in_month = get_days_in_month(year, month);
    int starting_day = get_day_of_week(year, month);

    for (int i = 0; i < starting_day; i++) {
        printf("   ");
    }

    time_t t;
    struct tm *tm_info;
    time(&t);
    tm_info = localtime(&t);
    int current_day = tm_info->tm_mday;
    int current_month = tm_info->tm_mon + 1;
    int current_year = tm_info->tm_year + 1900;

    for (int day = 1; day <= days_in_month; day++) {
        if (year == current_year && month == current_month && day == current_day) {
            #ifdef _WIN32
                printf("[%2d]", day);
            #else
                printf("\x1b[48;5;15m\x1b[30m%2d\x1b[39m\x1b[49m", day);
            #endif
        } else {
            printf("%2d ", day);
        }
        if ((starting_day + day) % 7 == 0) {
            printf("\n");
        }
    }

    printf("\n");
}

int main() {
    time_t t;
    struct tm *tm_info;

    time(&t);
    tm_info = localtime(&t);

    int year = tm_info->tm_year + 1900;
    int month = tm_info->tm_mon + 1;

    print_calendar(year, month);
    return 0;
}

