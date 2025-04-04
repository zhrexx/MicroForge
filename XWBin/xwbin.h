#ifndef XWBIN_H
#define XWBIN_H 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef enum {
    XWB_I32 = 0x01,
    XWB_I64 = 0x02,
    XWB_F32 = 0x03,
    XWB_F64 = 0x04,
    XWB_STRING = 0x05,
} XWBValueType;

typedef enum {
    XWB_IMPORT = 0x00,
    XWB_FUNCTION = 0x01,
    XWB_TABLE = 0x02,
    XWB_MEMORY = 0x03,
    XWB_GLOBAL = 0x04,
    XWB_EXPORT = 0x05,
    XWB_START = 0x06,
    XWB_ELEMENT = 0x07,
    XWB_CODE = 0x08,
    XWB_DATA = 0x09
} XWBSectionType;

typedef enum {
    XWB_OP_UNREACHABLE = 0x00,
    XWB_OP_NOP = 0x01,
    XWB_OP_BLOCK = 0x02,
    XWB_OP_LOOP = 0x03,
    XWB_OP_IF = 0x04,
    XWB_OP_ELSE = 0x05,
    XWB_OP_END = 0x0B,
    XWB_OP_BR = 0x0C,
    XWB_OP_BR_IF = 0x0D,
    XWB_OP_BR_TABLE = 0x0E,
    XWB_OP_RETURN = 0x0F,
    XWB_OP_CALL = 0x10,
    XWB_OP_CALL_INDIRECT = 0x11,
    XWB_OP_DROP = 0x1A,
    XWB_OP_SELECT = 0x1B,
    XWB_OP_LOCAL_GET = 0x20,
    XWB_OP_LOCAL_SET = 0x21,
    XWB_OP_LOCAL_TEE = 0x22,
    XWB_OP_GLOBAL_GET = 0x23,
    XWB_OP_GLOBAL_SET = 0x24,
    XWB_OP_I32_LOAD = 0x28,
    XWB_OP_I64_LOAD = 0x29,
    XWB_OP_F32_LOAD = 0x2A,
    XWB_OP_F64_LOAD = 0x2B,
    XWB_OP_I32_STORE = 0x36,
    XWB_OP_I64_STORE = 0x37,
    XWB_OP_F32_STORE = 0x38,
    XWB_OP_F64_STORE = 0x39,
    XWB_OP_MEMORY_SIZE = 0x3F,
    XWB_OP_MEMORY_GROW = 0x40,
    XWB_OP_I32_CONST = 0x41,
    XWB_OP_I64_CONST = 0x42,
    XWB_OP_F32_CONST = 0x43,
    XWB_OP_F64_CONST = 0x44,
    XWB_OP_I32_EQZ = 0x45,
    XWB_OP_I32_ADD = 0x6A,
    XWB_OP_I32_SUB = 0x6B,
    XWB_OP_I32_MUL = 0x6C,
    XWB_OP_I32_DIV_S = 0x6D,
    XWB_OP_STRING_CONST = 0x75,
    XWB_OP_STRING_CONCAT = 0x76,
    XWB_OP_STRING_EQ = 0x77,
    XWB_OP_STRING_LENGTH = 0x78,
    XWB_OP_STRING_STORE = 0x79,
    XWB_OP_STRING_LOAD = 0x7A
} XWBOpcode;

typedef struct {
    uint8_t type;
    union {
        int32_t i32;
        int64_t i64;
        float f32;
        double f64;
        struct {
            char *data;
            uint32_t length;
        } str;
    } value;
} XWBValue;

typedef struct {
    XWBOpcode op;
    XWBValue immediate;
} XWBInstruction;

typedef struct {
    uint32_t num_params;
    XWBValueType* param_types;
    uint32_t num_results;
    XWBValueType* result_types;
} XWBFunctionType;

typedef struct {
    uint32_t type_idx;
    uint32_t num_locals;
    XWBValueType* local_types;
    uint32_t num_instructions;
    XWBInstruction* code;
} XWBFunction;

typedef struct {
    uint32_t initial_size;
    uint32_t maximum_size;
    uint8_t* data;
} XWBMemory;

typedef struct {
    char* name;
    uint32_t idx;
    uint8_t kind;
} XWBExport;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t num_types;
    XWBFunctionType* types;
    uint32_t num_imports;
    uint32_t num_functions;
    XWBFunction* functions;
    uint32_t num_tables;
    uint32_t num_memories;
    XWBMemory* memories;
    uint32_t num_globals;
    uint32_t num_exports;
    XWBExport* exports;
    uint32_t start_func_idx;
    uint32_t num_elements;
    uint32_t num_data_segments;
} XWBModule;

typedef struct {
    XWBValue* stack;
    uint32_t stack_size;
    uint32_t stack_capacity;
    XWBValue* locals;
    uint32_t num_locals;
    XWBMemory* memory;
    XWBModule* module;
    uint32_t pc;
    XWBFunction* current_function;
} XWBExecutionContext;

uint32_t read_leb128(uint8_t* bytes, uint32_t* pos) {
    uint32_t result = 0;
    uint32_t shift = 0;
    uint8_t byte;
    
    do {
        byte = bytes[(*pos)++];
        result |= (byte & 0x7F) << shift;
        shift += 7;
    } while (byte & 0x80);
    
    return result;
}

void write_leb128(uint8_t* bytes, uint32_t* pos, uint32_t value) {
    do {
        uint8_t byte = value & 0x7F;
        value >>= 7;
        if (value != 0) {
            byte |= 0x80;
        }
        bytes[(*pos)++] = byte;
    } while (value != 0);
}

XWBModule* xwb_parse_module(uint8_t* bytes, size_t len) {
    uint32_t pos = 0;
    XWBModule* module = (XWBModule*)malloc(sizeof(XWBModule));
    memset(module, 0, sizeof(XWBModule));
    
    module->magic = *(uint32_t*)(bytes + pos);
    pos += 4;
    
    module->version = *(uint32_t*)(bytes + pos);
    pos += 4;
    
    while (pos < len) {
        uint8_t section_id = bytes[pos++];
        uint32_t section_size = read_leb128(bytes, &pos);
        uint32_t section_start = pos;
        
        switch (section_id) {
            case XWB_FUNCTION: {
                module->num_functions = read_leb128(bytes, &pos);
                module->functions = (XWBFunction*)malloc(module->num_functions * sizeof(XWBFunction));
                
                for (uint32_t i = 0; i < module->num_functions; i++) {
                    module->functions[i].type_idx = read_leb128(bytes, &pos);
                }
                break;
            }
            case XWB_CODE: {
                uint32_t count = read_leb128(bytes, &pos);
                
                for (uint32_t i = 0; i < count; i++) {
                    uint32_t func_size = read_leb128(bytes, &pos);
                    uint32_t func_pos = pos;
                    
                    module->functions[i].num_locals = read_leb128(bytes, &pos);
                    module->functions[i].local_types = (XWBValueType*)malloc(module->functions[i].num_locals * sizeof(XWBValueType));
                    
                    for (uint32_t j = 0; j < module->functions[i].num_locals; j++) {
                        module->functions[i].local_types[j] = bytes[pos++];
                    }
                    
                    uint32_t code_size = func_size - (pos - func_pos);
                    module->functions[i].num_instructions = code_size;
                    module->functions[i].code = (XWBInstruction*)malloc(code_size * sizeof(XWBInstruction));
                    
                    uint32_t inst_idx = 0;
                    while (pos < func_pos + func_size) {
                        XWBOpcode op = bytes[pos++];
                        module->functions[i].code[inst_idx].op = op;
                        
                        switch (op) {
                            case XWB_OP_I32_CONST:
                                module->functions[i].code[inst_idx].immediate.type = XWB_I32;
                                module->functions[i].code[inst_idx].immediate.value.i32 = read_leb128(bytes, &pos);
                                break;
                            case XWB_OP_I64_CONST:
                                module->functions[i].code[inst_idx].immediate.type = XWB_I64;
                                module->functions[i].code[inst_idx].immediate.value.i64 = read_leb128(bytes, &pos);
                                break;

                            case XWB_OP_STRING_CONST: {
                                module->functions[i].code[inst_idx].immediate.type = XWB_STRING;
                                uint32_t str_len = read_leb128(bytes, &pos);
                                char* str_data = (char*)malloc(str_len + 1);
                                memcpy(str_data, bytes + pos, str_len);
                                str_data[str_len] = '\0';
                                pos += str_len;
                                module->functions[i].code[inst_idx].immediate.value.str.data = str_data;
                                module->functions[i].code[inst_idx].immediate.value.str.length = str_len;
                                break;
                            }
                            case XWB_OP_LOCAL_GET:
                            case XWB_OP_LOCAL_SET:
                            case XWB_OP_LOCAL_TEE:
                                module->functions[i].code[inst_idx].immediate.type = XWB_I32;
                                module->functions[i].code[inst_idx].immediate.value.i32 = read_leb128(bytes, &pos);
                                break;
                            case XWB_OP_CALL:
                                module->functions[i].code[inst_idx].immediate.type = XWB_I32;
                                module->functions[i].code[inst_idx].immediate.value.i32 = read_leb128(bytes, &pos);
                                break;
                        }
                        
                        inst_idx++;
                    }
                }
                break;
            }
            case XWB_MEMORY: {
                uint32_t count = read_leb128(bytes, &pos);
                module->num_memories = count;
                module->memories = (XWBMemory*)malloc(count * sizeof(XWBMemory));
                
                for (uint32_t i = 0; i < count; i++) {
                    uint8_t flags = bytes[pos++];
                    module->memories[i].initial_size = read_leb128(bytes, &pos);
                    
                    if (flags & 0x01) {
                        module->memories[i].maximum_size = read_leb128(bytes, &pos);
                    } else {
                        module->memories[i].maximum_size = 65536;
                    }
                    
                    module->memories[i].data = (uint8_t*)calloc(module->memories[i].initial_size, 65536);
                }
                break;
            }
            case XWB_EXPORT: {
                module->num_exports = read_leb128(bytes, &pos);
                module->exports = (XWBExport*)malloc(module->num_exports * sizeof(XWBExport));
                
                for (uint32_t i = 0; i < module->num_exports; i++) {
                    uint32_t name_len = read_leb128(bytes, &pos);
                    module->exports[i].name = (char*)malloc(name_len + 1);
                    memcpy(module->exports[i].name, bytes + pos, name_len);
                    module->exports[i].name[name_len] = '\0';
                    pos += name_len;
                    
                    module->exports[i].kind = bytes[pos++];
                    module->exports[i].idx = read_leb128(bytes, &pos);
                }
                break;
            }
            case XWB_START: {
                module->start_func_idx = read_leb128(bytes, &pos);
                break;
            }
            default:
                pos = section_start + section_size;
                break;
        }
    }
    
    return module;
}

XWBExecutionContext* xwb_create_context(XWBModule* module) {
    XWBExecutionContext* ctx = (XWBExecutionContext*)malloc(sizeof(XWBExecutionContext));
    ctx->stack_capacity = 1024;
    ctx->stack = (XWBValue*)malloc(ctx->stack_capacity * sizeof(XWBValue));
    ctx->stack_size = 0;
    ctx->module = module;
    ctx->memory = module->memories;
    ctx->pc = 0;
    
    return ctx;
}

void xwb_push_value(XWBExecutionContext* ctx, XWBValue value) {
    if (ctx->stack_size >= ctx->stack_capacity) {
        ctx->stack_capacity *= 2;
        ctx->stack = (XWBValue*)realloc(ctx->stack, ctx->stack_capacity * sizeof(XWBValue));
    }
    ctx->stack[ctx->stack_size++] = value;
}

XWBValue xwb_pop_value(XWBExecutionContext* ctx) {
    return ctx->stack[--ctx->stack_size];
}

void xwb_execute_function(XWBExecutionContext* ctx, uint32_t func_idx) {
    XWBFunction* func = &ctx->module->functions[func_idx];
    ctx->current_function = func;
    
    uint32_t num_locals = func->num_locals;
    ctx->locals = (XWBValue*)malloc(num_locals * sizeof(XWBValue));
    ctx->num_locals = num_locals;
    
    for (uint32_t i = 0; i < num_locals; i++) {
        ctx->locals[i].type = func->local_types[i];
        memset(&ctx->locals[i].value, 0, sizeof(ctx->locals[i].value));
    }
    
    ctx->pc = 0;
    while (ctx->pc < func->num_instructions) {
        XWBInstruction inst = func->code[ctx->pc++];
        
        switch (inst.op) {
            case XWB_OP_I32_CONST: {
                XWBValue val;
                val.type = XWB_I32;
                val.value.i32 = inst.immediate.value.i32;
                xwb_push_value(ctx, val);
                break;
            }
            case XWB_OP_I64_CONST: {
                XWBValue val;
                val.type = XWB_I64;
                val.value.i64 = inst.immediate.value.i64;
                xwb_push_value(ctx, val);
                break;
            }
            case XWB_OP_LOCAL_GET: {
                uint32_t local_idx = inst.immediate.value.i32;
                xwb_push_value(ctx, ctx->locals[local_idx]);
                break;
            }
            case XWB_OP_LOCAL_SET: {
                uint32_t local_idx = inst.immediate.value.i32;
                ctx->locals[local_idx] = xwb_pop_value(ctx);
                break;
            }
            case XWB_OP_LOCAL_TEE: {
                uint32_t local_idx = inst.immediate.value.i32;
                XWBValue val = ctx->stack[ctx->stack_size - 1];
                ctx->locals[local_idx] = val;
                break;
            }
            case XWB_OP_I32_ADD: {
                XWBValue b = xwb_pop_value(ctx);
                XWBValue a = xwb_pop_value(ctx);
                XWBValue result;
                result.type = XWB_I32;
                result.value.i32 = a.value.i32 + b.value.i32;
                xwb_push_value(ctx, result);
                break;
            }
            case XWB_OP_I32_SUB: {
                XWBValue b = xwb_pop_value(ctx);
                XWBValue a = xwb_pop_value(ctx);
                XWBValue result;
                result.type = XWB_I32;
                result.value.i32 = a.value.i32 - b.value.i32;
                xwb_push_value(ctx, result);
                break;
            }
            case XWB_OP_I32_MUL: {
                XWBValue b = xwb_pop_value(ctx);
                XWBValue a = xwb_pop_value(ctx);
                XWBValue result;
                result.type = XWB_I32;
                result.value.i32 = a.value.i32 * b.value.i32;
                xwb_push_value(ctx, result);
                break;
            }
            case XWB_OP_I32_DIV_S: {
                XWBValue b = xwb_pop_value(ctx);
                XWBValue a = xwb_pop_value(ctx);
                XWBValue result;
                result.type = XWB_I32;
                result.value.i32 = a.value.i32 / b.value.i32;
                xwb_push_value(ctx, result);
                break;
            }
            case XWB_OP_I32_LOAD: {
                XWBValue offset = xwb_pop_value(ctx);
                uint32_t addr = offset.value.i32;
                XWBValue result;
                result.type = XWB_I32;
                result.value.i32 = *(int32_t*)(ctx->memory->data + addr);
                xwb_push_value(ctx, result);
                break;
            }
            case XWB_OP_I32_STORE: {
                XWBValue val = xwb_pop_value(ctx);
                XWBValue offset = xwb_pop_value(ctx);
                uint32_t addr = offset.value.i32;
                *(int32_t*)(ctx->memory->data + addr) = val.value.i32;
                break;
            }
            case XWB_OP_CALL: {
                uint32_t func_idx = inst.immediate.value.i32;
                XWBExecutionContext call_ctx;
                call_ctx.module = ctx->module;
                call_ctx.memory = ctx->memory;
                xwb_execute_function(&call_ctx, func_idx);
                break;
            }
            case XWB_OP_RETURN:
                goto end_function;
            case XWB_OP_STRING_CONST: {
                XWBValue val;
                val.type = XWB_STRING;
                val.value.str.data = strdup(inst.immediate.value.str.data);
                val.value.str.length = inst.immediate.value.str.length;
                xwb_push_value(ctx, val);
                break;
            }
            case XWB_OP_STRING_CONCAT: {
                XWBValue b = xwb_pop_value(ctx);
                XWBValue a = xwb_pop_value(ctx);
                XWBValue result;
                result.type = XWB_STRING;
                result.value.str.length = a.value.str.length + b.value.str.length;
                result.value.str.data = (char*)malloc(result.value.str.length + 1);
                memcpy(result.value.str.data, a.value.str.data, a.value.str.length);
                memcpy(result.value.str.data + a.value.str.length, b.value.str.data, b.value.str.length);
                result.value.str.data[result.value.str.length] = '\0';
                free(a.value.str.data);
                free(b.value.str.data);
                xwb_push_value(ctx, result);
                break;
            }
            case XWB_OP_STRING_EQ: {
                XWBValue b = xwb_pop_value(ctx);
                XWBValue a = xwb_pop_value(ctx);
                XWBValue result;
                result.type = XWB_I32;
    
                if (a.value.str.length == b.value.str.length && 
                    memcmp(a.value.str.data, b.value.str.data, a.value.str.length) == 0) {
                    result.value.i32 = 1;
                } else {
                    result.value.i32 = 0;
                }
    
                free(a.value.str.data);
                free(b.value.str.data);
    
                xwb_push_value(ctx, result);
                break;
            }

            case XWB_OP_STRING_LENGTH: {
                XWBValue str = xwb_pop_value(ctx);
                XWBValue result;
                result.type = XWB_I32;
                result.value.i32 = str.value.str.length;
    
                free(str.value.str.data);
    
                xwb_push_value(ctx, result);
                break;
            }

        case XWB_OP_STRING_STORE: {
            XWBValue str = xwb_pop_value(ctx);
            XWBValue offset = xwb_pop_value(ctx);
            uint32_t addr = offset.value.i32;
    
            *(uint32_t*)(ctx->memory->data + addr) = str.value.str.length;
    
            memcpy(ctx->memory->data + addr + sizeof(uint32_t), str.value.str.data, str.value.str.length);
    
            free(str.value.str.data);
            break;
        }

        case XWB_OP_STRING_LOAD: {
            XWBValue offset = xwb_pop_value(ctx);
            uint32_t addr = offset.value.i32;
    
            uint32_t str_len = *(uint32_t*)(ctx->memory->data + addr);
    
            XWBValue result;
            result.type = XWB_STRING;
            result.value.str.length = str_len;
            result.value.str.data = (char*)malloc(str_len + 1);
    
            memcpy(result.value.str.data, ctx->memory->data + addr + sizeof(uint32_t), str_len);
            result.value.str.data[str_len] = '\0';
    
            xwb_push_value(ctx, result);
            break;
        }

        }
    }
    
end_function:
    free(ctx->locals);
}

uint8_t* xwb_serialize_module(XWBModule* module, size_t* out_size) {
    size_t estimated_size = 1024 * 1024;
    uint8_t* bytes = (uint8_t*)malloc(estimated_size);
    uint32_t pos = 0;
    
    *(uint32_t*)(bytes + pos) = module->magic;
    pos += 4;
    
    *(uint32_t*)(bytes + pos) = module->version;
    pos += 4;
    
    if (module->num_functions > 0) {
        bytes[pos++] = XWB_FUNCTION;
        uint32_t section_size_pos = pos;
        pos += 4;
        uint32_t section_start = pos;
        
        write_leb128(bytes, &pos, module->num_functions);
        
        for (uint32_t i = 0; i < module->num_functions; i++) {
            write_leb128(bytes, &pos, module->functions[i].type_idx);
        }
        
        uint32_t section_size = pos - section_start;
        *(uint32_t*)(bytes + section_size_pos) = section_size;
    }
    
    if (module->num_memories > 0) {
        bytes[pos++] = XWB_MEMORY;
        uint32_t section_size_pos = pos;
        pos += 4;
        uint32_t section_start = pos;
        
        write_leb128(bytes, &pos, module->num_memories);
        
        for (uint32_t i = 0; i < module->num_memories; i++) {
            uint8_t flags = 0;
            if (module->memories[i].maximum_size < 65536) {
                flags |= 0x01;
            }
            bytes[pos++] = flags;
            
            write_leb128(bytes, &pos, module->memories[i].initial_size);
            
            if (flags & 0x01) {
                write_leb128(bytes, &pos, module->memories[i].maximum_size);
            }
        }
        
        uint32_t section_size = pos - section_start;
        *(uint32_t*)(bytes + section_size_pos) = section_size;
    }
    
    if (module->num_exports > 0) {
        bytes[pos++] = XWB_EXPORT;
        uint32_t section_size_pos = pos;
        pos += 4;
        uint32_t section_start = pos;
        
        write_leb128(bytes, &pos, module->num_exports);
        
        for (uint32_t i = 0; i < module->num_exports; i++) {
            uint32_t name_len = strlen(module->exports[i].name);
            write_leb128(bytes, &pos, name_len);
            memcpy(bytes + pos, module->exports[i].name, name_len);
            pos += name_len;
            
            bytes[pos++] = module->exports[i].kind;
            write_leb128(bytes, &pos, module->exports[i].idx);
        }
        
        uint32_t section_size = pos - section_start;
        *(uint32_t*)(bytes + section_size_pos) = section_size;
    }
    
    if (module->start_func_idx != 0) {
        bytes[pos++] = XWB_START;
        uint32_t section_size_pos = pos;
        pos += 4;
        uint32_t section_start = pos;
        
        write_leb128(bytes, &pos, module->start_func_idx);
        
        uint32_t section_size = pos - section_start;
        *(uint32_t*)(bytes + section_size_pos) = section_size;
    }
    
    if (module->num_functions > 0) {
        bytes[pos++] = XWB_CODE;
        uint32_t section_size_pos = pos;
        pos += 4;
        uint32_t section_start = pos;
        
        write_leb128(bytes, &pos, module->num_functions);
        
        for (uint32_t i = 0; i < module->num_functions; i++) {
            uint32_t func_size_pos = pos;
            pos += 4;
            uint32_t func_start = pos;
            
            write_leb128(bytes, &pos, module->functions[i].num_locals);
            
            for (uint32_t j = 0; j < module->functions[i].num_locals; j++) {
                bytes[pos++] = module->functions[i].local_types[j];
            }
            
            for (uint32_t j = 0; j < module->functions[i].num_instructions; j++) {
                XWBInstruction inst = module->functions[i].code[j];
                bytes[pos++] = inst.op;
                
                switch (inst.op) {
                    case XWB_OP_I32_CONST:
                        write_leb128(bytes, &pos, inst.immediate.value.i32);
                        break;
                    case XWB_OP_I64_CONST:
                        write_leb128(bytes, &pos, inst.immediate.value.i64);
                        break;
                    case XWB_OP_LOCAL_GET:
                    case XWB_OP_LOCAL_SET:
                    case XWB_OP_LOCAL_TEE:
                        write_leb128(bytes, &pos, inst.immediate.value.i32);
                        break;
                    case XWB_OP_CALL:
                        write_leb128(bytes, &pos, inst.immediate.value.i32);
                        break;
                    case XWB_OP_STRING_CONST: {
                        write_leb128(bytes, &pos, inst.immediate.value.str.length);
                        memcpy(bytes + pos, inst.immediate.value.str.data, inst.immediate.value.str.length);
                        pos += inst.immediate.value.str.length;
                        break;
                    }
                }
            }
            
            uint32_t func_size = pos - func_start;
            *(uint32_t*)(bytes + func_size_pos) = func_size;
        }
        
        uint32_t section_size = pos - section_start;
        *(uint32_t*)(bytes + section_size_pos) = section_size;
    }
    
    *out_size = pos;
    return bytes;
}

void xwb_free_module(XWBModule* module) {
    for (uint32_t i = 0; i < module->num_functions; i++) {
        for (uint32_t j = 0; j < module->functions[i].num_instructions; j++) {
            XWBInstruction* inst = &module->functions[i].code[j];
            if (inst->op == XWB_OP_STRING_CONST && inst->immediate.type == XWB_STRING) {
                free(inst->immediate.value.str.data);
            }
        }

        free(module->functions[i].local_types);
        free(module->functions[i].code);
    }
    
    free(module->functions);
    
    for (uint32_t i = 0; i < module->num_memories; i++) {
        free(module->memories[i].data);
    }
    
    free(module->memories);
    
    for (uint32_t i = 0; i < module->num_exports; i++) {
        free(module->exports[i].name);
    }
    
    free(module->exports);
    
    free(module);
}

void xwb_free_context(XWBExecutionContext* ctx) {
    free(ctx->stack);
    free(ctx);
}

XWBValue xwb_call_function_by_name(XWBModule* module, const char* name, XWBValue* args, uint32_t num_args) {
    XWBExecutionContext* ctx = xwb_create_context(module);
    uint32_t func_idx = 0;
    
    for (uint32_t i = 0; i < module->num_exports; i++) {
        if (module->exports[i].kind == XWB_FUNCTION && strcmp(module->exports[i].name, name) == 0) {
            func_idx = module->exports[i].idx;
            break;
        }
    }
    
    for (uint32_t i = 0; i < num_args; i++) {
        xwb_push_value(ctx, args[i]);
    }
    
    xwb_execute_function(ctx, func_idx);
    
    XWBValue result = ctx->stack_size > 0 ? xwb_pop_value(ctx) : (XWBValue){.type = XWB_I32, .value = {.i32 = 0}};
    
    xwb_free_context(ctx);
    
    return result;
}


#endif
