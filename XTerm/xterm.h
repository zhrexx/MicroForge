#ifndef XTERM_H
#define XTERM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
    #define XTERM_WINDOWS
#else
    #include <unistd.h>
    #include <termios.h>
    #include <sys/ioctl.h>
    #include <signal.h>
    #include <time.h>
    #include <fcntl.h>
    #include <termcap.h>
    #define XTERM_POSIX
#endif

typedef enum {
    XCOLOR_BLACK,
    XCOLOR_RED,
    XCOLOR_GREEN,
    XCOLOR_YELLOW,
    XCOLOR_BLUE,
    XCOLOR_MAGENTA,
    XCOLOR_CYAN,
    XCOLOR_WHITE,
    XCOLOR_DEFAULT,
    XCOLOR_BRIGHT_BLACK,
    XCOLOR_BRIGHT_RED,
    XCOLOR_BRIGHT_GREEN,
    XCOLOR_BRIGHT_YELLOW,
    XCOLOR_BRIGHT_BLUE,
    XCOLOR_BRIGHT_MAGENTA,
    XCOLOR_BRIGHT_CYAN,
    XCOLOR_BRIGHT_WHITE
} xterm_color_t;

typedef enum {
    XATTR_RESET,
    XATTR_BOLD,
    XATTR_DIM,
    XATTR_ITALIC,
    XATTR_UNDERLINE,
    XATTR_BLINK,
    XATTR_REVERSE,
    XATTR_HIDDEN,
    XATTR_STRIKETHROUGH
} xterm_attr_t;

typedef struct {
    int width;
    int height;
} xterm_size_t;

typedef enum {
    XKEY_UP = 1000,
    XKEY_DOWN,
    XKEY_RIGHT,
    XKEY_LEFT,
    XKEY_HOME,
    XKEY_END,
    XKEY_PAGEUP,
    XKEY_PAGEDOWN,
    XKEY_INSERT,
    XKEY_DELETE,
    XKEY_F1,
    XKEY_F2,
    XKEY_F3,
    XKEY_F4,
    XKEY_F5,
    XKEY_F6,
    XKEY_F7,
    XKEY_F8,
    XKEY_F9,
    XKEY_F10,
    XKEY_F11,
    XKEY_F12,
    XKEY_ESC,
    XKEY_ENTER,
    XKEY_BACKSPACE,
    XKEY_TAB
} xterm_special_key_t;

typedef enum {
    XTERM_CAP_COLOR,
    XTERM_CAP_CURSOR,
    XTERM_CAP_MOUSE,
    XTERM_CAP_KEYPAD,
    XTERM_CAP_AUTOREPEAT,
    XTERM_CAP_RESIZE,
    XTERM_CAP_BELL,
    XTERM_CAP_SCROLL,
    XTERM_CAP_MAX
} xterm_capability_t;


typedef struct {
    int x;
    int y;
} xterm_point_t;

typedef void (*xterm_resize_handler_t)(xterm_size_t size);

int xterm_init(void);
void xterm_cleanup(void);

void xterm_clear(void);
void xterm_clear_line(void);
void xterm_clear_to_end(void);

void xterm_move_cursor(int x, int y);
void xterm_move_up(int n);
void xterm_move_down(int n);
void xterm_move_right(int n);
void xterm_move_left(int n);
void xterm_save_cursor(void);
void xterm_restore_cursor(void);
void xterm_hide_cursor(void);
void xterm_show_cursor(void);
xterm_point_t xterm_get_cursor_position(void);
void xterm_set_cursor_style(int style);

void xterm_set_fg_color(xterm_color_t color);
void xterm_set_bg_color(xterm_color_t color);
void xterm_set_fg_rgb(int r, int g, int b);
void xterm_set_bg_rgb(int r, int g, int b);
void xterm_set_attr(xterm_attr_t attr);
void xterm_reset_formatting(void);

xterm_size_t xterm_get_size(void);
void xterm_set_resize_handler(xterm_resize_handler_t handler);

void xterm_raw_mode(void);
void xterm_normal_mode(void);

int xterm_kbhit(int timeout_ms);

int xterm_getch(void);
int xterm_get_key(void);

void xterm_draw_box(int x, int y, int width, int height, int style);
void xterm_draw_line(int x1, int y1, int x2, int y2, char c);
void xterm_fill_rect(int x, int y, int width, int height, char c);

void xterm_set_title(const char* title);
int xterm_get_capability(xterm_capability_t capability);

void xterm_beep(void);

#endif
