#include "filesystem/elf.h"
#include "system/entry.h"

extern void do_user_things();


void readelf(file_t* file){
    ext4_block* block = (ext4_block*) kmalloc(sizeof(ext4_block));
    read(file, block, sizeof(ext4_block));

    elf64_header* header = (elf64_header*) block->data;
    u32 magic = *(u32*) header->e_ident;

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

    u32 section_header_count = header->e_shnum;
    u32 string_table_section_header = header->e_shstrndx;

    // allocate process metadata
    // should now have a valid L0 page table and stack
    // pcb_t* process = procalloc((u64) header->e_entry);

    u64* new_proc_l0 = buddy_alloc_pt();

    elf64_program_header* program_header = UNSCALED_POINTER_ADD(header, header->e_phoff);

    for(int i = 0; i < header->e_phnum; i++){
        INFO("Program header %d:\n", i + 1);
        INFO("  Type: %d\n", program_header->p_type);
        INFO("  Flags: 0x%x\n", program_header->p_flags);
        INFO("  Offset: %d\n", program_header->p_offset);
        INFO("  Virtual address: 0x%x\n", program_header->p_vaddr);

        // if the section requires allocation, then allocate it
        u64 flags = MAP_USER;
        if(program_header->p_flags & PF_R) flags |= MAP_READ;
        if(program_header->p_flags & PF_W) flags |= MAP_WRITE;
        if(program_header->p_flags & PF_X) flags |= MAP_EXEC;

        u16 order = log2_pow2(program_header->p_memsz / 4096);

        u64 phys_block = buddy_alloc(program_header->p_memsz);

        // map physical block into kernel memory so we can set up the process
        map(phys_block, va_to_pa(phys_block), order, MAP_KERNEL | MAP_WRITE, L0_TABLE);

        seek(file, program_header->p_offset, SEEK_SET);
        read(file, rootfs.block_buf, program_header->p_filesz);

        u64 in_page_offset = program_header->p_vaddr % PAGE_SIZE;

        memcpy(UNSCALED_POINTER_ADD(phys_block, in_page_offset), rootfs.block_buf, program_header->p_filesz);

        // map the program sections into the new L0 table for the current process
        map(program_header->p_vaddr, va_to_pa(phys_block), order, flags, new_proc_l0); // 0x400000 -> 3ffd7000

        program_header++;
    }

    pcb_t* current = get_current();

    // needs updating to the location of the ELF maybe
    // current->cwd

    // switch over ttbr
    u64* old_ttbr = current->ttbr;

    current->ttbr = va_to_pa(new_proc_l0);

    // tear down old_ttbr
    // teardown_vm(old_ttbr);

    // read trapframe to set the new process proper PC
    u64* trapframe = (u64*)(ALIGN_UP(current->kernel_stack, PAGE_SIZE) - 0x10 - S_FRAME_SIZE);
    trapframe[31] = 0x0000800000000ULL - 16;           // SP_EL0
    trapframe[32] = header->e_entry;                   // ELR_EL1
    trapframe[33] = 0x0;                               // SPSR


    u32 stack_size = PAGE_SIZE * 2;
    u64 stack_base = buddy_alloc(stack_size);

    // map new user stack
    map(0x0000800000000ULL - stack_size, va_to_pa(stack_base), 1, MAP_USER | MAP_READ | MAP_WRITE, new_proc_l0);
    // 0x800000000
    // 0x7fffffff0

    // swap the base table for the process
    switch_user_tlb(current->ttbr);

    // update user sp, pc, and zero out other registers
}