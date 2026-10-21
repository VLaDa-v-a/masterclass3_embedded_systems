#ifndef INC_TETRIS_H_
#define INC_TETRIS_H_

#include "main.h"

// стакан 10x20 клеток
#define BOARD_W  10
#define BOARD_H  20

// состояния игры
#define GAME_START  0
#define GAME_RUN    1
#define GAME_PAUSE  2
#define GAME_OVER   3

// результат обработки события (нужно для звука и перерисовки)
#define RES_NONE   0   // ничего не поменялось
#define RES_MOVE   1   // фигура сдвинулась или сменилось состояние
#define RES_LOCK   2   // фигура упала
#define RES_LINES  3   // фигура упала и убрались линии
#define RES_OVER   4   // конец игры

void Tetris_Init(void);
uint8_t Tetris_Key(char key);
uint8_t Tetris_Fall(void);
void Tetris_Draw(void);
uint8_t Tetris_GetState(void);
uint32_t Tetris_GetSpeed(void);

#endif /* INC_TETRIS_H_ */
