#include "mmu.h"
#include "mm.h"
#include "virtual_memory.h"
#include "peripherals/base.h"
#include "printf.h"

/*
+===========================+===========================+
|          Address          |          Region           |
+===========================+===========================+
| 0x0                       | RAM Start                 |
+---------------------------+---------------------------+
| 0x8000                    | Kernel code + data        |
+---------------------------+---------------------------+
| __kernel_end              | End of kernel code        |
+---------------------------+---------------------------+
| __kernel_end + KSTACK     | Start of the kernel stack |
+---------------------------+---------------------------+
| __kernel_end + KSTACK + 1 | User addressable memory   |
+---------------------------+---------------------------+
*/

uint64_t mmutest(uint64_t);

void print_page_table(uint64_t* pt_addr, int n_entries){
    int n = n_entries;
    printf("Dumping top %d entries of page table at address 0x%x:\n", n, pt_addr);
    for(int i = 0; i < n; i++){
        printf("\t%d: 0x%x\n", i, pt_addr[i]);
    }
}

void create_kernel_identity_mapping(){
    /*
    Once page frame array is populated, we can allocate pages and create 
    the page table mapping for the kernel memory
    */
}


void initialize_page_tables(void* ptb, pt_metadata_t* pt_metadata_start){
    initialize_page_frame_array();
    create_kernel_identity_mapping();
}