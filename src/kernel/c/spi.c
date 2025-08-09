#include "spi.h"
#include "peripherals/spi.h"
#include "gpio.h"
#include "print.h"

void spi_init(){
    gpio_pin_set_func(7, GFAlt0);
    gpio_pin_set_func(8, GFAlt0);
    gpio_pin_set_func(9, GFAlt0);
    gpio_pin_set_func(10, GFAlt0);
    gpio_pin_set_func(11, GFAlt0);

    gpio_pin_enable(7);
    gpio_pin_enable(8);
    gpio_pin_enable(9);
    gpio_pin_enable(10);
    gpio_pin_enable(11);
}

void spi_send_recv(uint8_t chip_select, uint8_t* sbuffer, uint8_t* rbuffer, uint32_t size){
    REGS_SPI0->data_length = size;
    REGS_SPI0->cs = (REGS_SPI0->cs & ~CS_CS) | (chip_select << CS_CS__SHIFT) | CS_CLEAR_RX | CS_CLEAR_TX | CS_TA;

    uint32_t readcount = 0;
    uint32_t writecount = 0;

    while(readcount < size || writecount < size){
        while(writecount < size && REGS_SPI0->cs & CS_TXD){
            REGS_SPI0->fifo = sbuffer ? *sbuffer++ : 0;
            writecount++;
        }

        while(readcount < size && REGS_SPI0->cs & CS_RXD){
            uint32_t data = REGS_SPI0->fifo;
            if(rbuffer) *rbuffer++ = data;
            readcount++;
        }
    }

    while(!(REGS_SPI0->cs & CS_DONE)){
        while(REGS_SPI0->cs & CS_RXD){
            uint32_t r = REGS_SPI0->fifo;
            printf("Extra data: %d\n", r);
        }
    }

    REGS_SPI0->cs = (REGS_SPI0->cs & ~CS_TA);
}

void spi_send(uint8_t chip_select, uint8_t* data, uint32_t size){
    spi_send_recv(chip_select, data, 0, size);
}

void spi_recv(uint8_t chip_select, uint8_t* data, uint32_t size){
    spi_send_recv(chip_select, 0, data, size);
}
