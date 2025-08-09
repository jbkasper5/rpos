#ifndef __I2C_H__
#define __I2C_H__

#include "macros.h"

typedef enum I2C_STATUS_E{
    I2CS_SUCCESS,
    I2CS_ACK_ERROR,
    I2CS_DATA_LOSS,
    I2CS_CLOCK_TIMEOUT
}i2c_status_t;

void i2c_init();
i2c_status_t i2c_receive(uint8_t address, uint8_t* buffer, uint32_t size);
i2c_status_t i2c_send(uint8_t address, uint8_t* buffer, uint32_t size);

#endif