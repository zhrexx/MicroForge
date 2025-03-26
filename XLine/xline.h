#ifndef XLINE_H
#define XLINE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#endif

#define MAX_LINE_LENGTH 2048
#define MAX_HISTORY 500
#define MAX_COMPLETIONS 50
#define MAX_KEYBINDINGS 20

typedef enum {
    XLINE_KEY_UP,
    XLINE_KEY_DOWN,
    XLINE_KEY_LEFT,
    XLINE_KEY_RIGHT,
    XLINE_KEY_DELETE,
    XLINE_KEY_BACKSPACE,
    XLINE_KEY_HOME,
    XLINE_KEY_END,
    XLINE_KEY_TAB,
    XLINE_KEY_CTRL_A,
    XLINE_KEY_CTRL_E,
    XLINE_KEY_CTRL_K,
    XLINE_KEY_CTRL_U
} XLineKeyType;

typedef char* (*XLineCompletionCallback)(const char*, int);
typedef void (*XLineKeyHandler)(void*);

typedef struct {
    char** entries;
    int count;
    int current;
    int max_entries;
} HistoryManager;

typedef struct {
    XLineKeyType key;
    XLineKeyHandler handler;
    void* context;
} KeyBinding;

typedef struct {
    char buffer[MAX_LINE_LENGTH];
    char* kill_buffer;
    int cursor_pos;
    int buffer_len;
    HistoryManager* history;
    XLineCompletionCallback completion_callback;
    char** completions;
    int completion_count;
    int current_completion;
    KeyBinding key_bindings[MAX_KEYBINDINGS];
    int key_binding_count;
} LineState;

typedef struct {
#ifdef _WIN32
    HANDLE console_handle;
    DWORD original_mode;
#else
    struct termios original_termios;
#endif
} TerminalState;

typedef struct {
    int width;
    int height;
} TerminalSize;

typedef struct {
    char* text;
    int start;
    int end;
} TextRange;

typedef struct {
    char* name;
    char* value;
} XLineVariable;

typedef enum {
    XLINE_COLOR_DEFAULT,
    XLINE_COLOR_RED,
    XLINE_COLOR_GREEN,
    XLINE_COLOR_YELLOW,
    XLINE_COLOR_BLUE,
    XLINE_COLOR_MAGENTA,
    XLINE_COLOR_CYAN,
    XLINE_COLOR_WHITE
} XLineColor;

typedef struct {
    HistoryManager history;
    TerminalState terminal_state;
    LineState line_state;
    XLineColor prompt_color;
    bool multiline_enabled;
    XLineVariable* variables;
    int variable_count;
    char color_codes[8][10];
    int terminal_width;
    bool hide_chars;
} XLineState;

XLineState g_xline_state = {
    .history = {0},
    .terminal_state = {0},
    .line_state = {0},
    .prompt_color = XLINE_COLOR_DEFAULT,
    .multiline_enabled = false,
    .variables = NULL,
    .variable_count = 0,
    .color_codes = {
        "\033[0m", "\033[31m", "\033[32m", "\033[33m", 
        "\033[34m", "\033[35m", "\033[36m", "\033[37m"
    }
};

char* xline_readline(const char* prompt);
void xline_add_history(const char* line);
void xline_clear_history(void);
void xline_init(void);
void xline_cleanup(void);

void xline_set_completion_callback(XLineCompletionCallback callback);
void xline_add_keybinding(XLineKeyType key, XLineKeyHandler handler, void* context);
void xline_set_prompt_color(XLineColor color);
void xline_enable_multiline(bool enable);
TextRange xline_get_word_at_cursor(void);
void xline_set_variable(const char* name, const char* value);
char* xline_get_variable(const char* name);

static void init_history(void);
static void free_history(void);
static int get_terminal_width(void);
static void enable_raw_mode(TerminalState* term_state);
static void disable_raw_mode(TerminalState* term_state);
static int read_key(void);
static void print_colored_prompt(const char* prompt, XLineColor color);
static void handle_completion(LineState* line_state);
static void kill_to_end(LineState* line_state);
static void kill_to_start(LineState* line_state);

static void print_colored_prompt(const char* prompt, XLineColor color) {
    printf("%s%s%s", g_xline_state.color_codes[color], prompt, g_xline_state.color_codes[XLINE_COLOR_DEFAULT]);
    fflush(stdout);
}

void xline_set_prompt_color(XLineColor color) {
    g_xline_state.prompt_color = color;
}

void xline_enable_multiline(bool enable) {
    g_xline_state.multiline_enabled = enable;
}

void xline_set_variable(const char* name, const char* value) {
    for (int i = 0; i < g_xline_state.variable_count; i++) {
        if (strcmp(g_xline_state.variables[i].name, name) == 0) {
            free(g_xline_state.variables[i].value);
            g_xline_state.variables[i].value = strdup(value);
            return;
        }
    }

    g_xline_state.variables = realloc(g_xline_state.variables, (g_xline_state.variable_count + 1) * sizeof(XLineVariable));
    g_xline_state.variables[g_xline_state.variable_count].name = strdup(name);
    g_xline_state.variables[g_xline_state.variable_count].value = strdup(value);
    g_xline_state.variable_count++;
}

char* xline_get_variable(const char* name) {
    for (int i = 0; i < g_xline_state.variable_count; i++) {
        if (strcmp(g_xline_state.variables[i].name, name) == 0) {
            return g_xline_state.variables[i].value;
        }
    }
    return NULL;
}

void xline_set_completion_callback(XLineCompletionCallback callback) {
    g_xline_state.line_state.completion_callback = callback;
}

void xline_add_keybinding(XLineKeyType key, XLineKeyHandler handler, void* context) {
    if (g_xline_state.line_state.key_binding_count < MAX_KEYBINDINGS) {
        g_xline_state.line_state.key_bindings[g_xline_state.line_state.key_binding_count].key = key;
        g_xline_state.line_state.key_bindings[g_xline_state.line_state.key_binding_count].handler = handler;
        g_xline_state.line_state.key_bindings[g_xline_state.line_state.key_binding_count].context = context;
        g_xline_state.line_state.key_binding_count++;
    }
}

TextRange xline_get_word_at_cursor(void) {
    TextRange range = {0};
    range.text = g_xline_state.line_state.buffer;
    
    int cursor = g_xline_state.line_state.cursor_pos;
    
    while (cursor > 0 && !isspace(g_xline_state.line_state.buffer[cursor - 1])) {
        cursor--;
    }
    range.start = cursor;
    
    cursor = g_xline_state.line_state.cursor_pos;
    while (cursor < g_xline_state.line_state.buffer_len && !isspace(g_xline_state.line_state.buffer[cursor])) {
        cursor++;
    }
    range.end = cursor;
    
    return range;
}

static void handle_completion(LineState* line_state) {
    if (!line_state->completion_callback) return;

    TextRange word = xline_get_word_at_cursor();
    char* partial = strndup(line_state->buffer + word.start, word.end - word.start);
    
    line_state->completions = malloc(MAX_COMPLETIONS * sizeof(char*));
    line_state->completion_count = 0;
    line_state->current_completion = -1;
    
    char* completion = line_state->completion_callback(partial, line_state->completion_count);
    while (completion && line_state->completion_count < MAX_COMPLETIONS) {
        line_state->completions[line_state->completion_count++] = completion;
        completion = line_state->completion_callback(partial, line_state->completion_count);
    }
    
    free(partial);
}

static void kill_to_end(LineState* line_state) {
    if (line_state->kill_buffer) {
        free(line_state->kill_buffer);
    }
    
    line_state->kill_buffer = strdup(line_state->buffer + line_state->cursor_pos);
    line_state->buffer[line_state->cursor_pos] = '\0';
    line_state->buffer_len = line_state->cursor_pos;
}

static void kill_to_start(LineState* line_state) {
    if (line_state->kill_buffer) {
        free(line_state->kill_buffer);
    }
    
    line_state->kill_buffer = strdup(line_state->buffer);
    line_state->kill_buffer[line_state->cursor_pos] = '\0';
    
    memmove(line_state->buffer, line_state->buffer + line_state->cursor_pos, 
            line_state->buffer_len - line_state->cursor_pos + 1);
    line_state->buffer_len -= line_state->cursor_pos;
    line_state->cursor_pos = 0;
}

static int get_terminal_width(void) {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
#else
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
#endif
}

static void enable_raw_mode(TerminalState* term_state) {
#ifdef _WIN32
    term_state->console_handle = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(term_state->console_handle, &term_state->original_mode);
    SetConsoleMode(term_state->console_handle, 
        ENABLE_PROCESSED_INPUT | ENABLE_MOUSE_INPUT);
#else
    tcgetattr(STDIN_FILENO, &term_state->original_termios);
    struct termios raw = term_state->original_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
#endif
}

static void disable_raw_mode(TerminalState* term_state) {
#ifdef _WIN32
    SetConsoleMode(term_state->console_handle, term_state->original_mode);
#else
    tcsetattr(STDIN_FILENO, TCSANOW, &term_state->original_termios);
#endif
}

static int read_key(void) {
#ifdef _WIN32
    int ch = _getch();
    if (ch == 224 || ch == 0) {
        ch = _getch();
        switch (ch) {
            case 72: return 'U';
            case 80: return 'D';
            case 75: return 'L';
            case 77: return 'R';
            case 83: return 127;
        }
    }
    return ch;
#else
    int ch;
    char seq[3];
    
    if ((ch = getchar()) == 27) {
        if (getchar() == '[') {
            switch (getchar()) {
                case 'A': return 'U';
                case 'B': return 'D';
                case 'C': return 'R';
                case 'D': return 'L';
                case '3': 
                    getchar();
                    return 127;
            }
        }
    }
    return ch;
#endif
}

static void init_history(void) {
    g_xline_state.history.entries = calloc(MAX_HISTORY, sizeof(char*));
    g_xline_state.history.count = 0;
    g_xline_state.history.current = -1;
    g_xline_state.history.max_entries = MAX_HISTORY;
}

static void free_history(void) {
    for (int i = 0; i < g_xline_state.history.count; i++) {
        free(g_xline_state.history.entries[i]);
    }
    free(g_xline_state.history.entries);
    g_xline_state.history.entries = NULL;
    g_xline_state.history.count = 0;
}

void xline_add_history(const char* line) {
    if (!g_xline_state.history.entries) {
        init_history();
    }

    if (g_xline_state.history.count > 0 && 
        strcmp(line, g_xline_state.history.entries[g_xline_state.history.count - 1]) == 0) {
        return;
    }

    if (g_xline_state.history.count >= g_xline_state.history.max_entries) {
        free(g_xline_state.history.entries[0]);
        memmove(g_xline_state.history.entries, g_xline_state.history.entries + 1, 
                (g_xline_state.history.max_entries - 1) * sizeof(char*));
        g_xline_state.history.count--;
    }

    g_xline_state.history.entries[g_xline_state.history.count] = strdup(line);
    g_xline_state.history.count++;
    g_xline_state.history.current = g_xline_state.history.count;
}

void xline_clear_history(void) {
    free_history();
}

char* xline_readline(const char* prompt) {
    LineState* line_state = &g_xline_state.line_state;
    memset(line_state, 0, sizeof(LineState));
    line_state->history = &g_xline_state.history;

    if (prompt) {
        print_colored_prompt(prompt, g_xline_state.prompt_color);
    }

    enable_raw_mode(&g_xline_state.terminal_state);

    while (1) {
        int ch = read_key();

        if (isprint(ch) && g_xline_state.hide_chars == false) {
            printf("%c", ch);
        }

        for (int i = 0; i < line_state->key_binding_count; i++) {
            if (line_state->key_bindings[i].key == ch) {
                line_state->key_bindings[i].handler(line_state->key_bindings[i].context);
                break;
            }
        }

        switch (ch) {
            case '\r':
            case '\n':
                printf("\n");
                disable_raw_mode(&g_xline_state.terminal_state);
                
                if (line_state->buffer_len > 0) {
                    xline_add_history(line_state->buffer);
                }
                
                line_state->buffer[line_state->buffer_len] = '\0';
                return strdup(line_state->buffer);

            case 127:
            case 8:
                if (line_state->cursor_pos > 0) {
                    printf("\b \b");
                    memmove(&line_state->buffer[line_state->cursor_pos - 1], 
                            &line_state->buffer[line_state->cursor_pos], 
                            line_state->buffer_len - line_state->cursor_pos);
                    line_state->cursor_pos--;
                    line_state->buffer_len--;
                    line_state->buffer[line_state->buffer_len] = '\0';
                    
                    printf("%s", line_state->buffer + line_state->cursor_pos);
        
                    for (int i = line_state->buffer_len - line_state->cursor_pos; i > 0; i--) {
                        printf(" ");
                    }
        
                    for (int i = line_state->buffer_len - line_state->cursor_pos; i > 0; i--) {
                        printf("\b");
                    }
                    fflush(stdout);
                }
                break;

            case 9:
                handle_completion(line_state);
                if (line_state->completion_count > 0) {
                    line_state->current_completion = 
                        (line_state->current_completion + 1) % line_state->completion_count;
                    
                    TextRange word = xline_get_word_at_cursor();
                    char* completion = line_state->completions[line_state->current_completion];
                    
                    memmove(line_state->buffer + word.end, 
                            line_state->buffer + word.start, 
                            line_state->buffer_len - word.start + 1);
                    memcpy(line_state->buffer + word.start, completion, strlen(completion));
                    
                    line_state->buffer_len = strlen(line_state->buffer);
                    line_state->cursor_pos = word.start + strlen(completion);
                }
                break;

            default:
                if (isprint(ch) && line_state->buffer_len < MAX_LINE_LENGTH - 1) {
                    memmove(&line_state->buffer[line_state->cursor_pos + 1], 
                            &line_state->buffer[line_state->cursor_pos], 
                            line_state->buffer_len - line_state->cursor_pos);
                    line_state->buffer[line_state->cursor_pos] = ch;
                    line_state->cursor_pos++;
                    line_state->buffer_len++;
                    line_state->buffer[line_state->buffer_len] = '\0';
                }
                break;
        }
    }
}

void xline_init(void) {
    init_history();
}

void xline_cleanup(void) {
    free_history();
    
    for (int i = 0; i < g_xline_state.variable_count; i++) {
        free(g_xline_state.variables[i].name);
        free(g_xline_state.variables[i].value);
    }
    free(g_xline_state.variables);
    g_xline_state.variables = NULL;
    g_xline_state.variable_count = 0;
}

#endif
