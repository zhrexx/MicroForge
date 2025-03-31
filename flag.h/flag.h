#ifndef FLAG_H
#define FLAG_H

#include <stdbool.h>

typedef enum {
    FLAG_BOOL,
    FLAG_STRING,
    FLAG_INT,
    FLAG_FLOAT,
    FLAG_DOUBLE,
    FLAG_ENUM,
    FLAG_CALLBACK
} FlagType;

typedef struct {
    const char* name;
    const char* shortname;
    const char* help;
    FlagType type;
    void* value;
    bool required;
    bool provided;
    bool hidden;
    const char* env_var;
    union {
        bool bool_default;
        const char* string_default;
        int int_default;
        float float_default;
        double double_default;
        int enum_default;
    } default_value;
    union {
        struct {
            const char** options;
            int options_count;
        } enum_data;
        struct {
            bool (*func)(const char*, void*);
            void* user_data;
        } callback_data;
    } extra_data;
} Flag;

typedef struct {
    const char* name;
    const char* description;
    void (*pre_parse_hook)(int, char**);
    void (*post_parse_hook)(int, char**);
    bool allow_unknown_flags;
    bool auto_help;
    bool show_defaults;
    const char* usage_pattern;
    const char* args_description;
    const char* version;
    const char* examples;
    const char* positional_args_help;
} FlagConfig;

void flag_init(const char* program_name, const char* program_description);
void flag_set_config(FlagConfig config);
FlagConfig flag_get_config(void);
void flag_bool(const char* name, const char* shortname, bool* value, bool default_value, const char* help, bool required);
void flag_string(const char* name, const char* shortname, char** value, const char* default_value, const char* help, bool required);
void flag_int(const char* name, const char* shortname, int* value, int default_value, const char* help, bool required);
void flag_float(const char* name, const char* shortname, float* value, float default_value, const char* help, bool required);
void flag_double(const char* name, const char* shortname, double* value, double default_value, const char* help, bool required);
void flag_enum(const char* name, const char* shortname, int* value, int default_value, const char** options, int options_count, const char* help, bool required);
void flag_callback(const char* name, const char* shortname, bool (*callback)(const char* value, void* user_data), void* user_data, const char* help, bool required);
bool flag_parse(int argc, char** argv);
void flag_print_help(void);
void flag_print_version(void);
void flag_print_error(const char* message);
char** flag_args(int* count);
void flag_set_hidden(const char* name, bool hidden);
void flag_set_env_var(const char* name, const char* env_var);
char* flag_get_env(const char* name);
const char* flag_format_help(void);
bool flag_validate_required(void);
void flag_free(void);
Flag* flag_get(const char* name);
bool flag_was_provided(const char* name);
void flag_set_groups(const char** group_names, int group_count);
void flag_add_to_group(const char* flag_name, const char* group_name);

#endif
