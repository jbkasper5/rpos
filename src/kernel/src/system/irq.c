#include "io/kprintf.h"
#include "system/entry.h"
#include "peripherals/irq.h"
#include "peripherals/aux.h"
#include "io/mini_uart.h"
#include "utils/utils.h"
#include "utils/timer.h"
#include "system/scheduler.h"
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

void show_invalid_entry_message(uint32_t type, uint64_t esr, uint64_t instruction, uint64_t address){
    ERROR("%s\n", entry_error_messages[type]);
	ERROR("\tException: 0x%x\n", esr);
	ERROR("\tFaulting instruction: 0x%x\n", instruction);
	ERROR("\tAddress causing fault: 0x%x\n", address);
}

void enable_interrupt_controller() {
	// enable CPU interface for GIC
	// interrupts have already been enabled in EL3 before dropping into kernel init
	REGS_GICC->gicc_ctlr = 0x1;
}


void handle_irq(uint64_t reg_addr, uint8_t el){
	DEBUG("Handling IRQ from EL %d\n", el);
	// DEBUG("Handling IRQ (context: 0x%x)...\n", reg_addr);

    uint32_t irq = REGS_BCMIRQ->irq0_pending_0;
	uint32_t gic_irq = REGS_GICC->gicc_iar;

	DEBUG("BCM IRQ: %d, GIC IRQ: %d\n", irq, gic_irq);
    while(irq){
        if(irq & AUX_IRQ){
            irq &= ~AUX_IRQ;

            while((REGS_AUX->mu_iir & 4) == 4){
                DEBUG("UART Recv: ");
                uart_putc(uart_getc());
                DEBUG("\n");
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

	// DEBUG("GIC IRQ: %d\n", gic_irq);

    if (gic_irq < 1020) {  // 1020 = spurious interrupt ID threshold
        if (gic_irq == 30) {
			DEBUG("Handling interrupt 30...\n");
            // Handle ARM physical timer interrupt
			// if in EL1, just prime the timer
			// if in EL0, actually jump to scheduler code
			if(el){
				prime_physical_timer();
			}else{
				scheduler((reglist_t*) reg_addr);
			}
        }else if(gic_irq == 27){
			DEBUG("Handling interrupt 27...\n");
			
			// handle the timer sleep stack
			handle_virtual_timer();
		}
        // Acknowledge end of interrupt
        REGS_GICC->gicc_eoir = gic_irq;
    }
}