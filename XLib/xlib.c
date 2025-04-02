#include "xlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

#define ELF_MAGIC 0x464C457F
#define ET_DYN 3
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_DYNSYM 11
#define STB_GLOBAL 1
#define STT_FUNC 2
#define SHN_UNDEF 0

static char error_buffer[256] = {0};

typedef struct {
    union {
        unsigned char e_ident[16];
        unsigned int e_magic;
    };
    unsigned short e_type;
    unsigned short e_machine;
    unsigned int e_version;
    size_t e_entry;
    size_t e_phoff;
    size_t e_shoff;
    unsigned int e_flags;
    unsigned short e_ehsize;
    unsigned short e_phentsize;
    unsigned short e_phnum;
    unsigned short e_shentsize;
    unsigned short e_shnum;
    unsigned short e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    unsigned int sh_name;
    unsigned int sh_type;
    size_t sh_flags;
    size_t sh_addr;
    size_t sh_offset;
    size_t sh_size;
    unsigned int sh_link;
    unsigned int sh_info;
    size_t sh_addralign;
    size_t sh_entsize;
} Elf64_Shdr;

typedef struct {
    unsigned int st_name;
    unsigned char st_info;
    unsigned char st_other;
    unsigned short st_shndx;
    size_t st_value;
    size_t st_size;
} Elf64_Sym;

#define PE_MAGIC 0x5A4D
#define PE_SIGNATURE 0x00004550

typedef struct {
    unsigned short e_magic;
    unsigned short e_cblp;
    unsigned short e_cp;
    unsigned short e_crlc;
    unsigned short e_cparhdr;
    unsigned short e_minalloc;
    unsigned short e_maxalloc;
    unsigned short e_ss;
    unsigned short e_sp;
    unsigned short e_csum;
    unsigned short e_ip;
    unsigned short e_cs;
    unsigned short e_lfarlc;
    unsigned short e_ovno;
    unsigned short e_res[4];
    unsigned short e_oemid;
    unsigned short e_oeminfo;
    unsigned short e_res2[10];
    unsigned int e_lfanew;
} IMAGE_DOS_HEADER;

typedef struct {
    unsigned short Machine;
    unsigned short NumberOfSections;
    unsigned int TimeDateStamp;
    unsigned int PointerToSymbolTable;
    unsigned int NumberOfSymbols;
    unsigned short SizeOfOptionalHeader;
    unsigned short Characteristics;
} IMAGE_FILE_HEADER;

typedef struct {
    unsigned int Signature;
    IMAGE_FILE_HEADER FileHeader;
} IMAGE_NT_HEADERS;

typedef struct {
    char Name[8];
    union {
        unsigned int PhysicalAddress;
        unsigned int VirtualSize;
    } Misc;
    unsigned int VirtualAddress;
    unsigned int SizeOfRawData;
    unsigned int PointerToRawData;
    unsigned int PointerToRelocations;
    unsigned int PointerToLinenumbers;
    unsigned short NumberOfRelocations;
    unsigned short NumberOfLinenumbers;
    unsigned int Characteristics;
} IMAGE_SECTION_HEADER;

typedef struct {
    union {
        unsigned int Name;
        unsigned char ShortName[8];
    } N;
    unsigned int Value;
    int SectionNumber;
    unsigned short Type;
    unsigned char StorageClass;
    unsigned char NumberOfAuxSymbols;
} IMAGE_SYMBOL;

struct XLib_Handle {
    void* memory;
    size_t memory_size;
    void* code_base;
    void** resolved_symbols;
    char** symbol_names;
    int symbol_count;
    char* string_table;
#ifdef _WIN32
    HMODULE module;
#endif
};

#ifndef _WIN32
#define ELF64_ST_TYPE(i) ((i)&0xf)
#define ELF64_ST_BIND(i) ((i)>>4)
#endif

static void set_error(const char* msg) {
    strncpy(error_buffer, msg, sizeof(error_buffer) - 1);
    error_buffer[sizeof(error_buffer) - 1] = '\0';
}

static void* map_file(const char* path, size_t* size) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        set_error("Failed to open file");
        return NULL;
    }
    
    struct stat sb;
    if (fstat(fd, &sb) < 0) {
        set_error("Failed to stat file");
        close(fd);
        return NULL;
    }
    
    void* mem = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE, fd, 0);
    close(fd);
    
    if (mem == MAP_FAILED) {
        set_error("Failed to map file into memory");
        return NULL;
    }
    
    *size = sb.st_size;
    return mem;
}

static int load_elf_symbols(XLib_Handle* handle) {
    void* memory = handle->memory;
    size_t size = handle->memory_size;
    
    Elf64_Ehdr* header = (Elf64_Ehdr*)memory;
    
    if (size < sizeof(Elf64_Ehdr) || header->e_magic != ELF_MAGIC) {
        set_error("Not a valid ELF file");
        return XLIB_ERROR_FORMAT;
    }
    
    Elf64_Shdr* section_headers = (Elf64_Shdr*)((char*)memory + header->e_shoff);
    
    Elf64_Shdr* shstrtab_header = &section_headers[header->e_shstrndx];
    char* shstrtab = (char*)memory + shstrtab_header->sh_offset;
    
    Elf64_Shdr* symtab = NULL;
    Elf64_Shdr* strtab = NULL;
    
    for (int i = 0; i < header->e_shnum; i++) {
        Elf64_Shdr* sh = &section_headers[i];
        char* name = shstrtab + sh->sh_name;
        
        if (sh->sh_type == SHT_SYMTAB) {
            symtab = sh;
        }
        else if (sh->sh_type == SHT_STRTAB && strcmp(name, ".strtab") == 0) {
            strtab = sh;
        }
    }
    
    if (!symtab || !strtab) {
        set_error("Symbol or string table not found");
        return XLIB_ERROR_FORMAT;
    }
    
    Elf64_Sym* symbols = (Elf64_Sym*)((char*)memory + symtab->sh_offset);
    handle->string_table = (char*)memory + strtab->sh_offset;
    int symbol_count = symtab->sh_size / sizeof(Elf64_Sym);
    
    handle->symbol_count = symbol_count;
    handle->resolved_symbols = (void**)malloc(symbol_count * sizeof(void*));
    handle->symbol_names = (char**)malloc(symbol_count * sizeof(char*));
    
    if (!handle->resolved_symbols || !handle->symbol_names) {
        set_error("Out of memory");
        return XLIB_ERROR_OPEN;
    }
    
    handle->code_base = memory;
    
    for (int i = 0; i < symbol_count; i++) {
        Elf64_Sym* sym = &symbols[i];
        unsigned char type = ELF64_ST_TYPE(sym->st_info);
        unsigned char bind = ELF64_ST_BIND(sym->st_info);
        
        handle->symbol_names[i] = handle->string_table + sym->st_name;
        
        if (sym->st_shndx != SHN_UNDEF && (type == STT_FUNC || bind == STB_GLOBAL)) {
            handle->resolved_symbols[i] = (char*)handle->code_base + sym->st_value;
        } else {
            handle->resolved_symbols[i] = NULL;
        }
    }
    
    return XLIB_OK;
}

#ifdef _WIN32

static int load_pe_symbols(XLib_Handle* handle) {
    void* memory = handle->memory;
    size_t size = handle->memory_size;
    
    IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)memory;
    if (dos_header->e_magic != PE_MAGIC) {
        set_error("Not a valid PE file");
        return XLIB_ERROR_FORMAT;
    }
    
    IMAGE_NT_HEADERS* nt_headers = (IMAGE_NT_HEADERS*)((char*)memory + dos_header->e_lfanew);
    if (nt_headers->Signature != PE_SIGNATURE) {
        set_error("Invalid PE signature");
        return XLIB_ERROR_FORMAT;
    }
    
    IMAGE_SECTION_HEADER* sections = (IMAGE_SECTION_HEADER*)((char*)&nt_headers->FileHeader + 
                                    sizeof(IMAGE_FILE_HEADER) + 
                                    nt_headers->FileHeader.SizeOfOptionalHeader);
    
    for (int i = 0; i < nt_headers->FileHeader.NumberOfSections; i++) {
        if (strncmp(sections[i].Name, ".text", 5) == 0) {
            handle->code_base = (char*)memory + sections[i].PointerToRawData;
            break;
        }
    }
    
    if (!handle->code_base) {
        handle->code_base = memory;
    }
    
    IMAGE_SYMBOL* symbols = (IMAGE_SYMBOL*)((char*)memory + nt_headers->FileHeader.PointerToSymbolTable);
    int symbol_count = nt_headers->FileHeader.NumberOfSymbols;
    
    char* string_table = (char*)(symbols + symbol_count);
    
    handle->symbol_count = symbol_count;
    handle->resolved_symbols = (void**)malloc(symbol_count * sizeof(void*));
    handle->symbol_names = (char**)malloc(symbol_count * sizeof(char*));
    handle->string_table = string_table;
    
    if (!handle->resolved_symbols || !handle->symbol_names) {
        set_error("Out of memory");
        return XLIB_ERROR_OPEN;
    }
    
    for (int i = 0; i < symbol_count; i++) {
        IMAGE_SYMBOL* sym = &symbols[i];
        
        if (sym->N.Name == 0 && sym->N.ShortName[0] == 0) {
            handle->symbol_names[i] = string_table + *((unsigned int*)sym->N.ShortName);
        } else {
            static char short_name[9];
            memcpy(short_name, sym->N.ShortName, 8);
            short_name[8] = '\0';
            handle->symbol_names[i] = _strdup(short_name);
        }
        
        if (sym->SectionNumber > 0 && sym->Type == 0x20) {
            IMAGE_SECTION_HEADER* section = &sections[sym->SectionNumber - 1];
            handle->resolved_symbols[i] = (char*)handle->code_base + (sym->Value - section->VirtualAddress);
        } else {
            handle->resolved_symbols[i] = NULL;
        }
    }
    
    return XLIB_OK;
}

#endif

XLib_Handle* XLib_open(const char* path) {
    XLib_Handle* handle = (XLib_Handle*)malloc(sizeof(XLib_Handle));
    if (!handle) {
        set_error("Out of memory");
        return NULL;
    }
    
    memset(handle, 0, sizeof(XLib_Handle));
    
#ifdef _WIN32
    handle->module = LoadLibraryA(path);
    if (handle->module) {
        return handle;
    }
    
    size_t size;
    handle->memory = map_file(path, &size);
    if (!handle->memory) {
        free(handle);
        return NULL;
    }
    handle->memory_size = size;
    
    IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)handle->memory;
    if (dos_header->e_magic == PE_MAGIC) {
        if (load_pe_symbols(handle) != XLIB_OK) {
            XLib_close(handle);
            return NULL;
        }
    } else {
        set_error("Unsupported file format");
        XLib_close(handle);
        return NULL;
    }
#else
    size_t size;
    handle->memory = map_file(path, &size);
    if (!handle->memory) {
        free(handle);
        return NULL;
    }
    handle->memory_size = size;
    
    Elf64_Ehdr* elf_header = (Elf64_Ehdr*)handle->memory;
    if (elf_header->e_magic == ELF_MAGIC) {
        if (load_elf_symbols(handle) != XLIB_OK) {
            XLib_close(handle);
            return NULL;
        }
    } else {
        set_error("Unsupported file format");
        XLib_close(handle);
        return NULL;
    }
#endif
    
    return handle;
}

void* XLib_sym(XLib_Handle* handle, const char* symbol) {
    if (!handle) {
        set_error("Invalid handle");
        return NULL;
    }
    
#ifdef _WIN32
    if (handle->module) {
        void* addr = (void*)GetProcAddress(handle->module, symbol);
        if (!addr) {
            DWORD error = GetLastError();
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "Failed to find symbol '%s': error code %lu", symbol, error);
            set_error(error_msg);
            return NULL;
        }
        return addr;
    }
#endif
    
    if (handle->memory && handle->resolved_symbols && handle->symbol_names) {
        for (int i = 0; i < handle->symbol_count; i++) {
            if (strcmp(handle->symbol_names[i], symbol) == 0 && handle->resolved_symbols[i]) {
                return handle->resolved_symbols[i];
            }
        }
    }
    
    set_error("Symbol not found");
    return NULL;
}

int XLib_close(XLib_Handle* handle) {
    if (!handle) {
        set_error("Invalid handle");
        return XLIB_ERROR_INVALID;
    }
    
#ifdef _WIN32
    if (handle->module) {
        if (!FreeLibrary(handle->module)) {
            DWORD error = GetLastError();
            char error_msg[256];
            snprintf(error_msg, sizeof(error_msg), "Failed to close library: error code %lu", error);
            set_error(error_msg);
            free(handle);
            return XLIB_ERROR_CLOSE;
        }
    }
#endif
    
    if (handle->memory && handle->memory_size > 0) {
#ifdef _WIN32
        VirtualFree(handle->memory, 0, MEM_RELEASE);
#else
        munmap(handle->memory, handle->memory_size);
#endif
    }
    
    if (handle->resolved_symbols) {
        free(handle->resolved_symbols);
    }
    
    if (handle->symbol_names) {
#ifdef _WIN32
        for (int i = 0; i < handle->symbol_count; i++) {
            if (handle->symbol_names[i] && handle->symbol_names[i] != handle->string_table) {
                free(handle->symbol_names[i]);
            }
        }
#endif
        free(handle->symbol_names);
    }
    
    free(handle);
    return XLIB_OK;
}

const char* XLib_error(void) {
    return error_buffer;
}
