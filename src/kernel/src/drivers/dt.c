#include "drivers/dt.h"
#include "memory/mem.h"

static inline uint32_t swap_endianness_32(uint32_t data) {
    return __builtin_bswap32(data);
}

static inline uint64_t swap_endianness_64(uint64_t data) {
    return __builtin_bswap64(data);
}

static uint32_t* parse_property(fdt_header* header, uint32_t* property){

    fdt_property prop;
    memcpy(&prop, property, sizeof(fdt_property));
    
    prop.len = swap_endianness_32(prop.len);
    prop.nameoff = swap_endianness_32(prop.nameoff);

    char* strtable = UNSCALED_POINTER_ADD(header, header->off_dt_strings);

    INFO("\t%s: len = %d\n", strtable + prop.nameoff, prop.len);

    int skip = ALIGN_UP(prop.len, 4) + sizeof(fdt_property);
    return UNSCALED_POINTER_ADD(property, skip);
}

static uint32_t* parse_node(fdt_header* header, uint32_t* ptr){
    char* nodename = (char*) ptr;
    INFO("Node: \e[34m%s\e[0m\n", nodename);

    // align ptr to a 4 byte value after parsing the name
    ptr = UNSCALED_POINTER_ADD(ptr, ALIGN_UP(strlen(nodename), 4));

    // aftter the name, we can start parsing the node
    uint32_t val = swap_endianness_32(*ptr);
    while(val != FDT_END_NODE){
        if(val == FDT_BEGIN_NODE){
            ptr = parse_node(header, ptr + 1);
        }else if(val == FDT_PROP){
            ptr = parse_property(header, ptr + 1);
        }else{
            ptr++;
        }
        val = swap_endianness_32(*ptr);
    }

    return ptr;
}

static void parse_device_tree(fdt_header* header){
    uint32_t* p = UNSCALED_POINTER_ADD(header, header->off_dt_struct);
    uint32_t val = swap_endianness_32(*p);
    while(val != FDT_END){
        if(val == FDT_BEGIN_NODE){
            p = parse_node(header, p + 1);
        }else{
            p++;
        }
        val = swap_endianness_32(*p);
    }
}

void device_tree_init(){
    uint64_t tree = get_device_tree_start();


    INFO("Tree loaded at 0x%x\n", tree);

    
    fdt_header* header = (fdt_header*) tree;

    header->magic = swap_endianness_32(header->magic);
    header->totalsize = swap_endianness_32(header->totalsize);
    header->off_dt_struct = swap_endianness_32(header->off_dt_struct);
    header->off_dt_strings = swap_endianness_32(header->off_dt_strings);
    header->off_mem_rsvmap = swap_endianness_32(header->off_mem_rsvmap);
    header->version = swap_endianness_32(header->version);
    header->last_comp_version = swap_endianness_32(header->last_comp_version);
    header->boot_cpuid_phys = swap_endianness_32(header->boot_cpuid_phys);
    header->size_dt_strings = swap_endianness_32(header->size_dt_strings);
    header->size_dt_struct = swap_endianness_32(header->size_dt_struct);

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

    parse_device_tree(header);
}