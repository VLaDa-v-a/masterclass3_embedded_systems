#include "main.h"
#include "pca9538.h"
#include "kb.h"

// адрес PCA9538 к которой подключена клавиатура
#define KBRD_ADDR 0xE2

// функции Set_Keyboard и Check_Row взяты из примера SDK_Keyboard

HAL_StatusTypeDef Set_Keyboard( void ) {
	HAL_StatusTypeDef ret = HAL_OK;
	uint8_t buf;

	buf = 0;
	ret = PCA9538_Write_Register(KBRD_ADDR, POLARITY_INVERSION, &buf);
	if( ret != HAL_OK ) {
		return ret;
	}

	buf = 0;
	ret = PCA9538_Write_Register(KBRD_ADDR, OUTPUT_PORT, &buf);

	return ret;
}

uint8_t Check_Row( uint8_t  Nrow ) {
	uint8_t Nkey = 0x00;
	HAL_StatusTypeDef ret = HAL_OK;
	uint8_t buf;
	uint8_t kbd_in;

	ret = Set_Keyboard();
	if( ret != HAL_OK ) {
		return 0;
	}

	// выставляем 0 на нужную строку, остальные строки на вход
	buf = Nrow;
	ret = PCA9538_Write_Register(KBRD_ADDR, CONFIG, &buf);
	if( ret != HAL_OK ) {
		return 0;
	}

	ret = PCA9538_Read_Inputs(KBRD_ADDR, &buf);
	if( ret != HAL_OK ) {
		return 0;
	}

	// столбцы на P4-P6, нажатая кнопка дает 0
	kbd_in = buf & 0x70;
	if( kbd_in != 0x70) {
		if( !(kbd_in & 0x10) ) Nkey = 0x04;
		if( !(kbd_in & 0x20) ) Nkey = 0x02;
		if( !(kbd_in & 0x40) ) Nkey = 0x01;
	}
	else Nkey = 0x00;

	return Nkey;
}

char KB_Scan( void ) {
	static const uint8_t rows[4] = { ROW1, ROW2, ROW3, ROW4 };
	static const char keymap[4][3] = {
		{ '1', '2', '3' },
		{ '4', '5', '6' },
		{ '7', '8', '9' },
		{ '*', '0', '#' }
	};
	uint8_t key;

	for (int r = 0; r < 4; r++) {
		key = Check_Row(rows[r]);
		if (key == 0x04) {
			return keymap[r][0]; // левый столбец
		} else if (key == 0x02) {
			return keymap[r][1]; // средний столбец
		} else if (key == 0x01) {
			return keymap[r][2]; // правый столбец
		}
	}
	return 0;
}
