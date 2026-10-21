#include "tetris.h"
#include "oled.h"
#include "fonts.h"
#include <stdio.h>
#include <stdlib.h>

// положение стакана на экране, клетка 3x3 пикселя
#define BOARD_X  2
#define BOARD_Y  2
#define CELL     3

// окошко со следующей фигурой, там клетка 4x4
#define NEXT_X   94
#define NEXT_Y   12

// 7 фигур по 4 поворота, фигура записана в квадрате 4x4
// старший бит - левая верхняя клетка, дальше по строкам
static const uint16_t pieces[7][4] = {
	{ 0x0F00, 0x2222, 0x00F0, 0x4444 }, // I
	{ 0x44C0, 0x8E00, 0x6440, 0x0E20 }, // J
	{ 0x4460, 0x0E80, 0xC440, 0x2E00 }, // L
	{ 0xCC00, 0xCC00, 0xCC00, 0xCC00 }, // O
	{ 0x06C0, 0x8C40, 0x6C00, 0x4620 }, // S
	{ 0x0E40, 0x4C40, 0x4E00, 0x4640 }, // T
	{ 0x0C60, 0x4C80, 0xC600, 0x2640 }  // Z
};

// очки за 1, 2, 3 и 4 линии сразу
static const uint16_t linePoints[5] = { 0, 100, 300, 500, 800 };

// примитивы клеток (строки по 3 и 4 бита)
static const uint8_t spr_block3[3] = { 0x07, 0x05, 0x07 };
static const uint8_t spr_block4[4] = { 0x0F, 0x09, 0x09, 0x0F };

static uint8_t board[BOARD_H][BOARD_W];

static uint8_t curType, curRot, nextType;
static int curX, curY;

static uint8_t state = GAME_START;
static unsigned long score;
static uint16_t lines;
static uint8_t level;

// есть ли клетка фигуры в строке r и столбце c
static uint8_t PieceCell(uint16_t shape, int r, int c)
{
	return (shape & (0x8000 >> (r * 4 + c))) != 0;
}

// проверка, можно ли поставить фигуру в это место
static uint8_t Collide(uint8_t type, uint8_t rot, int x, int y)
{
	uint16_t shape = pieces[type][rot];
	int bx, by;

	for (int r = 0; r < 4; r++) {
		for (int c = 0; c < 4; c++) {
			if (!PieceCell(shape, r, c)) continue;
			bx = x + c;
			by = y + r;
			if (bx < 0 || bx >= BOARD_W || by >= BOARD_H) return 1;
			if (by >= 0 && board[by][bx]) return 1;
		}
	}
	return 0;
}

// новая фигура сверху, возвращает 0 если ее уже некуда поставить
static uint8_t NewPiece(void)
{
	curType = nextType;
	nextType = rand() % 7;
	curRot = 0;
	curX = 3;
	// если верхняя строка фигуры пустая, поднимаем ее на клетку выше
	curY = (pieces[curType][0] & 0xF000) ? 0 : -1;

	return !Collide(curType, curRot, curX, curY);
}

// удаляет заполненные строки, возвращает их количество
static uint8_t ClearLines(void)
{
	uint8_t count = 0;
	uint8_t full;

	for (int y = BOARD_H - 1; y >= 0; y--) {
		full = 1;
		for (int x = 0; x < BOARD_W; x++) {
			if (!board[y][x]) {
				full = 0;
				break;
			}
		}
		if (!full) continue;

		// сдвигаем все что выше на строку вниз
		for (int yy = y; yy > 0; yy--) {
			for (int x = 0; x < BOARD_W; x++) {
				board[yy][x] = board[yy - 1][x];
			}
		}
		for (int x = 0; x < BOARD_W; x++) {
			board[0][x] = 0;
		}
		count++;
		y++; // эту же строку проверяем еще раз, туда съехала верхняя
	}
	return count;
}

// фигура легла на дно
static uint8_t LockPiece(void)
{
	uint16_t shape = pieces[curType][curRot];
	uint8_t n;

	for (int r = 0; r < 4; r++) {
		for (int c = 0; c < 4; c++) {
			if (PieceCell(shape, r, c) && curY + r >= 0) {
				board[curY + r][curX + c] = 1;
			}
		}
	}

	n = ClearLines();
	if (n > 0) {
		score += linePoints[n] * (level + 1);
		lines += n;
		level = lines / 10;
	}

	if (!NewPiece()) {
		state = GAME_OVER;
		return RES_OVER;
	}
	return n > 0 ? RES_LINES : RES_LOCK;
}

static void NewGame(void)
{
	for (int y = 0; y < BOARD_H; y++) {
		for (int x = 0; x < BOARD_W; x++) {
			board[y][x] = 0;
		}
	}
	score = 0;
	lines = 0;
	level = 0;
	srand(HAL_GetTick());
	nextType = rand() % 7;
	NewPiece();
	state = GAME_RUN;
}

static void Rotate(void)
{
	uint8_t r = (curRot + 1) % 4;
	// если поворот упирается в стенку, пробуем чуть сдвинуть фигуру
	static const int shift[5] = { 0, -1, 1, -2, 2 };

	for (int i = 0; i < 5; i++) {
		if (!Collide(curType, r, curX + shift[i], curY)) {
			curRot = r;
			curX += shift[i];
			return;
		}
	}
}

void Tetris_Init(void)
{
	state = GAME_START;
}

uint8_t Tetris_Key(char key)
{
	switch (state) {
	case GAME_START:
	case GAME_OVER:
		if (key == '#') {
			NewGame();
			return RES_MOVE;
		}
		break;

	case GAME_PAUSE:
		if (key == '*') {
			state = GAME_RUN;
			return RES_MOVE;
		}
		if (key == '#') {
			NewGame();
			return RES_MOVE;
		}
		break;

	case GAME_RUN:
		if (key == '4' && !Collide(curType, curRot, curX - 1, curY)) {
			curX--;
			return RES_MOVE;
		}
		if (key == '6' && !Collide(curType, curRot, curX + 1, curY)) {
			curX++;
			return RES_MOVE;
		}
		if (key == '2') {
			Rotate();
			return RES_MOVE;
		}
		if (key == '8') {
			// на одну клетку вниз, как будто фигура упала сама
			return Tetris_Fall();
		}
		if (key == '5') {
			// бросить фигуру до самого дна
			while (!Collide(curType, curRot, curX, curY + 1)) {
				curY++;
			}
			return LockPiece();
		}
		if (key == '*') {
			state = GAME_PAUSE;
			return RES_MOVE;
		}
		break;
	}
	return RES_NONE;
}

// падение фигуры на одну клетку (вызывается по программному таймеру)
uint8_t Tetris_Fall(void)
{
	if (state != GAME_RUN) {
		return RES_NONE;
	}

	if (!Collide(curType, curRot, curX, curY + 1)) {
		curY++;
		return RES_MOVE;
	}
	return LockPiece();
}

// рисует клетку стакана
static void DrawCell(int x, int y)
{
	int px = BOARD_X + x * CELL;
	int py = BOARD_Y + y * CELL;

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			if (spr_block3[i] & (0x04 >> j)) {
				oled_DrawPixel(px + j, py + i, White);
			}
		}
	}
}

// следующая фигура рисуется крупнее
static void DrawNext(void)
{
	uint16_t shape = pieces[nextType][0];

	for (int r = 0; r < 4; r++) {
		for (int c = 0; c < 4; c++) {
			if (!PieceCell(shape, r, c)) continue;
			for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 4; j++) {
					if (spr_block4[i] & (0x08 >> j)) {
						oled_DrawPixel(NEXT_X + c * 4 + j, NEXT_Y + r * 4 + i, White);
					}
				}
			}
		}
	}
}

static void DrawStartScreen(void)
{
	oled_SetCursor(31, 0);
	oled_WriteString("TETRIS", Font_11x18, White);
	oled_SetCursor(0, 21);
	oled_WriteString("4,6-move 2-rotate", Font_7x10, White);
	oled_SetCursor(0, 32);
	oled_WriteString("8-down 5-drop", Font_7x10, White);
	oled_SetCursor(0, 43);
	oled_WriteString("*-pause", Font_7x10, White);
	oled_SetCursor(0, 53);
	oled_WriteString("# - start", Font_7x10, White);
}

static void DrawOverScreen(void)
{
	char buf[20];

	oled_SetCursor(14, 0);
	oled_WriteString("GAME OVER", Font_11x18, White);
	sprintf(buf, "Score: %lu", score);
	oled_SetCursor(0, 22);
	oled_WriteString(buf, Font_7x10, White);
	sprintf(buf, "Lines: %d", lines);
	oled_SetCursor(0, 34);
	oled_WriteString(buf, Font_7x10, White);
	oled_SetCursor(0, 50);
	oled_WriteString("# - new game", Font_7x10, White);
}

static void DrawGame(void)
{
	char buf[20];
	uint16_t shape = pieces[curType][curRot];

	// стакан
	oled_DrawSquare(BOARD_X - 1, BOARD_X + BOARD_W * CELL, BOARD_Y - 1, BOARD_Y + BOARD_H * CELL, White);
	for (int y = 0; y < BOARD_H; y++) {
		for (int x = 0; x < BOARD_W; x++) {
			if (board[y][x]) DrawCell(x, y);
		}
	}

	// текущая фигура
	for (int r = 0; r < 4; r++) {
		for (int c = 0; c < 4; c++) {
			if (PieceCell(shape, r, c) && curY + r >= 0) {
				DrawCell(curX + c, curY + r);
			}
		}
	}

	// справа счет и следующая фигура
	oled_SetCursor(38, 0);
	oled_WriteString("Score", Font_7x10, White);
	sprintf(buf, "%lu", score);
	oled_SetCursor(38, 11);
	oled_WriteString(buf, Font_7x10, White);

	oled_SetCursor(38, 24);
	oled_WriteString("Lines", Font_7x10, White);
	sprintf(buf, "%d", lines);
	oled_SetCursor(38, 35);
	oled_WriteString(buf, Font_7x10, White);

	sprintf(buf, "Lvl %d", level);
	oled_SetCursor(38, 48);
	oled_WriteString(buf, Font_7x10, White);

	oled_SetCursor(92, 0);
	oled_WriteString("Next", Font_7x10, White);
	DrawNext();

	if (state == GAME_PAUSE) {
		oled_SetCursor(86, 48);
		oled_WriteString("PAUSE", Font_7x10, White);
	}
}

// рисует кадр в буфер дисплея
void Tetris_Draw(void)
{
	oled_Fill(Black);

	if (state == GAME_START) {
		DrawStartScreen();
	} else if (state == GAME_OVER) {
		DrawOverScreen();
	} else {
		DrawGame();
	}
}

uint8_t Tetris_GetState(void)
{
	return state;
}

// период падения фигуры в мс, растет скорость с уровнем
uint32_t Tetris_GetSpeed(void)
{
	int t = 800 - level * 70;
	if (t < 120) t = 120;
	return t;
}
