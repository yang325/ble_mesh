#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <logging/log.h>

#include "board.h"

LOG_MODULE_REGISTER(board);

#define LED_INDICATOR_NODE      DT_ALIAS(led0)
#define LED_INDICATOR_PIN		DT_GPIO_PIN(LED_INDICATOR_NODE, gpios)
#define LED_INDICATOR_PORT		DT_GPIO_LABEL(LED_INDICATOR_NODE, gpios)
#define LED_INDICATOR_FLAG      DT_GPIO_FLAGS(LED_INDICATOR_NODE, gpios)

#define LED_BULB_NODE           DT_ALIAS(led1)
#define LED_BULB_PIN			DT_GPIO_PIN(LED_BULB_NODE, gpios)
#define LED_BULB_PORT			DT_GPIO_LABEL(LED_BULB_NODE, gpios)
#define LED_BULB_FLAG           DT_GPIO_FLAGS(LED_BULB_NODE, gpios)

void board_init(void)
{
	int err;
    const struct device *dev;

	/* Set LED indicator pin as output */
	dev = device_get_binding(LED_INDICATOR_PORT);
	if (NULL == dev) {
		LOG_ERR("Failed to get binding %s", LED_INDICATOR_PORT);
		return;
	}
	err = gpio_pin_configure(dev, LED_INDICATOR_PIN, LED_INDICATOR_FLAG | GPIO_OUTPUT_LOW | GPIO_INPUT);
	if (err) {
		LOG_ERR("Failed to configure pin %s (err %d)", LED_INDICATOR_PORT, err);
		return;
	}

	/* Set LED bulb pin as output */
	dev = device_get_binding(LED_BULB_PORT);
	if (NULL == dev) {
		LOG_ERR("Failed to get binding %s", LED_BULB_PORT);
		return;
	}
	err = gpio_pin_configure(dev, LED_BULB_PIN, LED_BULB_FLAG | GPIO_OUTPUT_LOW | GPIO_INPUT);
	if (err) {
		LOG_ERR("Failed to configure pin %s (err %d)", LED_BULB_PORT, err);
		return;
	}
}

void board_led_set(led_alias_t led, led_state_t new_state)
{
	int err;
	const struct device *dev;
	const char * name;
	gpio_pin_t pin;

	switch (led) {
		case LED_ALIAS_INDICATOR:
			name = LED_INDICATOR_PORT;
			pin = LED_INDICATOR_PIN;
			break;
		case LED_ALIAS_BULB:
			name = LED_BULB_PORT;
			pin = LED_BULB_PIN;
			break;
		default:
			LOG_WRN("Unknown LED: %u", led);
			return;
	}

	dev = device_get_binding(name);
	if (NULL == dev) {
		LOG_ERR("Failed to get binding %s", name);
		return;
	}
	err = gpio_pin_set(dev, pin, (LED_STATE_ON == new_state) ? 1 : 0);
	if (err) {
		LOG_ERR("Failed to set pin %s (err %d)", name, err);
		return;
	}
}

led_state_t board_led_get(led_alias_t led)
{
	int err;
	const struct device *dev;
	const char * name;
	gpio_pin_t pin;

	switch (led) {
		case LED_ALIAS_INDICATOR:
			name = LED_INDICATOR_PORT;
			pin = LED_INDICATOR_PIN;
			break;
		case LED_ALIAS_BULB:
			name = LED_BULB_PORT;
			pin = LED_BULB_PIN;
			break;
		default:
			LOG_WRN("Unknown LED: %u", led);
			return LED_STATE_OFF;
	}

	dev = device_get_binding(name);
	if (NULL == dev) {
		LOG_ERR("Failed to get binding %s", name);
		return LED_STATE_OFF;
	}
	err = gpio_pin_get(dev, pin);
	if (err < 0) {
		LOG_ERR("Failed to get pin %s (err %d)", name, err);
		return LED_STATE_OFF;
	}

	return (1 == err) ? LED_STATE_ON : LED_STATE_OFF;
}

void board_led_trigger(led_alias_t led)
{
	led_state_t old_state;

	old_state = board_led_get(led);
	switch (old_state) {
		case LED_STATE_ON:
			LOG_DBG("Old state is ON");
			board_led_set(led, LED_STATE_OFF);
			break;
		case LED_STATE_OFF:
			LOG_DBG("Old state is OFF");
			board_led_set(led, LED_STATE_ON);
			break;
		default:
			LOG_WRN("Unknown state: %u", old_state);
			return;
	}
}
