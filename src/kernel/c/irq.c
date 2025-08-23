#include "printf.h"
#include "entry.h"
#include "peripherals/irq.h"
#include "peripherals/aux.h"
#include "mini_uart.h"
#include "utils.h"
#include "timer.h"
#include "scheduler.h"
#include "peripherals/gic.h"


const char entry_error_messages[16][32] = {
	"SYNC_INVALID_EL1t",
	"IRQ_INVALID_EL1t",		
	"FIQ_INVALID_EL1t",		
	"ERROR_INVALID_EL1T",		

	"SYNC_INVALID_EL1h",
	"IRQ_INVALID_EL1h",		
	"FIQ_INVALID_EL1h",		
	"ERROR_INVALID_EL1h",		

	"SYNC_INVALID_EL0_64",		
	"IRQ_INVALID_EL0_64",		
	"FIQ_INVALID_EL0_64",		
	"ERROR_INVALID_EL0_64",	

	"SYNC_INVALID_EL0_32",		
	"IRQ_INVALID_EL0_32",		
	"FIQ_INVALID_EL0_32",		
	"ERROR_INVALID_EL0_32"	
};

void show_invalid_entry_message(uint32_t type, uint64_t esr, uint64_t address){
    printf("ERROR CAUGHT: %s - %d, ESR: %x, ADDRESS: %x\n", entry_error_messages[type], type, esr, address);
}

void enable_gic_interrupt(uint32_t interrupt_id, uint8_t priority){
	uint32_t indexer = interrupt_id / 32;
	uint32_t offset = interrupt_id % 32;
	REGS_GICD->gicd_isenabler[indexer] |= (1 << offset);
}

void assign_interrupt_core(uint32_t INTID, uint32_t core)
{
    uint32_t n = INTID / 4;
    uint32_t byte_offset = INTID % 4;
    uint32_t shift = byte_offset * 8 + core;
    REGS_GICD->gicd_itargetsr[n] |= (1 << shift);
}


#define GENERIC_TIMER_INTERRUPT_ID 	30

void enable_interrupt_controller() {
	// enable_gic_interrupt(GENERIC_TIMER_INTERRUPT_ID, 0xA0);
	// assign_interrupt_core(GENERIC_TIMER_INTERRUPT_ID, 0);

	// enable CPU interface for GIC
	REGS_GICC->gicc_ctlr = 0x1;
}


void handle_irq(){
	printf("Handling IRQ...\n");
    uint32_t irq = REGS_BCMIRQ->irq0_pending_0;
	uint32_t gic_irq = REGS_GICC->gicc_iar;

	printf("BCM IRQ: %d, GIC IRQ: %d\n", irq, gic_irq);
    while(irq){
        if(irq & AUX_IRQ){
            irq &= ~AUX_IRQ;

            while((REGS_AUX->mu_iir & 4) == 4){
                printf("UART Recv: ");
                uart_putc(uart_getc());
                printf("\n");
            }
        }

		if (irq & SYS_TIMER_IRQ_1){
			irq &= ~SYS_TIMER_IRQ_1;

			handle_timer_1();
		}

		if (irq & SYS_TIMER_IRQ_3){
			irq &= ~SYS_TIMER_IRQ_3;

			handle_timer_3();
		}
    }

	printf("GIC IRQ: %d\n", gic_irq);

    if (gic_irq < 1020) {  // 1020 = spurious interrupt ID threshold
        if (gic_irq == 30) {
			printf("Handling interrupt 30...\n");
            // Handle ARM physical timer interrupt
			handle_physical_timer();
        }
        // Acknowledge end of interrupt
        REGS_GICC->gicc_eoir = gic_irq;
    }
}