#ifndef INC_KEYBOARD_H_
#define INC_KEYBOARD_H_

#include "main.h"

#define ROW1 0xFE
#define ROW2 0xFD
#define ROW3 0xFB
#define ROW4 0xF7

// раскладка клавиатуры стенда (сверху вниз):
//   1 2 3
//   4 5 6
//   7 8 9
//   * 0 #

uint8_t Check_Row( uint8_t  Nrow );
HAL_StatusTypeDef Set_Keyboard( void );

// опрашивает все 4 строки и возвращает символ нажатой кнопки или 0
char KB_Scan( void );

#endif /* INC_KEYBOARD_H_ */
