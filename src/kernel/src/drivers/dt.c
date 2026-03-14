#include "drivers/dt.h"

static inline uint32_t swap_endianness(uint32_t data) {
    return __builtin_bswap32(data);
}

void device_tree_init(){
    uint64_t tree = get_device_tree_start();


    INFO("Tree loaded at 0x%x\n", tree);

    
    dt_header* header = (dt_header*) tree;

    header->magic = swap_endianness(header->magic);
    header->totalsize = swap_endianness(header->totalsize);
    header->off_dt_struct = swap_endianness(header->off_dt_struct);
    header->off_dt_strings = swap_endianness(header->off_dt_strings);
    header->off_mem_rsvmap = swap_endianness(header->off_mem_rsvmap);
    header->version = swap_endianness(header->version);
    header->last_comp_version = swap_endianness(header->last_comp_version);
    header->boot_cpuid_phys = swap_endianness(header->boot_cpuid_phys);
    header->size_dt_strings = swap_endianness(header->size_dt_strings);
    header->size_dt_struct = swap_endianness(header->size_dt_struct);

    INFO("Parsed header: \n");
    INFO("\tmagic: 0x%x\n", header->magic);
    INFO("\tsize: 0x%x\n", header->totalsize);
    INFO("\toff_dt_struct: 0x%x\n", header->off_dt_struct);
    INFO("\toff_dt_strings: 0x%x\n", header->off_dt_strings);
    INFO("\toff_mem_rsvmap: 0x%x\n", header->off_mem_rsvmap);
    INFO("\tversion: 0x%x\n", header->version);
    INFO("\tlast_comp_version: 0x%x\n", header->last_comp_version);
    INFO("\tboot_cpuid_phys: 0x%x\n", header->boot_cpuid_phys);
    INFO("\tsize_dt_strings: 0x%x\n", header->size_dt_strings);
    INFO("\tsize_dt_struct: 0x%x\n", header->size_dt_struct);


    uint32_t* p = (uint32_t *)(tree + header->off_dt_struct);

    // while (swap_endianness(*p) != 0x00000009) {
    //     uint32_t token = swap_endianness(*p++);

    //     if (token == 0x00000001) { // BEGIN_NODE
    //         INFO("Name: %s\n", (char*) p);
    //     } else if (token == 0x00000003) { // PROP
    //         uint32_t len = swap_endianness(*p++);
    //         uint32_t nameoff = swap_endianness(*p++);

    //         INFO("Len: %d\n", len);
    //         INFO("Name offset: %d\n", nameoff);

    //         // Property name is at: (dtb_base + off_dt_strings + nameoff)
    //         // Property data is at: (p)
            
    //         p += (len + 3) / 4; // Skip data + align
    //     }
    //     // ... handle END_NODE and NOP ...
    // }
}