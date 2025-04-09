/* xwrapper.h -- Version 2.0
 *
 * A clean, simplified wrapper around the Crossline library
 * providing the same functionality with more intuitive function names.
 * 
 * This wrapper removes the "crossline_" prefix from all functions,
 * provides a cleaner API, and adds automatic memory management
 * while maintaining all original functionality.
 */

#ifndef XWRAPPER_H
#define XWRAPPER_H

#include <stdlib.h>
#include <string.h>
#include "crossline.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * Color definitions - reusing the original enums but with cleaner names
 */
typedef crossline_color_e color_e;

// Color constants
#define COLOR_DEFAULT       CROSSLINE_COLOR_DEFAULT
#define FGCOLOR_DEFAULT     CROSSLINE_FGCOLOR_DEFAULT
#define FGCOLOR_BLACK       CROSSLINE_FGCOLOR_BLACK
#define FGCOLOR_RED         CROSSLINE_FGCOLOR_RED
#define FGCOLOR_GREEN       CROSSLINE_FGCOLOR_GREEN
#define FGCOLOR_YELLOW      CROSSLINE_FGCOLOR_YELLOW
#define FGCOLOR_BLUE        CROSSLINE_FGCOLOR_BLUE
#define FGCOLOR_MAGENTA     CROSSLINE_FGCOLOR_MAGENTA
#define FGCOLOR_CYAN        CROSSLINE_FGCOLOR_CYAN
#define FGCOLOR_WHITE       CROSSLINE_FGCOLOR_WHITE
#define FGCOLOR_BRIGHT      CROSSLINE_FGCOLOR_BRIGHT
#define FGCOLOR_MASK        CROSSLINE_FGCOLOR_MASK

#define BGCOLOR_DEFAULT     CROSSLINE_BGCOLOR_DEFAULT
#define BGCOLOR_BLACK       CROSSLINE_BGCOLOR_BLACK
#define BGCOLOR_RED         CROSSLINE_BGCOLOR_RED
#define BGCOLOR_GREEN       CROSSLINE_BGCOLOR_GREEN
#define BGCOLOR_YELLOW      CROSSLINE_BGCOLOR_YELLOW
#define BGCOLOR_BLUE        CROSSLINE_BGCOLOR_BLUE
#define BGCOLOR_MAGENTA     CROSSLINE_BGCOLOR_MAGENTA
#define BGCOLOR_CYAN        CROSSLINE_BGCOLOR_CYAN
#define BGCOLOR_WHITE       CROSSLINE_BGCOLOR_WHITE
#define BGCOLOR_BRIGHT      CROSSLINE_BGCOLOR_BRIGHT
#define BGCOLOR_MASK        CROSSLINE_BGCOLOR_MASK

#define UNDERLINE           CROSSLINE_UNDERLINE

/* 
 * Type definitions 
 */
typedef crossline_completions_t completions_t;
typedef void (*completion_callback)(const char *buf, completions_t *pCompletions);

// Configuration constants
#define DEFAULT_BUFFER_SIZE 1024

/* 
 * Main readline functions 
 */

/* 
 * readline - Read a line from the user with automatic memory allocation
 * 
 * @param prompt: The prompt to display to the user
 * @return: A newly allocated string containing the input (must be freed by caller)
 *          or NULL if an error occurred
 */
static inline char* readline(const char *prompt) {
    char buffer[DEFAULT_BUFFER_SIZE];
    char* result = crossline_readline(prompt, buffer, DEFAULT_BUFFER_SIZE);
    
    if (!result) return NULL;
    
    char* output = (char*)malloc(strlen(buffer) + 1);
    if (!output) return NULL;
    
    strcpy(output, buffer);
    return output;
}

/* 
 * readline_len - Read a line with custom buffer size (advanced usage)
 * 
 * @param prompt: The prompt to display to the user
 * @param size: The maximum size of input to accept
 * @return: A newly allocated string containing the input (must be freed by caller)
 *          or NULL if an error occurred
 */
static inline char* readline_len(const char *prompt, int size) {
    char* buffer = (char*)malloc(size);
    if (!buffer) return NULL;
    
    char* result = crossline_readline(prompt, buffer, size);
    
    if (!result) {
        free(buffer);
        return NULL;
    }
    
    char* output = (char*)realloc(buffer, strlen(buffer) + 1);
    return output ? output : buffer;
}

/* 
 * readline_with_initial - Read a line with pre-filled content
 * 
 * @param prompt: The prompt to display to the user
 * @param initial: The initial text to pre-fill
 * @return: A newly allocated string containing the input (must be freed by caller)
 *          or NULL if an error occurred
 */
static inline char* readline_with_initial(const char *prompt, const char *initial) {
    char* buffer = (char*)malloc(DEFAULT_BUFFER_SIZE);
    if (!buffer) return NULL;
    
    strncpy(buffer, initial, DEFAULT_BUFFER_SIZE - 1);
    buffer[DEFAULT_BUFFER_SIZE - 1] = '\0';
    
    char* result = crossline_readline2(prompt, buffer, DEFAULT_BUFFER_SIZE);
    
    if (!result) {
        free(buffer);
        return NULL;
    }
    
    char* output = (char*)realloc(buffer, strlen(buffer) + 1);
    return output ? output : buffer;
}

static inline void set_delimiter(const char *delim) {
    crossline_delimiter_set(delim);
}

static inline int getch(void) {
    return crossline_getch();
}

/* 
 * History functions 
 */
static inline int history_save(const char *filename) {
    return crossline_history_save(filename);
}

static inline int history_load(const char *filename) {
    return crossline_history_load(filename);
}

static inline void history_show(void) {
    crossline_history_show();
}

static inline void history_clear(void) {
    crossline_history_clear();
}

/* 
 * Completion functions 
 */
static inline void register_completion(completion_callback pCbFunc) {
    crossline_completion_register(pCbFunc);
}

static inline void add_completion(completions_t *pCompletions, const char *word, const char *help) {
    crossline_completion_add(pCompletions, word, help);
}

static inline void add_completion_with_color(completions_t *pCompletions, const char *word, 
                                          color_e wcolor, const char *help, color_e hcolor) {
    crossline_completion_add_color(pCompletions, word, wcolor, help, hcolor);
}

static inline void set_hints(completions_t *pCompletions, const char *hints) {
    crossline_hints_set(pCompletions, hints);
}

static inline void set_hints_with_color(completions_t *pCompletions, const char *hints, color_e color) {
    crossline_hints_set_color(pCompletions, hints, color);
}

/* 
 * Paging functions 
 */
static inline int set_paging(int enable) {
    return crossline_paging_set(enable);
}

static inline int check_paging(int line_len) {
    return crossline_paging_check(line_len);
}

/* 
 * Screen and cursor functions 
 */
static inline void get_screen_size(int *pRows, int *pCols) {
    crossline_screen_get(pRows, pCols);
}

static inline void clear_screen(void) {
    crossline_screen_clear();
}

static inline int get_cursor_position(int *pRow, int *pCol) {
    return crossline_cursor_get(pRow, pCol);
}

static inline void set_cursor_position(int row, int col) {
    crossline_cursor_set(row, col);
}

static inline void move_cursor(int row_off, int col_off) {
    crossline_cursor_move(row_off, col_off);
}

static inline void hide_cursor(int bHide) {
    crossline_cursor_hide(bHide);
}

/* 
 * Color functions 
 */
static inline void set_color(color_e color) {
    crossline_color_set(color);
}

static inline void set_prompt_color(color_e color) {
    crossline_prompt_color_set(color);
}

#ifdef __cplusplus
}
#endif

#endif /* XWRAPPER_H */
