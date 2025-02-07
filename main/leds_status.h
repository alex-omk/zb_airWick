#include "led_strip.h"

// extern  uint8_t s_led_state;

extern TaskHandle_t ledTaskHandle;

void configure_led(void);
void blink_led(void);
void led_task(void *pvParameters);