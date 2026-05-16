#include "system/gic.h"

void initialize_gic() {

    // Disable first
    *GICD_CTRLR = 0;
    *GICC_CTRLR = 0;

    // Set all interrupts to Group 1
    for (int i = 0; i < (IT_NR / 32); i++) {
        GICD_IGROUPR[i] = 0xFFFFFFFF;
    }

    *GICC_PMR = 0xFF;

    ((u8*)GICD_IPRIORITYR)[PHYS_TIMER] = 0x80;
    ((u8*)GICD_IPRIORITYR)[VIRT_TIMER] = 0x80;
    ((u8*)GICD_IPRIORITYR)[MINI_UART_RECV] = 0x80;

    // 3. Set Target Core for Interrupt 96 (SPI)
    // We use a byte pointer here because ITARGETSR is byte-addressable
    GICD_ISENABLER[PHYS_TIMER / 32] |= (1 << (PHYS_TIMER % 32));
    GICD_ISENABLER[VIRT_TIMER / 32] |= (1 << (VIRT_TIMER % 32));
    GICD_ISENABLER[MINI_UART_RECV / 32] |= (1 << (MINI_UART_RECV % 32));

    // GICD_NSACR[PHYS_TIMER / 32] |= (1 << (PHYS_TIMER % 32));
    // GICD_NSACR[VIRT_TIMER / 32] |= (1 << (VIRT_TIMER % 32));

    // set the key press handling to core 1
    ((u8*)GICD_ITARGETSR)[MINI_UART_RECV] = 0x01;

    *GICD_CTRLR = 3;
    *GICC_CTRLR = 3;
}