#ifndef __IRQ_H__
#define __IRQ_H__

void irq_init_vectors();
void irq_enable();
void irq_disable(); 
void enable_interrupt_controller();

#endif