/* board.h - Board-specific hooks */

typedef enum {
    LED_ALIAS_INDICATOR,
    LED_ALIAS_BULB
} led_alias_t;

typedef enum {
    LED_STATE_ON,
    LED_STATE_OFF
} led_state_t;

void board_init(void);
void board_led_set(led_alias_t led, led_state_t new_state);
void board_led_get(led_alias_t led, led_state_t *old_state);
void board_led_trigger(led_alias_t led);
