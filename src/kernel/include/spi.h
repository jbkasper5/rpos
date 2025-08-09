#ifndef __SPI_H__
#define __SPI_H__

#include "macros.h"

void spi_init();
void spi_send_recv(uint8_t chip_select, uint8_t* sbuffer, uint8_t* rbuffer, uint32_t size);

void spi_send(uint8_t chip_select, uint8_t* data, uint32_t size);
void spi_recv(uint8_t chip_select, uint8_t* data, uint32_t size);

#endif