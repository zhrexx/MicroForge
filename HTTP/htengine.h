/*
 *  ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
 *  ┃ Project:  MicroForgeHTTP                   ┃
 *  ┃ File:     htengine.h                       ┃
 *  ┃ Author:   zhrexx                           ┃
 *  ┃ License:  NovaLicense                      ┃
 *  ┃━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┃     
 *  ┃ MFH is a WebAPI and HTTP server library.   ┃
 *  ┃ Its Fast and simple to use.                ┃
 *  ┃ It has it own features which other servers ┃
 *  ┃ don't have, Were trying to create our own  ┃
 *  ┃ API which would add some features but same ┃
 *  ┃ times be compatible with all Browsers      ┃
 *  ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
 */

#ifndef HTML_TEMPLATE_H
#define HTML_TEMPLATE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define HT_MAX_VAR_NAME    64
#define HT_MAX_VAR_VALUE   1024
#define HT_MAX_VARS        100
#define HT_MAX_TEMPLATE    16384
#define HT_MAX_ARRAY_SIZE  100
#define HT_MAX_STACK_SIZE  32

typedef enum {
    HT_TYPE_STRING,
    HT_TYPE_ARRAY,
    HT_TYPE_BOOL,
    HT_TYPE_INT
} HtVarType;

typedef struct {
    char* items[HT_MAX_ARRAY_SIZE];
    int count;
} HtArray;

typedef struct {
    char name[HT_MAX_VAR_NAME];
    HtVarType type;
    union {
        char str_value[HT_MAX_VAR_VALUE];
        HtArray array;
        bool bool_value;
        int int_value;
    } value;
} HtVar;

typedef struct {
    HtVar vars[HT_MAX_VARS];
    int var_count;
    struct {
        int loop_index[HT_MAX_STACK_SIZE];
        HtArray* current_array[HT_MAX_STACK_SIZE];
        int depth;
    } stack;
} HtmlTemplate;

typedef struct {
    const char* start;
    const char* end;
    size_t length;
} Token;

static Token get_token(const char** ptr) {
    Token token = {NULL, NULL, 0};
    const char* p = *ptr;
    
    while (*p && isspace(*p)) p++;
    
    if (!*p) return token;
    
    token.start = p;
    
    if (strncmp(p, "{%", 2) == 0 || strncmp(p, "{{", 2) == 0) {
        const char* end_marker = (p[1] == '%') ? "%}" : "}}";
        p += 2;
        token.end = strstr(p, end_marker);
        if (token.end) {
            token.length = token.end - token.start + 2;
            *ptr = token.end + 2;
            return token;
        }
    }
    
    while (*p) {
        if (strncmp(p, "{{", 2) == 0 || strncmp(p, "{%", 2) == 0) break;
        p++;
    }
    
    token.end = p;
    token.length = token.end - token.start;
    *ptr = p;
    
    return token;
}

static bool evaluate_condition(HtmlTemplate* tmpl, const char* condition) {
    char var_name[HT_MAX_VAR_NAME];
    const char* p = condition;
    while (*p && isspace(*p)) p++;
    
    size_t i = 0;
    while (*p && !isspace(*p) && i < HT_MAX_VAR_NAME - 1) {
        var_name[i++] = *p++;
    }
    var_name[i] = '\0';
    
    for (int i = 0; i < tmpl->var_count; i++) {
        if (strcmp(tmpl->vars[i].name, var_name) == 0) {
            switch (tmpl->vars[i].type) {
                case HT_TYPE_BOOL:
                    return tmpl->vars[i].value.bool_value;
                case HT_TYPE_INT:
                    return tmpl->vars[i].value.int_value != 0;
                case HT_TYPE_STRING:
                    return strlen(tmpl->vars[i].value.str_value) > 0 &&
                           strcmp(tmpl->vars[i].value.str_value, "false") != 0 &&
                           strcmp(tmpl->vars[i].value.str_value, "0") != 0;
                default:
                    return false;
            }
        }
    }
    return false;
}

HtmlTemplate* ht_create(void) {
    HtmlTemplate* tmpl = (HtmlTemplate*)malloc(sizeof(HtmlTemplate));
    if (tmpl) {
        tmpl->var_count = 0;
        tmpl->stack.depth = 0;
    }
    return tmpl;
}

void ht_destroy(HtmlTemplate* tmpl) {
    if (!tmpl) return;
    
    for (int i = 0; i < tmpl->var_count; i++) {
        if (tmpl->vars[i].type == HT_TYPE_ARRAY) {
            for (int j = 0; j < tmpl->vars[i].value.array.count; j++) {
                free(tmpl->vars[i].value.array.items[j]);
            }
        }
    }
    
    free(tmpl);
}

bool ht_set_var(HtmlTemplate* tmpl, const char* name, const char* value) {
    if (!tmpl || !name || !value || tmpl->var_count >= HT_MAX_VARS) {
        return false;
    }

    HtVar* var = &tmpl->vars[tmpl->var_count];
    strncpy(var->name, name, HT_MAX_VAR_NAME - 1);
    var->name[HT_MAX_VAR_NAME - 1] = '\0';
    var->type = HT_TYPE_STRING;
    strncpy(var->value.str_value, value, HT_MAX_VAR_VALUE - 1);
    var->value.str_value[HT_MAX_VAR_VALUE - 1] = '\0';
    
    tmpl->var_count++;
    return true;
}

bool ht_set_bool(HtmlTemplate* tmpl, const char* name, bool value) {
    if (!tmpl || !name || tmpl->var_count >= HT_MAX_VARS) {
        return false;
    }

    HtVar* var = &tmpl->vars[tmpl->var_count];
    strncpy(var->name, name, HT_MAX_VAR_NAME - 1);
    var->name[HT_MAX_VAR_NAME - 1] = '\0';
    var->type = HT_TYPE_BOOL;
    var->value.bool_value = value;
    
    tmpl->var_count++;
    return true;
}

bool ht_set_array(HtmlTemplate* tmpl, const char* name, const char** items, int count) {
    if (!tmpl || !name || !items || count > HT_MAX_ARRAY_SIZE || tmpl->var_count >= HT_MAX_VARS) {
        return false;
    }

    HtVar* var = &tmpl->vars[tmpl->var_count];
    strncpy(var->name, name, HT_MAX_VAR_NAME - 1);
    var->name[HT_MAX_VAR_NAME - 1] = '\0';
    var->type = HT_TYPE_ARRAY;
    
    var->value.array.count = count;
    for (int i = 0; i < count; i++) {
        var->value.array.items[i] = strdup(items[i]);
    }
    
    tmpl->var_count++;
    return true;
}

static const HtVar* find_var(HtmlTemplate* tmpl, const char* name) {
    for (int i = 0; i < tmpl->var_count; i++) {
        if (strcmp(tmpl->vars[i].name, name) == 0) {
            return &tmpl->vars[i];
        }
    }
    return NULL;
}

char* ht_render(HtmlTemplate* tmpl, const char* template_str) {
    if (!tmpl || !template_str) return NULL;

    char* result = (char*)malloc(HT_MAX_TEMPLATE);
    if (!result) return NULL;

    char* write_ptr = result;
    const char* read_ptr = template_str;
    size_t remaining = HT_MAX_TEMPLATE - 1;
    bool skip_section = false;
    int if_depth = 0;
    
    while (*read_ptr && remaining > 0) {
        Token token = get_token(&read_ptr);
        if (!token.start) break;
        
        if (token.start[0] == '{') {
            char command[32] = {0};
            char var_name[HT_MAX_VAR_NAME] = {0};

            if (strncmp(token.start, "{%", 2) == 0) {
                sscanf(token.start + 2, " %31s %63[^}]", command, var_name);
                
                if (strcmp(command, "if") == 0) {
                    if_depth++;
                    skip_section = !evaluate_condition(tmpl, var_name);
                }
                else if (strcmp(command, "endif") == 0) {
                    if_depth--;
                    skip_section = false;
                }
                else if (strcmp(command, "for") == 0) {
                    char array_name[HT_MAX_VAR_NAME];
                    if (sscanf(var_name, "%*s in %63s", array_name) == 1) {
                        const HtVar* array_var = find_var(tmpl, array_name);
                        if (array_var && array_var->type == HT_TYPE_ARRAY) {
                            tmpl->stack.current_array[tmpl->stack.depth] = (HtArray *) &array_var->value.array;
                            tmpl->stack.loop_index[tmpl->stack.depth] = 0;
                            tmpl->stack.depth++;
                        }
                    }
                }
                else if (strcmp(command, "endfor") == 0) {
                    if (tmpl->stack.depth > 0) {
                        tmpl->stack.depth--;
                    }
                }
            }
            else if (strncmp(token.start, "{{", 2) == 0 && !skip_section) {
                sscanf(token.start + 2, " %63[^}]", var_name);
                
                if (tmpl->stack.depth > 0 && strcmp(var_name, "loop_item") == 0) {
                    HtArray* current_array = tmpl->stack.current_array[tmpl->stack.depth - 1];
                    int index = tmpl->stack.loop_index[tmpl->stack.depth - 1];
                    
                    if (index < current_array->count) {
                        const char* value = current_array->items[index];
                        size_t len = strlen(value);
                        if (len < remaining) {
                            strcpy(write_ptr, value);
                            write_ptr += len;
                            remaining -= len;
                        }
                    }
                }
                else {
                    const HtVar* var = find_var(tmpl, var_name);
                    if (var) {
                        const char* value = NULL;
                        char temp[32];
                        
                        switch (var->type) {
                            case HT_TYPE_STRING:
                                value = var->value.str_value;
                                break;
                            case HT_TYPE_BOOL:
                                value = var->value.bool_value ? "true" : "false";
                                break;
                            case HT_TYPE_INT:
                                snprintf(temp, sizeof(temp), "%d", var->value.int_value);
                                value = temp;
                                break;
                            default:
                                value = "";
                        }
                        
                        size_t len = strlen(value);
                        if (len < remaining) {
                            strcpy(write_ptr, value);
                            write_ptr += len;
                            remaining -= len;
                        }
                    }
                }
            }
        }
        else if (!skip_section) {
            size_t len = token.length;
            if (len > remaining) len = remaining;
            memcpy(write_ptr, token.start, len);
            write_ptr += len;
            remaining -= len;
        }
    }
    
    *write_ptr = '\0';
    return result;
}

void ht_free_rendered(char* rendered) {
    free(rendered);
}

char* ht_load_file(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) return NULL;

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size > HT_MAX_TEMPLATE - 1) {
        fclose(file);
        return NULL;
    }

    char* content = (char*)malloc(size + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }

    size_t read = fread(content, 1, size, file);
    content[read] = '\0';
    fclose(file);

    return content;
}

#endif // HTML_TEMPLATE_H
