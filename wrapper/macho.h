//
// Created by Park Yu on 2024/10/10.
//

#ifndef PCC_MACHO_H
#define PCC_MACHO_H

#include <stdint.h>

#define MH_MAGIC_64 0xfeedfacf /* the 64-bit mach magic number */

#define VM_PROT_NONE    ((int) 0x00)
#define VM_PROT_READ    ((int) 0x01)      /* read permission */
#define VM_PROT_WRITE   ((int) 0x02)      /* write permission */
#define VM_PROT_EXECUTE ((int) 0x04)      /* execute permission */


#define    S_REGULAR        0x0    /* regular section */
#define S_ATTR_PURE_INSTRUCTIONS 0x80000000    // section contains only true machine instructions
#define S_ATTR_SOME_INSTRUCTIONS 0x00000400    // section contains some machine instructions
/*
 * The 64-bit mach header appears at the very beginning of object files for
 * 64-bit architectures.
 */
struct mach_header_64 {
    uint32_t magic;        /* mach magic number identifier */
    int32_t cputype;    /* cpu specifier */
    int32_t cpusubtype;    /* machine specifier */
    uint32_t filetype;    /* type of file */
    uint32_t ncmds;        /* number of load commands */
    uint32_t sizeofcmds;    /* the size of all the load commands */
    uint32_t flags;        /* flags */
    uint32_t reserved;    /* reserved */
};

typedef enum {
    CPU_TYPE_ANY = -1,       /* Wildcard */
    CPU_TYPE_X86 = 7,        /* Intel x86 */
    CPU_TYPE_X86_64 = (CPU_TYPE_X86 | 0x1000000), /* Intel x86-64 */
    CPU_TYPE_ARM = 12,       /* ARM */
    CPU_TYPE_ARM64 = (CPU_TYPE_ARM | 0x1000000), /* ARM64 */
    CPU_TYPE_ARM64_32 = (CPU_TYPE_ARM | 0x2000000), /* ARM64_32 (watchOS) */
    CPU_TYPE_POWERPC = 18,       /* PowerPC */
    CPU_TYPE_POWERPC64 = (CPU_TYPE_POWERPC | 0x1000000), /* PowerPC 64 */
    CPU_TYPE_SPARC = 14,       /* SPARC (deprecated) */
    CPU_TYPE_M68K = 6,        /* Motorola 68k */
    CPU_TYPE_M88K = 13,       /* Motorola 88k */
    CPU_TYPE_I860 = 15,       /* Intel 860 */
    CPU_TYPE_VEO = 255,      /* Vector Engine Option (obsolete) */
    CPU_TYPE_HPPA = 16,       /* HP PA-RISC */
    CPU_TYPE_S390 = 40,       /* IBM S/390 */
    CPU_TYPE_I386 = CPU_TYPE_X86, /* Alias for Intel x86 */
    CPU_TYPE_I486 = 8,        /* Intel i486 (obsolete) */
    CPU_TYPE_I586 = 10,       /* Intel i586 (Pentium, obsolete) */
    CPU_TYPE_I686 = 11,       /* Intel i686 (obsolete) */
} CpuType;

typedef enum {
    // Generic subtype wildcard
    CPU_SUBTYPE_ANY = -1,      /* Wildcard */
    // x86 and x86_64
    CPU_SUBTYPE_I386_ALL = 3,       /* All x86 models */

    // ARM64
    CPU_SUBTYPE_ARM64_ALL = 0,       /* All ARM64 architectures */
} CpuSubType;

typedef enum {
    MH_OBJECT = 0x1,  /* 目标文件（Relocatable object file） */
    MH_EXECUTE = 0x2,  /* 可执行文件（Demand paged executable file） */
    MH_FVMLIB = 0x3,  /* 固定虚拟内存共享库（Fixed VM shared library file） */
    MH_CORE = 0x4,  /* 核心转储文件（Core file） */
    MH_PRELOAD = 0x5,  /* 预加载文件（Preloaded executable file） */
    MH_DYLIB = 0x6,  /* 动态库文件（Dynamically bound shared library） */
    MH_DYLINKER = 0x7,  /* 动态链接器（Dynamic link editor） */
    MH_BUNDLE = 0x8,  /* 动态加载的代码包（Dynamically bound bundle file） */
    MH_DYLIB_STUB = 0x9,  /* 动态库存根文件（Shared library stub for static linking） */
    MH_DSYM = 0xa,  /* 符号信息文件（Companion file with only debug symbols） */
    MH_KEXT_BUNDLE = 0xb   /* 内核扩展包（Kernel extension bundle） */
} MachOFileType;


typedef enum {
    MH_NOUNDEFS = 0x1,        /* 没有未定义符号（No undefined symbols allowed） */
    MH_INCRLINK = 0x2,        /* 增量链接（Incremental linkable） */
    MH_DYLDLINK = 0x4,        /* 支持动态链接器（Input for the dynamic linker） */
    MH_BINDATLOAD = 0x8,        /* 运行时绑定（Bind at load time） */
    MH_PREBOUND = 0x10,       /* 预绑定（Prebound） */
    MH_SPLIT_SEGS = 0x20,       /* 分割段（Split segments） */
    MH_LAZY_INIT = 0x40,       /* 延迟初始化（Lazy initialization） */
    MH_TWOLEVEL = 0x80,       /* 两级命名空间（Two-level namespace bindings） */
    MH_FORCE_FLAT = 0x100,      /* 强制扁平命名空间（Force flat namespace） */
    MH_NOMULTIDEFS = 0x200,      /* 不允许多重定义（No multiple definitions） */
    MH_NOFIXPREBINDING = 0x400,      /* 禁止修复预绑定（Do not fix prebinding） */
    MH_PREBINDABLE = 0x800,      /* 文件可预绑定（File can be prebound） */
    MH_ALLMODSBOUND = 0x1000,     /* 所有模块已绑定（All modules are bound） */
    MH_SUBSECTIONS_VIA_SYMBOLS = 0x2000, /* 子段通过符号分割（Sections via symbols for dead code stripping） */
    MH_CANONICAL = 0x4000,     /* 规范的字节顺序（Canonicalized file, used by the dynamic linker） */
    MH_WEAK_DEFINES = 0x8000,     /* 支持弱定义符号（Contains weak definitions） */
    MH_BINDS_TO_WEAK = 0x10000,    /* 支持绑定到弱符号（Binds to weak symbols） */
    MH_ALLOW_STACK_EXECUTION = 0x20000, /* 允许堆栈执行（Allow stack execution） */
    MH_ROOT_SAFE = 0x40000,    /* 安全运行（Safe for use in root environment） */
    MH_SETUID_SAFE = 0x80000,    /* 安全用于 setuid 程序（Safe for use in setuid programs） */
    MH_NO_REEXPORTED_DYLIBS = 0x100000, /* 动态库不会被重新导出（No re-exported dynamic libraries） */
    MH_PIE = 0x200000,   /* 支持位置无关代码（Position-independent executable） */
    MH_DEAD_STRIPPABLE_DYLIB = 0x400000, /* 支持死代码剥离的动态库（Dead-strippable dylib） */
    MH_HAS_TLV_DESCRIPTORS = 0x800000,  /* 包含线程本地变量描述符（Contains thread-local variable descriptors） */
    MH_NO_HEAP_EXECUTION = 0x1000000,  /* 禁止堆执行（Heap execution is not allowed） */
    MH_APP_EXTENSION_SAFE = 0x2000000   /* 安全用于应用扩展（Safe for use in app extensions） */
} MachOFlags;

struct load_command {
    uint32_t cmd;        /* type of load command */
    uint32_t cmdsize;    /* total size of command in bytes */
};

struct entry_point_command {
    uint32_t cmd;    /* LC_MAIN only used in MH_EXECUTE filetypes */
    uint32_t cmdsize;    /* 24 */
    uint64_t entryoff;    /* file (__TEXT) offset of main() */
    uint64_t stacksize;/* if not zero, initial stack size */
};

// 定义 LC_REQ_DYLD 常量
#define LC_REQ_DYLD 0x80000000

// 枚举所有的 Load Command 类型
typedef enum {
    LC_SEGMENT = 0x1,                 /* segment of this file to be mapped */
    LC_SYMTAB = 0x2,                  /* link-edit stab symbol table info */
    LC_SYMSEG = 0x3,                  /* link-edit gdb symbol table info (obsolete) */
    LC_THREAD = 0x4,                  /* thread */
    LC_UNIXTHREAD = 0x5,              /* unix thread (includes a stack) */
    LC_LOADFVMLIB = 0x6,              /* load a specified fixed VM shared library */
    LC_IDFVMLIB = 0x7,                /* fixed VM shared library identification */
    LC_IDENT = 0x8,                   /* object identification info (obsolete) */
    LC_FVMFILE = 0x9,                 /* fixed VM file inclusion (internal use) */
    LC_PREPAGE = 0xa,                 /* prepage command (internal use) */
    LC_DYSYMTAB = 0xb,                /* dynamic link-edit symbol table info */
    LC_LOAD_DYLIB = 0xc,              /* load a dynamically linked shared library */
    LC_ID_DYLIB = 0xd,                /* dynamically linked shared lib ident */
    LC_LOAD_DYLINKER = 0xe,           /* load a dynamic linker */
    LC_ID_DYLINKER = 0xf,             /* dynamic linker identification */
    LC_PREBOUND_DYLIB = 0x10,         /* modules prebound for a dynamically linked shared library */
    LC_ROUTINES = 0x11,               /* image routines */
    LC_SUB_FRAMEWORK = 0x12,          /* sub framework */
    LC_SUB_UMBRELLA = 0x13,           /* sub umbrella */
    LC_SUB_CLIENT = 0x14,             /* sub client */
    LC_SUB_LIBRARY = 0x15,            /* sub library */
    LC_TWOLEVEL_HINTS = 0x16,         /* two-level namespace lookup hints */
    LC_PREBIND_CKSUM = 0x17,          /* prebind checksum */
    LC_LOAD_WEAK_DYLIB = (0x18 | LC_REQ_DYLD), /* load weak dylib (all symbols weak imported) */
    LC_SEGMENT_64 = 0x19,             /* 64-bit segment of this file to be mapped */
    LC_ROUTINES_64 = 0x1a,            /* 64-bit image routines */
    LC_UUID = 0x1b,                   /* the uuid */
    LC_RPATH = (0x1c | LC_REQ_DYLD),  /* runpath additions */
    LC_CODE_SIGNATURE = 0x1d,         /* location of code signature */
    LC_SEGMENT_SPLIT_INFO = 0x1e,     /* location of segment split info */
    LC_REEXPORT_DYLIB = (0x1f | LC_REQ_DYLD), /* load and re-export dylib */
    LC_LAZY_LOAD_DYLIB = 0x20,        /* delay load of dylib until first use */
    LC_ENCRYPTION_INFO = 0x21,        /* encrypted segment information */
    LC_DYLD_INFO = 0x22,              /* compressed dyld information */
    LC_DYLD_INFO_ONLY = (0x22 | LC_REQ_DYLD), /* compressed dyld information only */
    LC_LOAD_UPWARD_DYLIB = (0x23 | LC_REQ_DYLD), /* load upward dylib */
    LC_VERSION_MIN_MACOSX = 0x24,     /* build for MacOSX min OS version */
    LC_VERSION_MIN_IPHONEOS = 0x25,   /* build for iPhoneOS min OS version */
    LC_FUNCTION_STARTS = 0x26,        /* compressed table of function start addresses */
    LC_DYLD_ENVIRONMENT = 0x27,       /* string for dyld to treat like environment variable */
    LC_MAIN = (0x28 | LC_REQ_DYLD),   /* replacement for LC_UNIXTHREAD */
    LC_DATA_IN_CODE = 0x29,           /* table of non-instructions in __text */
    LC_SOURCE_VERSION = 0x2A,         /* source version used to build binary */
    LC_DYLIB_CODE_SIGN_DRS = 0x2B,    /* code signing DRs copied from linked dylibs */
    LC_ENCRYPTION_INFO_64 = 0x2C,     /* 64-bit encrypted segment information */
    LC_LINKER_OPTION = 0x2D,          /* linker options in MH_OBJECT files */
    LC_LINKER_OPTIMIZATION_HINT = 0x2E, /* optimization hints in MH_OBJECT files */
    LC_VERSION_MIN_TVOS = 0x2F,       /* build for AppleTV min OS version */
    LC_VERSION_MIN_WATCHOS = 0x30,    /* build for WatchOS min OS version */
    LC_NOTE = 0x31,                   /* arbitrary data included within a Mach-O file */
    LC_BUILD_VERSION = 0x32,          /* build for platform min OS version */
    LC_DYLD_EXPORTS_TRIE = (0x33 | LC_REQ_DYLD), /* used with linkedit_data_command, payload is trie */
    LC_DYLD_CHAINED_FIXUPS = (0x34 | LC_REQ_DYLD), /* used with linkedit_data_command */
    LC_FILESET_ENTRY = (0x35 | LC_REQ_DYLD), /* used with fileset_entry_command */
    LC_ATOM_INFO = 0x36               /* used with linkedit_data_command */
} LoadCommandType;

struct segment_command_64 { /* for 64-bit architectures */
    uint32_t cmd;        /* LC_SEGMENT_64 */
    uint32_t cmdsize;    /* includes sizeof section_64 structs */
    char segname[16];    /* segment name */
    uint64_t vmaddr;        /* memory address of this segment */
    uint64_t vmsize;        /* memory size of this segment */
    uint64_t fileoff;    /* file offset of this segment */
    uint64_t filesize;    /* amount to map from the file */
    int32_t maxprot;    /* maximum VM protection */
    int32_t initprot;    /* initial VM protection */
    uint32_t nsects;        /* number of sections in segment */
    uint32_t flags;        /* flags */
};

struct symtab_command {
    uint32_t cmd;        /* LC_SYMTAB */
    uint32_t cmdsize;    /* sizeof(struct symtab_command) */
    uint32_t symoff;        /* symbol table offset */
    uint32_t nsyms;        /* number of symbol table entries */
    uint32_t stroff;        /* string table offset */
    uint32_t strsize;    /* string table size in bytes */
};

struct section_64 { /* for 64-bit architectures */
    char sectname[16];    /* name of this section */
    char segname[16];    /* segment this section goes in */
    uint64_t addr;        /* memory address of this section */
    uint64_t size;        /* size in bytes of this section */
    uint32_t offset;        /* file offset of this section */
    uint32_t align;        /* section alignment (power of 2) */
    uint32_t reloff;        /* file offset of relocation entries */
    uint32_t nreloc;        /* number of relocation entries */
    uint32_t flags;        /* flags (section type and attributes)*/
    uint32_t reserved1;    /* reserved (for offset or index) */
    uint32_t reserved2;    /* reserved (for count or sizeof) */
    uint32_t reserved3;    /* reserved */
};

section_64 *createSection64(const char *sectname,
                            const char *segname,
                            uint64_t addr,
                            uint64_t size,
                            uint32_t offset,
                            uint32_t align,
                            uint32_t reloff,
                            uint32_t nreloc,
                            uint32_t flags);

symtab_command *createSymtabCommand(uint32_t symoff,
                                    uint32_t nsyms,
                                    uint32_t stroff,
                                    uint32_t strsize);


segment_command_64 *createSegmentCommand64(const char *segname,
                                           uint64_t vmaddr,
                                           uint64_t vmsize,
                                           uint64_t fileoff,
                                           uint64_t filesize,
                                           uint32_t maxprot,
                                           uint32_t initprot,
                                           uint32_t nsects,
                                           uint32_t flags);

mach_header_64 *createMachHeader64(uint32_t cputype,
                                   uint32_t cpusubtype,
                                   uint32_t filetype,
                                   uint32_t ncmds,
                                   uint32_t sizeofcmds,
                                   uint32_t flags);

entry_point_command *createEntryPointCommand(uint64_t entryoff,
                                             uint64_t stacksize);

#endif //PCC_MACHO_H
