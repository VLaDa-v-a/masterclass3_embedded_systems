#include "main.h"

#ifndef PCA9538_H_
#define PCA9538_H_

// регистры расширителя портов PCA9538 (взято из примера SDK_Keyboard)
typedef enum{
	INPUT_PORT = 0x00, //Read byte XXXX XXXX
	OUTPUT_PORT = 0x01, //Read/write byte 1111 1111
	POLARITY_INVERSION = 0x02, //Read/write byte 0000 0000
	CONFIG = 0x03 //Read/write byte 1111 1111
}pca9538_regs_t;

HAL_StatusTypeDef PCA9538_Read_Register(uint16_t addr, pca9538_regs_t reg, uint8_t* buf);
HAL_StatusTypeDef PCA9538_Write_Register(uint16_t addr, pca9538_regs_t reg, uint8_t* buf);
HAL_StatusTypeDef PCA9538_Read_Inputs(uint16_t addr, uint8_t* buf);

#endif /* PCA9538_H_ */
