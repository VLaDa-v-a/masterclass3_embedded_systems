#ifndef INC_BUZZER_H_
#define INC_BUZZER_H_

#include "main.h"

// излучатель звука подключен к PE9 (TIM1_CH1)
// чтобы звук был слышен, на стенде надо замкнуть 7-й контакт переключателя SA2

void Buzzer_On(uint32_t freq);
void Buzzer_Off(void);

#endif /* INC_BUZZER_H_ */
