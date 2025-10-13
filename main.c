#include "main.h"
#include "input_processing.h"
#include "input_reading.h"

/*==================== DEFINE ====================*/
#define TICK_MS         10
#define COUNT_PER_SEC   (1000 / TICK_MS)
#define BLINK_PERIOD_MS 500

// Traffic states
#define S_RED_GREEN     0
#define S_RED_YELLOW    1
#define S_GREEN_RED     2
#define S_YELLOW_RED    3

// 7-seg enable pins
#define SEG_EN0 GPIO_PIN_0
#define SEG_EN1 GPIO_PIN_1
#define SEG_EN2 GPIO_PIN_2
#define SEG_EN3 GPIO_PIN_3
#define SEG_PORT GPIOA

/*==================== GLOBAL VARIABLES ====================*/
int status = S_GREEN_RED;
int rem_main = 0, rem_sub = 0;
int blink_ms = 0, blink_state = 0;
uint8_t digits[4] = {0,0,0,0};

static GPIO_PinState btn_db1[N0_OF_BUTTONS], btn_db2[N0_OF_BUTTONS];
static GPIO_PinState btn_state[N0_OF_BUTTONS];
static uint8_t btn_flag[N0_OF_BUTTONS];
static uint16_t btn_cnt_long[N0_OF_BUTTONS];
static uint8_t btn_flag_1s[N0_OF_BUTTONS];

int mode = 1;
int red_duration;
int yellow_duration;
int green_duration;

static void MX_GPIO_Init(void);
void SystemClock_Config(void);

/*==================== BUTTON FUNCTIONS ====================*/
void read_buttons(void) {
    for (int i = 0; i < N0_OF_BUTTONS; i++) {
        btn_db2[i] = btn_db1[i];
        btn_db1[i] = HAL_GPIO_ReadPin(GPIOB,
                        (i==0 ? GPIO_PIN_13 : (i==1 ? GPIO_PIN_14 : GPIO_PIN_15)));
        if (btn_db1[i] == btn_db2[i]) {
            if (btn_state[i] != btn_db1[i]) {
                btn_state[i] = btn_db1[i];
                if (btn_state[i] == BUTTON_IS_PRESSED) btn_flag[i] = 1;
            }
            if (btn_state[i] == BUTTON_IS_PRESSED) {
                if (btn_cnt_long[i] < 0xFFFF) btn_cnt_long[i]++;
                if (btn_cnt_long[i] == COUNT_PER_SEC) btn_flag_1s[i] = 1;
            } else {
                btn_cnt_long[i] = 0;
                btn_flag_1s[i] = 0;
            }
        }
    }
}

unsigned char is_button_pressed(unsigned char index) {
    if (index >= N0_OF_BUTTONS) return 0;
    if (btn_flag[index]) { btn_flag[index] = 0; return 1; }
    return 0;
}

unsigned char is_button_pressed_1s(unsigned char index) {
    if (index >= N0_OF_BUTTONS) return 0;
    if (btn_flag_1s[index]) { btn_flag_1s[index] = 0; return 1; }
    return 0;
}

GPIO_PinState get_button_state(unsigned char index) {
    if (index >= N0_OF_BUTTONS) return BUTTON_IS_RELEASED;
    return btn_state[index];
}

/*==================== LED CONTROL ====================*/
void clear_all_leds(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|
                             GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13, GPIO_PIN_RESET);
}

void set_main_red(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9|GPIO_PIN_10, GPIO_PIN_RESET);
}

void set_main_yellow(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8|GPIO_PIN_10, GPIO_PIN_RESET);
}

void set_main_green(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8|GPIO_PIN_9, GPIO_PIN_RESET);
}

void set_sub_red(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12|GPIO_PIN_13, GPIO_PIN_RESET);
}

void set_sub_yellow(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11|GPIO_PIN_13, GPIO_PIN_RESET);
}

void set_sub_green(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_13, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11|GPIO_PIN_12, GPIO_PIN_RESET);
}

/*==================== TRAFFIC CONTROL ====================*/
void start_status_counters(void) {
    if (status == S_GREEN_RED) {
        rem_main = green_duration * COUNT_PER_SEC;
        rem_sub  = red_duration * COUNT_PER_SEC;
    } else if (status == S_YELLOW_RED) {
        rem_main = yellow_duration * COUNT_PER_SEC;
        rem_sub  = red_duration * COUNT_PER_SEC - green_duration * COUNT_PER_SEC;
        if (rem_sub < 0) rem_sub = yellow_duration * COUNT_PER_SEC;
    } else if (status == S_RED_GREEN) {
        rem_main = red_duration * COUNT_PER_SEC;
        rem_sub  = green_duration * COUNT_PER_SEC;
    } else if (status == S_RED_YELLOW) {
        rem_sub = yellow_duration * COUNT_PER_SEC;
        rem_main = red_duration * COUNT_PER_SEC - green_duration * COUNT_PER_SEC;
        if (rem_main < 0) rem_main = yellow_duration * COUNT_PER_SEC;
    }
}

/*==================== 7-SEGMENT DISPLAY ====================*/
const uint8_t seg_pattern[10] = {
    0xC0, 0xF9, 0xA4, 0xB0, 0x99,
    0x92, 0x82, 0xF8, 0x80, 0x90
};

void display7SEG(uint8_t num) {
    for (int i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(GPIOB, (1 << i), ((seg_pattern[num] >> i) & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}



void update7SEG(int index) {
    HAL_GPIO_WritePin(SEG_PORT, SEG_EN0|SEG_EN1|SEG_EN2|SEG_EN3, GPIO_PIN_SET);
    switch(index) {
        case 0: HAL_GPIO_WritePin(SEG_PORT, SEG_EN0, GPIO_PIN_RESET); display7SEG(digits[0]); break;
        case 1: HAL_GPIO_WritePin(SEG_PORT, SEG_EN1, GPIO_PIN_RESET); display7SEG(digits[1]); break;
        case 2: HAL_GPIO_WritePin(SEG_PORT, SEG_EN2, GPIO_PIN_RESET); display7SEG(digits[2]); break;
        case 3: HAL_GPIO_WritePin(SEG_PORT, SEG_EN3, GPIO_PIN_RESET); display7SEG(digits[3]); break;
    }
}

void update_display_values_normal(void) {
    int s_main = (rem_main + COUNT_PER_SEC - 1) / COUNT_PER_SEC;
    int s_sub = (rem_sub + COUNT_PER_SEC - 1) / COUNT_PER_SEC;
    if (s_main < 0) s_main = 0;
    if (s_sub < 0) s_sub = 0;
    digits[0] = (s_main / 10) % 10;
    digits[1] = s_main % 10;
    digits[2] = (s_sub / 10) % 10;
    digits[3] = s_sub % 10;
}

/*==================== MAIN ====================*/
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    status = S_GREEN_RED;
    start_status_counters();

    uint32_t last_tick = HAL_GetTick();
    static int prev_mode = 1;
    static int led_index = 0;
    if (red_duration == 0 && yellow_duration == 0 && green_duration == 0) {
            red_duration = 5;
            yellow_duration = 2;
            green_duration = 3;
        }
    while (1) {
        uint32_t now = HAL_GetTick();
        if ((now - last_tick) >= TICK_MS) {
            last_tick += TICK_MS;

            read_buttons();
            fsm_for_input_processing();

            /* ----- Normal Mode ----- */
            if (mode == 1) {
                if (prev_mode != 1) {
                    clear_all_leds();
                    prev_mode = 1;
                }

                if (rem_main > 0) rem_main--;
                if (rem_sub > 0) rem_sub--;

                switch (status) {
                    case S_GREEN_RED:
                        if (rem_main <= 0) {
                            status = S_YELLOW_RED;
                            rem_main = yellow_duration * COUNT_PER_SEC;
                        }
                        set_main_green();
                        set_sub_red();
                        break;

                    case S_YELLOW_RED:
                        if (rem_main <= 0) {
                            status = S_RED_GREEN;
                            rem_main = red_duration * COUNT_PER_SEC;
                            rem_sub = green_duration * COUNT_PER_SEC;
                        }
                        set_main_yellow();
                        set_sub_red();
                        break;

                    case S_RED_GREEN:
                        if (rem_sub <= 0) {
                            status = S_RED_YELLOW;
                            rem_sub = yellow_duration * COUNT_PER_SEC;
                        }
                        set_main_red();
                        set_sub_green();
                        break;

                    case S_RED_YELLOW:
                        if (rem_sub <= 0) {
                            status = S_GREEN_RED;
                            rem_main = green_duration * COUNT_PER_SEC;
                            rem_sub = red_duration * COUNT_PER_SEC;
                        }
                        set_main_red();
                        set_sub_yellow();
                        break;
                }

                update_display_values_normal();
            }

            /* ----- Setting Modes (2,3,4) ----- */
            else {
                blink_ms += TICK_MS;
                if (blink_ms >= BLINK_PERIOD_MS) {
                    blink_ms = 0;
                    blink_state = !blink_state;
                }

                if (prev_mode != mode) {
                    clear_all_leds();
                    prev_mode = mode;
                }

                if (mode == 2) {
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8|GPIO_PIN_11, blink_state);
                }
                else if (mode == 3) {
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9|GPIO_PIN_12, blink_state);
                }
                else if (mode == 4) {
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10|GPIO_PIN_13, blink_state);
                }

                if (is_button_pressed(1)) {
                    if (mode == 2) red_duration = (red_duration % 99) + 1;
                    else if (mode == 3) yellow_duration = (yellow_duration % 99) + 1;
                    else if (mode == 4) green_duration = (green_duration % 99) + 1;
                }

                if (is_button_pressed(2)) {
                    mode = 1;
                    clear_all_leds();

                    if (prev_mode == 2) {
                        status = S_RED_GREEN;
                        rem_main = red_duration * COUNT_PER_SEC;
                        rem_sub  = green_duration * COUNT_PER_SEC;
                        set_main_red();
                        set_sub_green();
                    }
                    else if (prev_mode == 3) {
                        status = S_YELLOW_RED;
                        rem_main = yellow_duration * COUNT_PER_SEC;
                        rem_sub  = red_duration * COUNT_PER_SEC;
                        set_main_yellow();
                        set_sub_red();
                    }
                    else if (prev_mode == 4) {
                        status = S_GREEN_RED;
                        rem_main = green_duration * COUNT_PER_SEC;
                        rem_sub  = red_duration * COUNT_PER_SEC;
                        set_main_green();
                        set_sub_red();
                    }
                    else {
                        status = S_GREEN_RED;
                        rem_main = green_duration * COUNT_PER_SEC;
                        rem_sub  = red_duration * COUNT_PER_SEC;
                        set_main_green();
                        set_sub_red();
                    }

                    update_display_values_normal();
                    prev_mode = 1;
                }


                int value = 0;
                if (mode == 2) value = red_duration;
                else if (mode == 3) value = yellow_duration;
                else if (mode == 4) value = green_duration;

                digits[0] = 0;
                digits[1] = mode;
                digits[2] = (value / 10) % 10;
                digits[3] = value % 10;
            }

            /* ----- Update 7-segment ----- */
            static int led_index = 0;
            static int seg_refresh = 0; // đếm tick 10ms

            seg_refresh++;
            if (seg_refresh >= 25) { // 5 x 10ms = 50ms
                seg_refresh = 0;
                led_index = (led_index + 1) % 4;
                update7SEG(led_index);
            }

        }
    }
}

/*==================== SYSTEM CONFIG ====================*/
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|
                                  RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}

static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* 7SEG: PB0-PB7 (segments a–g + dp) */
    HAL_GPIO_WritePin(GPIOB, 0xFF, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = 0xFF;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* LEDs (traffic lights) + 7SEG_EN pins */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|
                             GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, SEG_EN0|SEG_EN1|SEG_EN2|SEG_EN3, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|
                          GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|
                          SEG_EN0|SEG_EN1|SEG_EN2|SEG_EN3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Buttons PB13, PB14, PB15 */
    GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void Error_Handler(void) {
    __disable_irq();
    while (1) {}
}
