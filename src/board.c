#include <device.h>
#include <gpio.h>
#include <logging/log.h>

#include "board.h"

LOG_MODULE_REGISTER(board);

#define LED_INDICATOR_PIN		LED0_GPIO_PIN
#define LED_INDICATOR_PORT		LED0_GPIO_CONTROLLER
#define LED_BULB_PIN			LED1_GPIO_PIN
#define LED_BULB_PORT			LED1_GPIO_CONTROLLER

void board_init(void)
{
    struct device *dev;

    dev = device_get_binding(LED_BULB_PORT);
	/* Set LED pin as output */
	gpio_pin_configure(dev, LED_BULB_PIN, GPIO_DIR_OUT);
	gpio_pin_write(dev, LED_BULB_PIN, 1);

	dev = device_get_binding(LED_INDICATOR_PORT);
	/* Set LED pin as output */
	gpio_pin_configure(dev, LED_INDICATOR_PIN, GPIO_DIR_OUT);
	gpio_pin_write(dev, LED_INDICATOR_PIN, 1);
}

void board_led_set(led_alias_t led, led_state_t new_state)
{
	struct device *dev;

	switch (led) {
		case LED_ALIAS_INDICATOR:
			dev = device_get_binding(LED_INDICATOR_PORT);
			gpio_pin_write(dev, LED_INDICATOR_PIN, LED_STATE_OFF == new_state);
			break;
		case LED_ALIAS_BULB:
			dev = device_get_binding(LED_BULB_PORT);
			gpio_pin_write(dev, LED_BULB_PIN, LED_STATE_OFF == new_state);
			break;
		default:
			LOG_WRN("Unknown LED: %u", led);
			return;
	}
}

void board_led_get(led_alias_t led, led_state_t *old_state)
{
	struct device *dev;
	u32_t value;

	switch (led) {
		case LED_ALIAS_INDICATOR:
			dev = device_get_binding(LED_INDICATOR_PORT);
			gpio_pin_read(dev, LED_INDICATOR_PIN, &value);
			break;
		case LED_ALIAS_BULB:
			dev = device_get_binding(LED_BULB_PORT);
			gpio_pin_read(dev, LED_BULB_PIN, &value);
			break;
		default:
			LOG_WRN("Unknown LED: %u", led);
			return;
	}

	*old_state = (0 == value) ? LED_STATE_ON : LED_STATE_OFF;
}

void board_led_trigger(led_alias_t led)
{
	led_state_t old_state;

	board_led_get(led, &old_state);
	switch (old_state) {
		case LED_STATE_ON:
			board_led_set(led, LED_STATE_OFF);
			break;
		case LED_STATE_OFF:
			board_led_set(led, LED_STATE_ON);
			break;
		default:
			LOG_WRN("Unknown state: %u", old_state);
			return;
	}
}
