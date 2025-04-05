#include "xterm.h"

#ifdef XTERM_WINDOWS
    static HANDLE hStdout = INVALID_HANDLE_VALUE;
    static HANDLE hStdin = INVALID_HANDLE_VALUE;
    static DWORD originalConsoleMode = 0;
    static DWORD originalOutputMode = 0;
#else
    static struct termios original_term_attr;
    static struct termios raw_term_attr;
    static xterm_resize_handler_t resize_handler = NULL;
    
    static void handle_sigwinch(int sig) {
        if (sig == SIGWINCH && resize_handler != NULL) {
            resize_handler(xterm_get_size());
        }
    }
#endif

int xterm_init(void) {
#ifdef XTERM_WINDOWS
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    
    if (hStdout == INVALID_HANDLE_VALUE || hStdin == INVALID_HANDLE_VALUE) {
        return -1;
    }
    
    if (!GetConsoleMode(hStdin, &originalConsoleMode)) {
        return -1;
    }
    
    if (!GetConsoleMode(hStdout, &originalOutputMode)) {
        return -1;
    }
    
    DWORD mode = originalOutputMode;
    SetConsoleMode(hStdout, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT);
    
#else
    tcgetattr(STDIN_FILENO, &original_term_attr);
    memcpy(&raw_term_attr, &original_term_attr, sizeof(struct termios));
    
    printf("\033[?1049h");  // Use alternate screen buffer
    fflush(stdout);
#endif
    
    return 0;
}

void xterm_cleanup(void) {
#ifdef XTERM_WINDOWS
    if (hStdin != INVALID_HANDLE_VALUE) {
        SetConsoleMode(hStdin, originalConsoleMode);
    }
    if (hStdout != INVALID_HANDLE_VALUE) {
        SetConsoleMode(hStdout, originalOutputMode);
    }
#else
    printf("\033[?1049l");  // Restore main screen buffer
    tcsetattr(STDIN_FILENO, TCSANOW, &original_term_attr);
#endif
    xterm_show_cursor();
    xterm_reset_formatting();
}

void xterm_clear(void) {
#ifdef XTERM_WINDOWS
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD count;
    DWORD cellCount;
    COORD homeCoords = { 0, 0 };
    
    if (hStdout == INVALID_HANDLE_VALUE) return;
    
    GetConsoleScreenBufferInfo(hStdout, &csbi);
    cellCount = csbi.dwSize.X * csbi.dwSize.Y;
    
    FillConsoleOutputCharacter(hStdout, ' ', cellCount, homeCoords, &count);
    FillConsoleOutputAttribute(hStdout, csbi.wAttributes, cellCount, homeCoords, &count);
    SetConsoleCursorPosition(hStdout, homeCoords);
#else
    printf("\033[2J\033[H");
    fflush(stdout);
#endif
}

void xterm_clear_line(void) {
#ifdef XTERM_WINDOWS
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD count;
    
    if (hStdout == INVALID_HANDLE_VALUE) return;
    
    GetConsoleScreenBufferInfo(hStdout, &csbi);
    COORD pos = {0, csbi.dwCursorPosition.Y};
    DWORD length = csbi.dwSize.X;
    
    FillConsoleOutputCharacter(hStdout, ' ', length, pos, &count);
    FillConsoleOutputAttribute(hStdout, csbi.wAttributes, length, pos, &count);
    SetConsoleCursorPosition(hStdout, pos);
#else
    printf("\033[2K\r");
    fflush(stdout);
#endif
}

void xterm_clear_to_end(void) {
#ifdef XTERM_WINDOWS
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD count;
    
    if (hStdout == INVALID_HANDLE_VALUE) return;
    
    GetConsoleScreenBufferInfo(hStdout, &csbi);
    DWORD length = csbi.dwSize.X - csbi.dwCursorPosition.X;
    
    FillConsoleOutputCharacter(hStdout, ' ', length, csbi.dwCursorPosition, &count);
    FillConsoleOutputAttribute(hStdout, csbi.wAttributes, length, csbi.dwCursorPosition, &count);
#else
    printf("\033[K");
    fflush(stdout);
#endif
}

void xterm_move_cursor(int x, int y) {
#ifdef XTERM_WINDOWS
    COORD coord;
    coord.X = (SHORT)x;
    coord.Y = (SHORT)y;
    SetConsoleCursorPosition(hStdout, coord);
#else
    printf("\033[%d;%dH", y + 1, x + 1);
    fflush(stdout);
#endif
}

void xterm_move_up(int n) {
#ifdef XTERM_WINDOWS
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hStdout, &csbi);
    csbi.dwCursorPosition.Y -= (SHORT)n;
    if (csbi.dwCursorPosition.Y < 0) csbi.dwCursorPosition.Y = 0;
    SetConsoleCursorPosition(hStdout, csbi.dwCursorPosition);
#else
    printf("\033[%dA", n);
    fflush(stdout);
#endif
}

void xterm_move_down(int n) {
#ifdef XTERM_WINDOWS
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hStdout, &csbi);
    csbi.dwCursorPosition.Y += (SHORT)n;
    SetConsoleCursorPosition(hStdout, csbi.dwCursorPosition);
#else
    printf("\033[%dB", n);
    fflush(stdout);
#endif
}

void xterm_move_right(int n) {
#ifdef XTERM_WINDOWS
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hStdout, &csbi);
    csbi.dwCursorPosition.X += (SHORT)n;
    SetConsoleCursorPosition(hStdout, csbi.dwCursorPosition);
#else
    printf("\033[%dC", n);
    fflush(stdout);
#endif
}

void xterm_move_left(int n) {
#ifdef XTERM_WINDOWS
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hStdout, &csbi);
    csbi.dwCursorPosition.X -= (SHORT)n;
    if (csbi.dwCursorPosition.X < 0) csbi.dwCursorPosition.X = 0;
    SetConsoleCursorPosition(hStdout, csbi.dwCursorPosition);
#else
    printf("\033[%dD", n);
    fflush(stdout);
#endif
}

void xterm_save_cursor(void) {
#ifdef XTERM_WINDOWS
    printf("\033[s");
#else
    printf("\033[s");
    fflush(stdout);
#endif
}

void xterm_restore_cursor(void) {
#ifdef XTERM_WINDOWS
    printf("\033[u");
#else
    printf("\033[u");
    fflush(stdout);
#endif
}

void xterm_hide_cursor(void) {
#ifdef XTERM_WINDOWS
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hStdout, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hStdout, &cursorInfo);
#else
    printf("\033[?25l");
    fflush(stdout);
#endif
}

void xterm_show_cursor(void) {
#ifdef XTERM_WINDOWS
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hStdout, &cursorInfo);
    cursorInfo.bVisible = TRUE;
    SetConsoleCursorInfo(hStdout, &cursorInfo);
#else
    printf("\033[?25h");
    fflush(stdout);
#endif
}

void xterm_set_fg_color(xterm_color_t color) {
    const char* ansi_colors[] = {
        "\033[30m",  // BLACK
        "\033[31m",  // RED
        "\033[32m",  // GREEN
        "\033[33m",  // YELLOW
        "\033[34m",  // BLUE
        "\033[35m",  // MAGENTA
        "\033[36m",  // CYAN
        "\033[37m",  // WHITE
        "\033[39m",  // DEFAULT
        "\033[90m",  // BRIGHT_BLACK
        "\033[91m",  // BRIGHT_RED
        "\033[92m",  // BRIGHT_GREEN
        "\033[93m",  // BRIGHT_YELLOW
        "\033[94m",  // BRIGHT_BLUE
        "\033[95m",  // BRIGHT_MAGENTA
        "\033[96m",  // BRIGHT_CYAN
        "\033[97m"   // BRIGHT_WHITE
    };
    
    fputs(ansi_colors[color], stdout);
    fflush(stdout);
}

void xterm_set_bg_color(xterm_color_t color) {
    const char* ansi_bg_colors[] = {
        "\033[40m",  // BLACK
        "\033[41m",  // RED
        "\033[42m",  // GREEN
        "\033[43m",  // YELLOW
        "\033[44m",  // BLUE
        "\033[45m",  // MAGENTA
        "\033[46m",  // CYAN
        "\033[47m",  // WHITE
        "\033[49m",  // DEFAULT
        "\033[100m", // BRIGHT_BLACK
        "\033[101m", // BRIGHT_RED
        "\033[102m", // BRIGHT_GREEN
        "\033[103m", // BRIGHT_YELLOW
        "\033[104m", // BRIGHT_BLUE
        "\033[105m", // BRIGHT_MAGENTA
        "\033[106m", // BRIGHT_CYAN
        "\033[107m"  // BRIGHT_WHITE
    };
    
    fputs(ansi_bg_colors[color], stdout);
    fflush(stdout);
}

void xterm_set_fg_rgb(int r, int g, int b) {
    printf("\033[38;2;%d;%d;%dm", r, g, b);
    fflush(stdout);
}

void xterm_set_bg_rgb(int r, int g, int b) {
    printf("\033[48;2;%d;%d;%dm", r, g, b);
    fflush(stdout);
}

void xterm_set_attr(xterm_attr_t attr) {
    const char* ansi_attrs[] = {
        "\033[0m",   // RESET
        "\033[1m",   // BOLD
        "\033[2m",   // DIM
        "\033[3m",   // ITALIC
        "\033[4m",   // UNDERLINE
        "\033[5m",   // BLINK
        "\033[7m",   // REVERSE
        "\033[8m",   // HIDDEN
        "\033[9m"    // STRIKETHROUGH
    };
    
    fputs(ansi_attrs[attr], stdout);
    fflush(stdout);
}

void xterm_reset_formatting(void) {
    fputs("\033[0m", stdout);
    fflush(stdout);
}

xterm_size_t xterm_get_size(void) {
    xterm_size_t size = {0, 0};
    
#ifdef XTERM_WINDOWS
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hStdout, &csbi)) {
        size.width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        size.height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
#else
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != -1) {
        size.width = ws.ws_col;
        size.height = ws.ws_row;
    }
#endif
    
    return size;
}

void xterm_set_resize_handler(xterm_resize_handler_t handler) {
#ifdef XTERM_WINDOWS
    // Windows doesn't have SIGWINCH
#else
    resize_handler = handler;
    signal(SIGWINCH, handle_sigwinch);
#endif
}

void xterm_raw_mode(void) {
#ifdef XTERM_WINDOWS
    DWORD mode;
    GetConsoleMode(hStdin, &mode);
    SetConsoleMode(hStdin, mode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT));
#else
    raw_term_attr.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    raw_term_attr.c_oflag &= ~OPOST;
    raw_term_attr.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    raw_term_attr.c_cflag &= ~(CSIZE | PARENB);
    raw_term_attr.c_cflag |= CS8;
    raw_term_attr.c_cc[VMIN] = 1;
    raw_term_attr.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw_term_attr);
#endif
}

void xterm_normal_mode(void) {
#ifdef XTERM_WINDOWS
    SetConsoleMode(hStdin, originalConsoleMode);
#else
    tcsetattr(STDIN_FILENO, TCSANOW, &original_term_attr);
#endif
}

int xterm_kbhit(int timeout_ms) {
#ifdef XTERM_WINDOWS
    DWORD events = 0;
    INPUT_RECORD record;
    DWORD numRead = 0;
    
    if (WaitForSingleObject(hStdin, timeout_ms) != WAIT_OBJECT_0) {
        return -1;
    }
    
    if (!GetNumberOfConsoleInputEvents(hStdin, &events) || events == 0) {
        return -1;
    }
    
    if (!PeekConsoleInput(hStdin, &record, 1, &numRead) || numRead == 0) {
        return -1;
    }
    
    if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown) {
        return record.Event.KeyEvent.wVirtualKeyCode;
    }
    
    return -1;
#else
    fd_set rfds;
    struct timeval tv;
    
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);
    
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    int result = select(STDIN_FILENO + 1, &rfds, NULL, NULL, &tv);
    
    if (result > 0) {
        char c;
        if (read(STDIN_FILENO, &c, 1) == 1) {
            ungetc(c, stdin);
            return c;
        }
    }
    
    return -1;
#endif
}

int xterm_getch(void) {
#ifdef XTERM_WINDOWS
    DWORD numRead;
    INPUT_RECORD record;
    WCHAR ch;
    
    while (1) {
        if (ReadConsoleInput(hStdin, &record, 1, &numRead) && 
            record.EventType == KEY_EVENT && 
            record.Event.KeyEvent.bKeyDown) {
            ch = record.Event.KeyEvent.uChar.UnicodeChar;
            if (ch != 0) {
                return ch;
            }
        }
    }
#else
    return getchar();
#endif
}

int xterm_get_key(void) {
    int ch = xterm_getch();
    
#ifndef XTERM_WINDOWS
    if (ch == 27) {
        int next = xterm_kbhit(100);
        if (next == -1) {
            return XKEY_ESC;
        }
        
        if (next == '[') {
            xterm_getch();  // consume the '['
            ch = xterm_getch();
            
            switch (ch) {
                case 'A': return XKEY_UP;
                case 'B': return XKEY_DOWN;
                case 'C': return XKEY_RIGHT;
                case 'D': return XKEY_LEFT;
                case 'H': return XKEY_HOME;
                case 'F': return XKEY_END;
                case '2': xterm_getch(); return XKEY_INSERT;
                case '3': xterm_getch(); return XKEY_DELETE;
                case '5': xterm_getch(); return XKEY_PAGEUP;
                case '6': xterm_getch(); return XKEY_PAGEDOWN;
                case 'P': return XKEY_F1;
                case 'Q': return XKEY_F2;
                case 'R': return XKEY_F3;
                case 'S': return XKEY_F4;
            }
        } else if (next == 'O') {
            xterm_getch();  // consume the 'O'
            ch = xterm_getch();
            
            switch (ch) {
                case 'P': return XKEY_F1;
                case 'Q': return XKEY_F2;
                case 'R': return XKEY_F3;
                case 'S': return XKEY_F4;
            }
        }
    } else if (ch == 127) {
        return XKEY_BACKSPACE;
    } else if (ch == 10 || ch == 13) {
        return XKEY_ENTER;
    } else if (ch == 9) {
        return XKEY_TAB;
    }
#else
    // Windows key handling
    if (ch == VK_UP) return XKEY_UP;
    if (ch == VK_DOWN) return XKEY_DOWN;
    if (ch == VK_RIGHT) return XKEY_RIGHT;
    if (ch == VK_LEFT) return XKEY_LEFT;
    if (ch == VK_HOME) return XKEY_HOME;
    if (ch == VK_END) return XKEY_END;
    if (ch == VK_INSERT) return XKEY_INSERT;
    if (ch == VK_DELETE) return XKEY_DELETE;
    if (ch == VK_PRIOR) return XKEY_PAGEUP;
    if (ch == VK_NEXT) return XKEY_PAGEDOWN;
    if (ch == VK_F1) return XKEY_F1;
    if (ch == VK_F2) return XKEY_F2;
    if (ch == VK_F3) return XKEY_F3;
    if (ch == VK_F4) return XKEY_F4;
    if (ch == VK_F5) return XKEY_F5;
    if (ch == VK_F6) return XKEY_F6;
    if (ch == VK_F7) return XKEY_F7;
    if (ch == VK_F8) return XKEY_F8;
    if (ch == VK_F9) return XKEY_F9;
    if (ch == VK_F10) return XKEY_F10;
    if (ch == VK_F11) return XKEY_F11;
    if (ch == VK_F12) return XKEY_F12;
    if (ch == VK_ESCAPE) return XKEY_ESC;
    if (ch == VK_RETURN) return XKEY_ENTER;
    if (ch == VK_BACK) return XKEY_BACKSPACE;
    if (ch == VK_TAB) return XKEY_TAB;
#endif
    
    return ch;
}

xterm_point_t xterm_get_cursor_position(void) {
    xterm_point_t pos = {0, 0};
    
#ifdef XTERM_WINDOWS
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hStdout, &csbi)) {
        pos.x = csbi.dwCursorPosition.X;
        pos.y = csbi.dwCursorPosition.Y;
    }
#else
    char buf[32];
    int i = 0;
    
    fputs("\033[6n", stdout);
    fflush(stdout);
    
    while (i < 31) {
        buf[i] = getchar();
        if (buf[i] == 'R') {
            buf[i] = '\0';
            break;
        }
        i++;
    }
    
    if (i < 31 && buf[0] == '\033' && buf[1] == '[') {
        if (sscanf(buf + 2, "%d;%d", &pos.y, &pos.x) == 2) {
            pos.x--; // convert to 0-based
            pos.y--;
        }
    }
#endif
    
    return pos;
}

void xterm_draw_box(int x, int y, int width, int height, int style) {
    const char* styles[][6] = {
        {"+", "-", "+", "|", "+", "+"}, // 0: ASCII
        {"┌", "─", "┐", "│", "└", "┘"}, // 1: Single line
        {"╔", "═", "╗", "║", "╚", "╝"}, // 2: Double line
        {"╓", "─", "╖", "║", "╙", "╜"}, // 3: Mixed
        {"╒", "═", "╕", "│", "╘", "╛"}  // 4: Mixed
    };
    
    // Validate style
    if (style < 0 || style > 4) style = 0;
    
    // Draw top border
    xterm_move_cursor(x, y);
    fputs(styles[style][0], stdout);
    for (int i = 1; i < width - 1; i++) {
        fputs(styles[style][1], stdout);
    }
    fputs(styles[style][2], stdout);
    
    // Draw sides
    for (int j = 1; j < height - 1; j++) {
        xterm_move_cursor(x, y + j);
        fputs(styles[style][3], stdout);
        xterm_move_cursor(x + width - 1, y + j);
        fputs(styles[style][3], stdout);
    }
    
    // Draw bottom border
    xterm_move_cursor(x, y + height - 1);
    fputs(styles[style][4], stdout);
    for (int i = 1; i < width - 1; i++) {
        fputs(styles[style][1], stdout);
    }
    fputs(styles[style][5], stdout);
    
    fflush(stdout);
}

void xterm_draw_line(int x1, int y1, int x2, int y2, char c) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    int e2;
    
    while (1) {
        xterm_move_cursor(x1, y1);
        putchar(c);
        
        if (x1 == x2 && y1 == y2) break;
        
        e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
    
    fflush(stdout);
}

void xterm_fill_rect(int x, int y, int width, int height, char c) {
    for (int j = 0; j < height; j++) {
        xterm_move_cursor(x, y + j);
        for (int i = 0; i < width; i++) {
            putchar(c);
        }
    }
    
    fflush(stdout);
}

void xterm_set_title(const char* title) {
    printf("\033]0;%s\007", title);
    fflush(stdout);
}

void xterm_beep(void) {
    putchar('\a');
    fflush(stdout);
}

void xterm_set_cursor_style(int style) {
    /* Cursor styles:
     * 0: Default
     * 1: Block blink
     * 2: Block steady
     * 3: Underline blink
     * 4: Underline steady
     * 5: Bar blink
     * 6: Bar steady
     */
    printf("\033[%d q", style);
    fflush(stdout);
}

int xterm_get_capability(xterm_capability_t capability) {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        return 0;
    }

    switch (capability) {
        case XTERM_CAP_COLOR:
            return csbi.wAttributes & (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
        case XTERM_CAP_CURSOR:
            return 1;
        case XTERM_CAP_MOUSE:
            return 0;
        case XTERM_CAP_KEYPAD:
            return 0;
        case XTERM_CAP_AUTOREPEAT:
            return 0;
        case XTERM_CAP_RESIZE:
            return 0;
        case XTERM_CAP_BELL:
            return 1;
        case XTERM_CAP_SCROLL:
            return 1;
        default:
            return 0;
    }
#else
    char *term_type = getenv("TERM");
    if (term_type == NULL) {
        fprintf(stderr, "TERM environment variable not set.\n");
        return 0;
    }

    if (tgetent(NULL, term_type) != 1) {
        fprintf(stderr, "Error initializing termcap database for TERM: %s\n", term_type);
        return 0;
    }

    switch (capability) {
        case XTERM_CAP_COLOR:
            return tgetflag("Co") > 0;
        case XTERM_CAP_CURSOR:
            return tgetflag("cd") > 0;
        case XTERM_CAP_MOUSE:
            return tgetflag("Ms") > 0;
        case XTERM_CAP_KEYPAD:
            return tgetflag("kp") > 0;
        case XTERM_CAP_AUTOREPEAT:
            return tgetflag("ar") > 0;
        case XTERM_CAP_RESIZE:
            return tgetflag("ri") > 0;
        case XTERM_CAP_BELL:
            return tgetflag("bl") > 0;
        case XTERM_CAP_SCROLL:
            return tgetflag("sc") > 0;
        default:
            return 0;
    }
#endif
}

