#ifndef __ELF_H__
#define __ELF_H__

#include "macros.h"
#include "filesystem/filesystem.h"
#include "filesystem/disk.h"
#include "memory/kmalloc.h"
#include "system/process.h"
#include "memory/paging.h"
#include "memory/mmap.h"

// 0x7f 'E' 'L' 'F', backwards for endianness
#define ELF_MAGIC   0x464c457f

typedef enum {
    ET_NONE = 0,   // No file type
    ET_REL = 1,    // Relocatable file
    ET_EXEC = 2,   // Executable file
    ET_DYN = 3,    // Shared object file
    ET_CORE = 4,   // Core file
} elf_type;

typedef enum {
    EM_NONE        = 0,   // No machine
    EM_M32         = 1,   // AT&T WE 32100
    EM_SPARC       = 2,   // SPARC
    EM_386         = 3,   // Intel 80386
    EM_68K         = 4,   // Motorola 68000
    EM_88K         = 5,   // Motorola 88000
    /* 6 reserved */
    EM_860         = 7,   // Intel 80860
    EM_MIPS        = 8,   // MIPS I Architecture
    EM_S370        = 9,   // IBM System/370
    EM_MIPS_RS3_LE = 10,  // MIPS RS3000 Little-endian
    /* 11–14 reserved */
    EM_PARISC      = 15,  // HP PA-RISC
    /* 16 reserved */
    EM_VPP500      = 17,  // Fujitsu VPP500
    EM_SPARC32PLUS = 18,  // Enhanced SPARC
    EM_960         = 19,  // Intel 80960
    EM_PPC         = 20,  // PowerPC
    EM_PPC64       = 21,  // 64-bit PowerPC
    EM_S390        = 22,  // IBM System/390
    /* 23–35 reserved */
    EM_V800        = 36,  // NEC V800
    EM_FR20        = 37,  // Fujitsu FR20
    EM_RH32        = 38,  // TRW RH-32
    EM_RCE         = 39,  // Motorola RCE
    EM_ARM         = 40,  // ARM
    EM_ALPHA       = 41,  // Digital Alpha
    EM_SH          = 42,  // Hitachi SH
    EM_SPARCV9     = 43,  // SPARC v9
    EM_TRICORE     = 44,  // Siemens TriCore
    EM_ARC         = 45,  // Argonaut RISC Core
    EM_H8_300      = 46,  // Hitachi H8/300
    EM_H8_300H     = 47,  // Hitachi H8/300H
    EM_H8S         = 48,  // Hitachi H8S
    EM_H8_500      = 49,  // Hitachi H8/500
    EM_IA_64       = 50,  // Intel IA-64
    EM_MIPS_X      = 51,  // Stanford MIPS-X
    EM_COLDFIRE    = 52,  // Motorola ColdFire
    EM_68HC12      = 53,  // Motorola M68HC12
    EM_MMA         = 54,  // Fujitsu MMA
    EM_PCP         = 55,  // Siemens PCP
    EM_NCPU        = 56,  // Sony nCPU
    EM_NDR1        = 57,  // Denso NDR1
    EM_STARCORE    = 58,  // Motorola Star*Core
    EM_ME16        = 59,  // Toyota ME16
    EM_ST100       = 60,  // STMicroelectronics ST100
    EM_TINYJ       = 61,  // TinyJ
    EM_X86_64      = 62,  // AMD x86-64
    EM_PDSP        = 63,  // Sony DSP
    EM_PDP10       = 64,  // PDP-10
    EM_PDP11       = 65,  // PDP-11
    EM_FX66        = 66,  // Siemens FX66
    EM_ST9PLUS     = 67,  // STMicro ST9+
    EM_ST7         = 68,  // STMicro ST7
    EM_68HC16      = 69,  // Motorola MC68HC16
    EM_68HC11      = 70,  // Motorola MC68HC11
    EM_68HC08      = 71,  // Motorola MC68HC08
    EM_68HC05      = 72,  // Motorola MC68HC05
    EM_SVX         = 73,  // Silicon Graphics SVx
    EM_ST19        = 74,  // STMicro ST19
    EM_VAX         = 75,  // VAX
    EM_CRIS        = 76,  // Axis CRIS
    EM_JAVELIN     = 77,  // Infineon Javelin
    EM_FIREPATH    = 78,  // Element 14 FirePath
    EM_ZSP         = 79,  // LSI ZSP
    EM_MMIX        = 80,  // MMIX
    EM_HUANY       = 81,  // Harvard HUANY
    EM_PRISM       = 82,  // SiTera Prism
    EM_AVR         = 83,  // Atmel AVR
    EM_FR30        = 84,  // Fujitsu FR30
    EM_D10V        = 85,  // Mitsubishi D10V
    EM_D30V        = 86,  // Mitsubishi D30V
    EM_V850        = 87,  // NEC v850
    EM_M32R        = 88,  // Mitsubishi M32R
    EM_MN10300     = 89,  // Matsushita MN10300
    EM_MN10200     = 90,  // Matsushita MN10200
    EM_PJ          = 91,  // picoJava
    EM_OPENRISC    = 92,  // OpenRISC
    EM_ARC_A5      = 93,  // ARC Tangent-A5
    EM_XTENSA      = 94,  // Xtensa
    EM_VIDEOCORE   = 95,  // VideoCore
    EM_TMM_GPP     = 96,  // Thompson Multimedia GPP
    EM_NS32K       = 97,  // National Semiconductor 32000
    EM_TPC         = 98,  // Tenor TPC
    EM_SNP1K       = 99,  // Trebia SNP 1000
    EM_ST200       = 100  // STMicro ST200
} elf_machine;

typedef enum {
    EV_NONE = 0,   // Invalid version
    EV_CURRENT = 1 // Current version
} elf_version;

typedef enum {
    EI_MAG0       = 0,   // 0x7F
    EI_MAG1       = 1,   // 'E'
    EI_MAG2       = 2,   // 'L'
    EI_MAG3       = 3,   // 'F'
    EI_CLASS      = 4,   // File class
    EI_DATA       = 5,   // Data encoding
    EI_VERSION    = 6,   // File version
    EI_OSABI      = 7,   // OS/ABI identification
    EI_ABIVERSION = 8,   // ABI version
    EI_PAD        = 9,   // Start of padding bytes
    EI_NIDENT     = 16   // Size of e_ident[]
} elf_ident_index;

typedef enum {
    ELFCLASSNONE = 0, // Invalid class
    ELFCLASS32   = 1, // 32-bit objects
    ELFCLASS64   = 2  // 64-bit objects
} elf_class;

typedef enum {
    ELFOSABI_NONE      = 0,   // No extensions or unspecified
    ELFOSABI_HPUX      = 1,   // HP-UX
    ELFOSABI_NETBSD    = 2,   // NetBSD
    ELFOSABI_LINUX     = 3,   // Linux
    ELFOSABI_SOLARIS   = 6,   // Sun Solaris
    ELFOSABI_AIX       = 7,   // AIX
    ELFOSABI_IRIX      = 8,   // IRIX
    ELFOSABI_FREEBSD   = 9,   // FreeBSD
    ELFOSABI_TRU64     = 10,  // Compaq TRU64 UNIX
    ELFOSABI_MODESTO   = 11,  // Novell Modesto
    ELFOSABI_OPENBSD   = 12,  // OpenBSD
    ELFOSABI_OPENVMS   = 13,  // OpenVMS
    ELFOSABI_NSK       = 14   // HP Non-Stop Kernel
    /* 64–255: architecture-specific */
} elf_osabi;

typedef enum {
    PT_NULL      = 0,          // Unused entry
    PT_LOAD      = 1,          // Loadable segment
    PT_DYNAMIC   = 2,          // Dynamic linking info
    PT_INTERP    = 3,          // Interpreter path
    PT_NOTE      = 4,          // Auxiliary info
    PT_SHLIB     = 5,          // Reserved
    PT_PHDR      = 6,          // Program header table
    PT_TLS       = 7,          // Thread-local storage
    PT_LOOS      = 0x60000000, // OS-specific
    PT_HIOS      = 0x6fffffff, // OS-specific
    PT_LOPROC    = 0x70000000, // Processor-specific
    PT_HIPROC    = 0x7fffffff  // Processor-specific
} elf_program_header_type;

typedef enum {
    PF_X        = 0x1,        // Execute
    PF_W        = 0x2,        // Write
    PF_R        = 0x4,        // Read
    PF_MASKOS   = 0x0ff00000, // OS-specific
    PF_MASKPROC = 0xf0000000  // Processor-specific
} elf_program_header_flags;

typedef enum {
    SHF_WRITE            = 0x1,
    SHF_ALLOC            = 0x2,
    SHF_EXECINSTR        = 0x4,
    SHF_MERGE            = 0x10,
    SHF_STRINGS          = 0x20,
    SHF_INFO_LINK        = 0x40,
    SHF_LINK_ORDER       = 0x80,
    SHF_OS_NONCONFORMING = 0x100,
    SHF_GROUP            = 0x200,
    SHF_TLS              = 0x400,
    SHF_MASKOS           = 0x0ff00000,
    SHF_MASKPROC         = 0xf0000000
} elf_section_header_flags;


typedef enum {
    PF_NONE        = 0,                 // No access
    PF_X_ONLY      = PF_X,              // Execute only
    PF_W_ONLY      = PF_W,              // Write only
    PF_WX          = PF_W | PF_X,       // Write + Execute
    PF_R_ONLY      = PF_R,              // Read only
    PF_RX          = PF_R | PF_X,       // Read + Execute
    PF_RW          = PF_R | PF_W,       // Read + Write
    PF_RWX         = PF_R | PF_W | PF_X // Read + Write + Execute
} elf_program_header_permissions;



typedef struct {
    uint8_t e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elf32_header;


typedef struct {
    uint8_t e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elf64_header;

typedef struct {
  uint32_t sh_name;      // Section name (index into string table)
  uint32_t sh_type;      // Section type (SHT_*)
  uint32_t sh_flags;     // Section flags (SHF_*)
  uint32_t sh_addr;      // Address where section is to be loaded
  uint32_t sh_offset;     // File offset of section data, in bytes
  uint32_t sh_size;      // Size of section, in bytes
  uint32_t sh_link;      // Section type-specific header table index link
  uint32_t sh_info;      // Section type-specific extra information
  uint32_t sh_addralign; // Section address alignment
  uint32_t sh_entsize;   // Size of records contained within the section
} elf32_section_header;
 
// Section header for ELF64 - same fields as ELF32, different types.
typedef struct {
  uint32_t sh_name;
  uint32_t sh_type;
  uint64_t sh_flags;
  uint64_t sh_addr;
  uint64_t sh_offset;
  uint64_t sh_size;
  uint32_t sh_link;
  uint32_t sh_info;
  uint64_t sh_addralign;
  uint64_t sh_entsize;
} elf64_section_header;


typedef struct {
	uint32_t	p_type;
	uint32_t	p_offset;
	uint32_t	p_vaddr;
	uint32_t	p_paddr;
	uint32_t	p_filesz;
	uint32_t	p_memsz;
	uint32_t	p_flags;
	uint32_t	p_align;
} elf32_program_header;


typedef struct {
	uint32_t	p_type;
	uint32_t	p_flags;
	uint64_t	p_offset;
	uint64_t	p_vaddr;
	uint64_t	p_paddr;
	uint64_t	p_filesz;
	uint64_t	p_memsz;
	uint64_t	p_align;
} elf64_program_header;



void readelf(file_t* file);

#endif
