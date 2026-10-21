#include "buzzer.h"
#include "tim.h"

// TIM1 настроен в CubeMX: тактирование 168 МГц, предделитель 167 -> счет идет с частотой 1 МГц
// поэтому период для нужной частоты = 1000000 / freq

void Buzzer_On(uint32_t freq)
{
	uint32_t arr;

	if (freq == 0) {
		Buzzer_Off();
		return;
	}

	arr = 1000000 / freq;
	if (arr < 2) {
		arr = 2;
	}

	__HAL_TIM_SET_AUTORELOAD(&htim1, arr - 1);
	// скважность 50% - громкость максимальная
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, arr / 2);
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
}

void Buzzer_Off(void)
{
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
}
