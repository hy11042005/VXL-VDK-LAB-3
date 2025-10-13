#include "input_processing.h"
#include "input_reading.h"

/*==================== DEFINE ====================*/
#define MODE_NORMAL  1
#define MODE_RED     2
#define MODE_YELLOW  3
#define MODE_GREEN   4

/*==================== EXTERN VARIABLES ====================*/
extern int mode;
extern int red_duration;
extern int yellow_duration;
extern int green_duration;

/*==================== FUNCTION ====================*/
void fsm_for_input_processing(void) {
    // Button 0: cycle through modes
    if (is_button_pressed(0)) {
        mode++;
        if (mode > MODE_GREEN) mode = MODE_NORMAL;
    }

    // Note:
    // Button 1 (increase) and Button 2 (confirm/next)
    // are now handled directly in main.c for better timing and clarity.
    // We no longer modify durations here — avoids flag conflicts.
}
