#include "filesystem/elf.h"


void readelf(file_t* file){
    ext4_block* block = (ext4_block*) kmalloc(sizeof(ext4_block));
    read(file, block, sizeof(ext4_block));

    elf64_header* header = (elf64_header*) block->data;
    uint32_t magic = *(uint32_t*) header->e_ident;

    // check magic
    if(magic != ELF_MAGIC){
        ERROR("File is not an ELF executable.\n");
        return;
    }

    INFO("ELF Header:\n");
    INFO("  Type: %d\n", header->e_type);
    INFO("  Machine: %d\n", header->e_machine);
    INFO("  Version: %d\n", header->e_version);
    INFO("  Entry point: 0x%x\n", header->e_entry);
    INFO("  Program header offset: %d\n", header->e_phoff);
    INFO("  Section header offset: %d\n", header->e_shoff);
    INFO("  Flags: 0x%x\n", header->e_flags);
    INFO("  ELF header size: %d\n", header->e_ehsize);
    INFO("  Program header entry size: %d\n", header->e_phentsize);
    INFO("  Program header entry count: %d\n", header->e_phnum);
    INFO("  Section header entry size: %d\n", header->e_shentsize);
    INFO("  Section header entry count: %d\n", header->e_shnum);
    INFO("  Section header string table index: %d\n", header->e_shstrndx);

    uint32_t section_header_count = header->e_shnum;
    uint32_t string_table_section_header = header->e_shstrndx;

    // allocate process metadata
    // should now have a valid L0 page table and stack
    pcb_t* process = procalloc();
    process->registers.pc = header->e_entry;

    elf64_program_header* program_header = UNSCALED_POINTER_ADD(header, header->e_phoff);

    for(int i = 0; i < header->e_phnum; i++){
        INFO("Program header %d:\n", i + 1);
        INFO("  Type: %d\n", program_header->p_type);
        INFO("  Flags: 0x%x\n", program_header->p_flags);
        INFO("  Offset: %d\n", program_header->p_offset);
        INFO("  Virtual address: 0x%x\n", program_header->p_vaddr);

        // if the section requires allocation, then allocate it
        uint64_t flags = MAP_USER;
        if(program_header->p_flags & PF_R) flags |= MAP_READ;
        if(program_header->p_flags & PF_W) flags |= MAP_WRITE;
        if(program_header->p_flags & PF_X) flags |= MAP_EXEC;

        uint16_t order = log2_pow2(program_header->p_memsz / 4096);

        uint64_t phys_block = buddy_alloc(program_header->p_memsz);

        // map physical block into kernel memory so we can set up the process
        map(phys_block, phys_block, order, MAP_KERNEL | MAP_WRITE, L0_TABLE);

        seek(file, program_header->p_offset, SEEK_SET);
        read(file, rootfs.block_buf, program_header->p_filesz);
        memcpy(phys_block, rootfs.block_buf, program_header->p_filesz);

        // munmap the physical block
        map(program_header->p_vaddr, phys_block, order, flags, process->registers.ttbr);
        program_header++;
    }

    add_to_schedule(process);
    

    seek(file, header->e_shoff, SEEK_SET);
    read(file, block, sizeof(ext4_block));

    elf64_section_header* section_header = (elf64_section_header*) block;

    // read the string table into memory
    char* string_table = (char*) kmalloc(sizeof(ext4_block));
    seek(file, (section_header + string_table_section_header)->sh_offset, SEEK_SET);
    read(file, string_table, (section_header + string_table_section_header)->sh_size);

    section_header = (elf64_section_header*) block;
    uint8_t requires_allocation;
    for(int i = 0; i < section_header_count; i++){
        requires_allocation = section_header->sh_flags & SHF_ALLOC ? TRUE : FALSE;
        INFO("Section header %d: \n", i + 1);
        INFO("  Name: %s\n", string_table + section_header->sh_name);
        INFO("  Type: %d\n", section_header->sh_type);
        INFO("  Flags: 0x%x\n", section_header->sh_flags);
        INFO("  Address: 0x%x\n", section_header->sh_addr);
        INFO("  Offset: %d\n", section_header->sh_offset);
        INFO("  Size: %d\n", section_header->sh_size);
        INFO("  \e[31mREQUIRES MAPPING:\e[0m %d\n", requires_allocation);

        section_header++;
    }
}