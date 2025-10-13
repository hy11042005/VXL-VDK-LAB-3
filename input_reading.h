#ifndef INC_INPUT_READING_H_
#define INC_INPUT_READING_H_

#include "main.h"

#define N0_OF_BUTTONS 3
#define BUTTON_IS_PRESSED GPIO_PIN_RESET
#define BUTTON_IS_RELEASED GPIO_PIN_SET

unsigned char is_button_pressed(unsigned char index);
void read_buttons(void);
GPIO_PinState get_button_state(unsigned char index); // Thêm prototype

#endif /* INC_INPUT_READING_H_ */
