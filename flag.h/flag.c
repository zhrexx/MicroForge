#include "flag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_FLAGS 100
#define MAX_REMAINING_ARGS 100
#define MAX_GROUPS 10
#define MAX_FLAGS_PER_GROUP 50
#define MAX_HELP_TEXT_SIZE 8192

static Flag flags[MAX_FLAGS];
static int flag_count = 0;
static char* remaining_args[MAX_REMAINING_ARGS];
static int remaining_args_count = 0;
static const char* program_name = NULL;
static const char* program_description = NULL;
static FlagConfig config = {0};
static char* groups[MAX_GROUPS] = {0};
static int group_count = 0;
static int flags_in_groups[MAX_GROUPS][MAX_FLAGS_PER_GROUP] = {0};
static int flags_in_group_count[MAX_GROUPS] = {0};
static char help_text[MAX_HELP_TEXT_SIZE] = {0};

void flag_init(const char* name, const char* description) {
    program_name = name;
    program_description = description;
    flag_count = 0;
    remaining_args_count = 0;
    
    config.name = name;
    config.description = description;
    config.auto_help = true;
    config.show_defaults = true;
    config.usage_pattern = "[OPTIONS] [ARGS]";
    
    for (int i = 0; i < MAX_GROUPS; i++) {
        flags_in_group_count[i] = 0;
    }
    group_count = 0;
}

void flag_set_config(FlagConfig cfg) {
    config = cfg;
}

FlagConfig flag_get_config(void) {
    return config;
}

static void add_flag(const char* name, const char* shortname, void* value, 
                    FlagType type, const char* help, bool required) {
    if (flag_count >= MAX_FLAGS) {
        fprintf(stderr, "Error: Maximum number of flags exceeded.\n");
        exit(EXIT_FAILURE);
    }
    
    flags[flag_count].name = name;
    flags[flag_count].shortname = shortname;
    flags[flag_count].help = help;
    flags[flag_count].type = type;
    flags[flag_count].value = value;
    flags[flag_count].required = required;
    flags[flag_count].provided = false;
    flags[flag_count].hidden = false;
    flags[flag_count].env_var = NULL;
    
    flag_count++;
}

void flag_bool(const char* name, const char* shortname, bool* value, 
              bool default_value, const char* help, bool required) {
    *value = default_value;
    add_flag(name, shortname, value, FLAG_BOOL, help, required);
    flags[flag_count-1].default_value.bool_default = default_value;
}

void flag_string(const char* name, const char* shortname, char** value, 
                const char* default_value, const char* help, bool required) {
    *value = (char*)default_value;
    add_flag(name, shortname, value, FLAG_STRING, help, required);
    flags[flag_count-1].default_value.string_default = default_value;
}

void flag_int(const char* name, const char* shortname, int* value, 
             int default_value, const char* help, bool required) {
    *value = default_value;
    add_flag(name, shortname, value, FLAG_INT, help, required);
    flags[flag_count-1].default_value.int_default = default_value;
}

void flag_float(const char* name, const char* shortname, float* value, 
               float default_value, const char* help, bool required) {
    *value = default_value;
    add_flag(name, shortname, value, FLAG_FLOAT, help, required);
    flags[flag_count-1].default_value.float_default = default_value;
}

void flag_double(const char* name, const char* shortname, double* value,
                double default_value, const char* help, bool required) {
    *value = default_value;
    add_flag(name, shortname, value, FLAG_DOUBLE, help, required);
    flags[flag_count-1].default_value.double_default = default_value;
}

void flag_enum(const char* name, const char* shortname, int* value, int default_value,
              const char** options, int options_count, const char* help, bool required) {
    *value = default_value;
    add_flag(name, shortname, value, FLAG_ENUM, help, required);
    flags[flag_count-1].default_value.enum_default = default_value;
    flags[flag_count-1].extra_data.enum_data.options = options;
    flags[flag_count-1].extra_data.enum_data.options_count = options_count;
}

void flag_callback(const char* name, const char* shortname, 
                  bool (*callback)(const char* value, void* user_data),
                  void* user_data, const char* help, bool required) {
    add_flag(name, shortname, NULL, FLAG_CALLBACK, help, required);
    flags[flag_count-1].extra_data.callback_data.func = callback;
    flags[flag_count-1].extra_data.callback_data.user_data = user_data;
}

void flag_set_hidden(const char* name, bool hidden) {
    Flag* flag = flag_get(name);
    if (flag) {
        flag->hidden = hidden;
    }
}

void flag_set_env_var(const char* name, const char* env_var) {
    Flag* flag = flag_get(name);
    if (flag) {
        flag->env_var = env_var;
    }
}

char* flag_get_env(const char* name) {
    return getenv(name);
}

Flag* flag_get(const char* name) {
    for (int i = 0; i < flag_count; i++) {
        if (strcmp(flags[i].name, name) == 0) {
            return &flags[i];
        }
    }
    return NULL;
}

bool flag_was_provided(const char* name) {
    Flag* flag = flag_get(name);
    return flag ? flag->provided : false;
}

static Flag* find_flag_by_name(const char* name) {
    for (int i = 0; i < flag_count; i++) {
        if (strcmp(flags[i].name, name) == 0) {
            return &flags[i];
        }
    }
    return NULL;
}

static Flag* find_flag_by_shortname(const char shortname) {
    for (int i = 0; i < flag_count; i++) {
        if (flags[i].shortname != NULL && flags[i].shortname[0] == shortname) {
            return &flags[i];
        }
    }
    return NULL;
}

static bool check_enum_value(const char* value, Flag* flag) {
    for (int i = 0; i < flag->extra_data.enum_data.options_count; i++) {
        if (strcmp(value, flag->extra_data.enum_data.options[i]) == 0) {
            *(int*)flag->value = i;
            return true;
        }
    }
    return false;
}

static bool parse_flag_value(Flag* flag, const char* arg) {
    switch (flag->type) {
        case FLAG_BOOL:
            *(bool*)flag->value = true;
            return true;
        
        case FLAG_STRING:
            *(char**)flag->value = (char*)arg;
            return true;
        
        case FLAG_INT: {
            char* endptr;
            int value = (int)strtol(arg, &endptr, 0);
            if (*endptr != '\0') {
                return false;
            }
            *(int*)flag->value = value;
            return true;
        }
        
        case FLAG_FLOAT: {
            char* endptr;
            float value = strtof(arg, &endptr);
            if (*endptr != '\0') {
                return false;
            }
            *(float*)flag->value = value;
            return true;
        }
        
        case FLAG_DOUBLE: {
            char* endptr;
            double value = strtod(arg, &endptr);
            if (*endptr != '\0') {
                return false;
            }
            *(double*)flag->value = value;
            return true;
        }
        
        case FLAG_ENUM:
            return check_enum_value(arg, flag);
            
        case FLAG_CALLBACK:
            return flag->extra_data.callback_data.func(arg, flag->extra_data.callback_data.user_data);
            
        default:
            return false;
    }
}

static void apply_env_vars(void) {
    for (int i = 0; i < flag_count; i++) {
        if (flags[i].env_var && !flags[i].provided) {
            const char* env_value = getenv(flags[i].env_var);
            if (env_value) {
                parse_flag_value(&flags[i], env_value);
                flags[i].provided = true;
            }
        }
    }
}

bool flag_parse(int argc, char** argv) {
    bool parsing_flags = true;
    
    if (config.pre_parse_hook) {
        config.pre_parse_hook(argc, argv);
    }
    
    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        
        if (strcmp(arg, "--") == 0) {
            parsing_flags = false;
            continue;
        }
        
        if (config.auto_help && (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0)) {
            flag_print_help();
            exit(EXIT_SUCCESS);
        }
        
        if (config.version && (strcmp(arg, "--version") == 0 || strcmp(arg, "-v") == 0)) {
            flag_print_version();
            exit(EXIT_SUCCESS);
        }
        
        if (parsing_flags && arg[0] == '-') {
            Flag* flag = NULL;
            const char* value = NULL;
            
            if (arg[1] == '-') {
                char* equals = strchr(arg + 2, '=');
                if (equals) {
                    *equals = '\0';
                    flag = find_flag_by_name(arg + 2);
                    value = equals + 1;
                    *equals = '=';
                } else {
                    flag = find_flag_by_name(arg + 2);
                    if (flag && flag->type != FLAG_BOOL && i + 1 < argc) {
                        value = argv[i + 1];
                        i++;
                    }
                }
            } else {
                flag = find_flag_by_shortname(arg[1]);
                if (flag && flag->type != FLAG_BOOL && arg[2] == '\0') {
                    if (i + 1 < argc) {
                        value = argv[i + 1];
                        i++;
                    }
                } else if (flag && flag->type != FLAG_BOOL) {
                    value = arg + 2;
                } else if (arg[2] == '\0') {
                    flag = find_flag_by_shortname(arg[1]);
                } else {
                    for (int j = 1; arg[j]; j++) {
                        flag = find_flag_by_shortname(arg[j]);
                        if (flag) {
                            if (flag->type == FLAG_BOOL) {
                                *(bool*)flag->value = true;
                                flag->provided = true;
                            } else {
                                if (arg[j+1]) {
                                    value = arg + j + 1;
                                    if (!parse_flag_value(flag, value)) {
                                        fprintf(stderr, "Error: Invalid value for flag %s: %s\n", 
                                                flag->name, value);
                                        return false;
                                    }
                                    flag->provided = true;
                                } else if (i + 1 < argc) {
                                    value = argv[i + 1];
                                    i++;
                                    if (!parse_flag_value(flag, value)) {
                                        fprintf(stderr, "Error: Invalid value for flag %s: %s\n", 
                                                flag->name, value);
                                        return false;
                                    }
                                    flag->provided = true;
                                } else {
                                    fprintf(stderr, "Error: Missing value for flag %s\n", flag->name);
                                    return false;
                                }
                                break;
                            }
                        } else if (!config.allow_unknown_flags) {
                            fprintf(stderr, "Error: Unknown flag: -%c\n", arg[j]);
                            return false;
                        }
                    }
                    continue;
                }
            }
            
            if (flag) {
                flag->provided = true;
                
                if (flag->type == FLAG_BOOL) {
                    *(bool*)flag->value = true;
                } else if (value) {
                    if (!parse_flag_value(flag, value)) {
                        fprintf(stderr, "Error: Invalid value for flag %s: %s\n", 
                                flag->name, value);
                        return false;
                    }
                } else {
                    fprintf(stderr, "Error: Missing value for flag %s\n", flag->name);
                    return false;
                }
            } else if (!config.allow_unknown_flags) {
                fprintf(stderr, "Error: Unknown flag: %s\n", arg);
                return false;
            }
        } else {
            if (remaining_args_count < MAX_REMAINING_ARGS) {
                remaining_args[remaining_args_count++] = argv[i];
            } else {
                fprintf(stderr, "Error: Too many arguments\n");
                return false;
            }
        }
    }
    
    apply_env_vars();
    
    if (config.post_parse_hook) {
        config.post_parse_hook(argc, argv);
    }
    
    return flag_validate_required();
}

bool flag_validate_required(void) {
    for (int i = 0; i < flag_count; i++) {
        if (flags[i].required && !flags[i].provided) {
            fprintf(stderr, "Error: Required flag --%s not provided\n", flags[i].name);
            return false;
        }
    }
    return true;
}

void flag_set_groups(const char** group_names, int count) {
    if (count > MAX_GROUPS) {
        fprintf(stderr, "Error: Too many groups specified\n");
        return;
    }
    
    group_count = count;
    for (int i = 0; i < count; i++) {
        groups[i] = (char*)group_names[i];
        flags_in_group_count[i] = 0;
    }
}

void flag_add_to_group(const char* flag_name, const char* group_name) {
    int flag_idx = -1;
    int group_idx = -1;
    
    for (int i = 0; i < flag_count; i++) {
        if (strcmp(flags[i].name, flag_name) == 0) {
            flag_idx = i;
            break;
        }
    }
    
    if (flag_idx == -1) {
        fprintf(stderr, "Error: Unknown flag: %s\n", flag_name);
        return;
    }
    
    for (int i = 0; i < group_count; i++) {
        if (strcmp(groups[i], group_name) == 0) {
            group_idx = i;
            break;
        }
    }
    
    if (group_idx == -1) {
        fprintf(stderr, "Error: Unknown group: %s\n", group_name);
        return;
    }
    
    if (flags_in_group_count[group_idx] < MAX_FLAGS_PER_GROUP) {
        flags_in_groups[group_idx][flags_in_group_count[group_idx]++] = flag_idx;
    } else {
        fprintf(stderr, "Error: Too many flags in group %s\n", group_name);
    }
}

const char* flag_format_help(void) {
    int pos = 0;
    
    pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, 
                   "Usage: %s %s\n\n", program_name, config.usage_pattern);
    
    if (program_description) {
        pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "%s\n\n", program_description);
    }
    
    if (config.args_description) {
        pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "%s\n\n", config.args_description);
    }
    
    int printed_flags = 0;
    
    if (group_count > 0) {
        for (int g = 0; g < group_count; g++) {
            pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "%s:\n", groups[g]);
            
            for (int i = 0; i < flags_in_group_count[g]; i++) {
                int f = flags_in_groups[g][i];
                
                if (!flags[f].hidden) {
                    printed_flags++;
                    pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "  ");
                    
                    if (flags[f].shortname) {
                        pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "-%s, ", flags[f].shortname);
                    } else {
                        pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "    ");
                    }
                    
                    pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "--%s", flags[f].name);
                    
                    switch (flags[f].type) {
                        case FLAG_BOOL:
                            break;
                        case FLAG_STRING:
                            pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "=STRING");
                            break;
                        case FLAG_INT:
                            pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "=INT");
                            break;
                        case FLAG_FLOAT:
                        case FLAG_DOUBLE:
                            pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "=FLOAT");
                            break;
                        case FLAG_ENUM:
                            pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "=ENUM");
                            break;
                        case FLAG_CALLBACK:
                            pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "=VALUE");
                            break;
                    }
                    
                    pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "\t%s", flags[f].help);
                    
                    if (config.show_defaults) {
                        switch (flags[f].type) {
                            case FLAG_BOOL:
                                pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, " (default: %s)", 
                                              flags[f].default_value.bool_default ? "true" : "false");
                                break;
                            case FLAG_STRING:
                                if (flags[f].default_value.string_default) {
                                    pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, " (default: \"%s\")", 
                                                  flags[f].default_value.string_default);
                                }
                                break;
                            case FLAG_INT:
                                pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, " (default: %d)", 
                                              flags[f].default_value.int_default);
                                break;
                            case FLAG_FLOAT:
                                pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, " (default: %.2f)", 
                                              flags[f].default_value.float_default);
                                break;
                            case FLAG_DOUBLE:
                                pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, " (default: %.2f)", 
                                              flags[f].default_value.double_default);
                                break;
                            case FLAG_ENUM:
                                pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, " (default: %s)", 
                                              flags[f].extra_data.enum_data.options[flags[f].default_value.enum_default]);
                                break;
                            case FLAG_CALLBACK:
                                break;
                        }
                    }
                    
                    if (flags[f].env_var) {
                        pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, " (env: %s)", flags[f].env_var);
                    }
                    
                    if (flags[f].required) {
                        pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, " [required]");
                    }
                    
                    pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "\n");
                }
            }
            
            pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "\n");
        }
    }
    
    if (printed_flags < flag_count) {
        pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "Options:\n");
        
        for (int i = 0; i < flag_count; i++) {
            bool in_group = false;
            
            for (int g = 0; g < group_count; g++) {
                for (int j = 0; j < flags_in_group_count[g]; j++) {
                    if (flags_in_groups[g][j] == i) {
                        in_group = true;
                        break;
                    }
                }
                if (in_group) break;
            }
            
            if (!in_group && !flags[i].hidden) {
                pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "  ");
                
                if (flags[i].shortname) {
                    pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "-%s, ", flags[i].shortname);
                } else {
                    pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "    ");
                }
                
                pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "--%s", flags[i].name);
                
                switch (flags[i].type) {
                    case FLAG_BOOL:
                        break;
                    case FLAG_STRING:
                        pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "=STRING");
                        break;
                    case FLAG_INT:
                        pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "=INT");
                        break;
                    case FLAG_FLOAT:
                    case FLAG_DOUBLE:
                        pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "=FLOAT");
                        break;
                    case FLAG_ENUM:
                        pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "=ENUM");
                        break;
                    case FLAG_CALLBACK:
                        pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "=VALUE");
                        break;
                }
                
                pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "\t%s", flags[i].help);
                
                if (config.show_defaults) {
                    switch (flags[i].type) {
                        case FLAG_BOOL:
                            pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, " (default: %s)", 
                                          flags[i].default_value.bool_default ? "true" : "false");
                            break;
                        case FLAG_STRING:
                            if (flags[i].default_value.string_default) {
                                pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, " (default: \"%s\")", 
                                              flags[i].default_value.string_default);
                            }
                            break;
                        case FLAG_INT:
                            pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, " (default: %d)", 
                                          flags[i].default_value.int_default);
                            break;
                        case FLAG_FLOAT:
                            pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, " (default: %.2f)", 
                                          flags[i].default_value.float_default);
                            break;
                        case FLAG_DOUBLE:
                            pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, " (default: %.2f)", 
                                          flags[i].default_value.double_default);
                            break;
                        case FLAG_ENUM:
                            pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, " (default: %s)", 
                                          flags[i].extra_data.enum_data.options[flags[i].default_value.enum_default]);
                            break;
                        case FLAG_CALLBACK:
                            break;
                    }
                }
                
                if (flags[i].env_var) {
                    pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, " (env: %s)", flags[i].env_var);
                }
                
                if (flags[i].required) {
                    pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, " [required]");
                }
                
                pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "\n");
            }
        }
        
        pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "\n");
    }
    
    if (config.positional_args_help) {
        pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "Arguments:\n%s\n\n", config.positional_args_help);
    }
    
    if (config.examples) {
        pos += snprintf(help_text + pos, MAX_HELP_TEXT_SIZE - pos, "Examples:\n%s\n", config.examples);
    }
    
    return help_text;
}

void flag_print_help(void) {
    printf("%s", flag_format_help());
}

void flag_print_version(void) {
    if (config.version) {
        printf("%s version %s\n", config.name ? config.name : program_name, config.version);
    } else {
        printf("%s\n", config.name ? config.name : program_name);
    }
}

const char** flag_get_remaining_args(int* count) {
    *count = remaining_args_count;
    return (const char**)remaining_args;
}

int flag_count_remaining_args(void) {
    return remaining_args_count;
}

bool flag_has_remaining_args(void) {
    return remaining_args_count > 0;
}

const char* flag_get_remaining_arg(int index) {
    if (index >= 0 && index < remaining_args_count) {
        return remaining_args[index];
    }
    return NULL;
}

char* flag_join_remaining_args(const char* separator) {
    if (remaining_args_count == 0) {
        return strdup("");
    }
    
    size_t sep_len = strlen(separator);
    size_t total_len = 0;
    
    for (int i = 0; i < remaining_args_count; i++) {
        total_len += strlen(remaining_args[i]);
    }
    
    total_len += sep_len * (remaining_args_count - 1);
    
    char* result = (char*)malloc(total_len + 1);
    if (!result) {
        return NULL;
    }
    
    strcpy(result, remaining_args[0]);
    size_t pos = strlen(remaining_args[0]);
    
    for (int i = 1; i < remaining_args_count; i++) {
        strcpy(result + pos, separator);
        pos += sep_len;
        strcpy(result + pos, remaining_args[i]);
        pos += strlen(remaining_args[i]);
    }
    
    return result;
}

void flag_print_flags(void) {
    printf("Flags:\n");
    for (int i = 0; i < flag_count; i++) {
        printf("  --%s", flags[i].name);
        
        switch (flags[i].type) {
            case FLAG_BOOL:
                printf(" (bool): %s", *(bool*)flags[i].value ? "true" : "false");
                break;
            case FLAG_STRING:
                printf(" (string): \"%s\"", *(char**)flags[i].value ? *(char**)flags[i].value : "NULL");
                break;
            case FLAG_INT:
                printf(" (int): %d", *(int*)flags[i].value);
                break;
            case FLAG_FLOAT:
                printf(" (float): %.2f", *(float*)flags[i].value);
                break;
            case FLAG_DOUBLE:
                printf(" (double): %.2f", *(double*)flags[i].value);
                break;
            case FLAG_ENUM:
                printf(" (enum): %s", flags[i].extra_data.enum_data.options[*(int*)flags[i].value]);
                break;
            case FLAG_CALLBACK:
                printf(" (callback)");
                break;
        }
        
        printf(" (provided: %s)\n", flags[i].provided ? "yes" : "no");
    }
}

void flag_free(void) {
    flag_count = 0;
    remaining_args_count = 0;
    program_name = NULL;
    program_description = NULL;
    group_count = 0;
    
    for (int i = 0; i < MAX_GROUPS; i++) {
        flags_in_group_count[i] = 0;
        groups[i] = NULL;
    }
}

