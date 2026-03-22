#include "system/gic.h"

void initialize_gic() {

    // Disable first
    *GICD_CTRLR = 0;
    *GICC_CTRLR = 0;

    // Set all interrupts to Group 1
    for (int i = 0; i < (IT_NR / 32); i++) {
        GICD_IGROUPR[i] = 0xFFFFFFFF;
    }


    // 1. Enable Distributor (Group 0 and 1)
    *GICD_CTRLR = 3;

    // 2. Enable CPU Interface (Group 0 and 1) and set priority mask
    *GICC_CTRLR = 3;
    *GICC_PMR = 0xFF;

    // 3. Set Target Core for Interrupt 96 (SPI)
    // We use a byte pointer here because ITARGETSR is byte-addressable
    // GICD_ITARGETSR[96] = 0x01; 
    GICD_ITARGETSR[MINI_UART_RECV] = 0x01; 
    GICD_IPRIORITYR[MINI_UART_RECV] = 0x80;

    GICD_ISENABLER[PHYS_TIMER / 32] |= (1 << (PHYS_TIMER % 32));
    GICD_ISENABLER[VIRT_TIMER / 32] |= (1 << (VIRT_TIMER % 32));
    GICD_ISENABLER[MINI_UART_RECV / 32] |= (1 << (MINI_UART_RECV % 32));



    *GICD_CTRLR = 1;
}