/* SPDX-License-Identifier: BSD-3-Clause */

#include <px4_platform_common/px4_config.h>
#include <px4_platform_common/module.h>
#include <px4_platform_common/px4_work_queue/ScheduledWorkItem.hpp>
#include <lib/led/led.h>
#include <board_config.h>

#if !defined(GPIO_nLED_RED) || !defined(GPIO_nLED_GREEN) || !defined(GPIO_nLED_BLUE)
#error "rgbled_gpio requires three active-low board LED GPIO definitions"
#endif

class RGBLED_GPIO : public ModuleBase<RGBLED_GPIO>, public px4::ScheduledWorkItem
{
public:
	RGBLED_GPIO() : ScheduledWorkItem(MODULE_NAME, px4::wq_configurations::lp_default) {}
	~RGBLED_GPIO() override { set_color(led_control_s::COLOR_OFF, 0); }

	static int task_spawn(int argc, char *argv[]);
	static int custom_command(int argc, char *argv[]) { return print_usage("unknown command"); }
	static int print_usage(const char *reason = nullptr);
	int print_status() override;

private:
	void Run() override;
	void set_color(uint8_t color, uint8_t brightness);
	LedController _controller;
	uint8_t _color{led_control_s::COLOR_OFF};
	uint8_t _brightness{0};
};

int RGBLED_GPIO::task_spawn(int argc, char *argv[])
{
	RGBLED_GPIO *instance = new RGBLED_GPIO();

	if (instance == nullptr) {
		PX4_ERR("allocation failed");
		return PX4_ERROR;
	}

	_object.store(instance);
	_task_id = task_id_is_work_queue;
	px4_arch_configgpio(GPIO_nLED_RED);
	px4_arch_configgpio(GPIO_nLED_GREEN);
	px4_arch_configgpio(GPIO_nLED_BLUE);
	instance->set_color(led_control_s::COLOR_OFF, 0);
	instance->ScheduleNow();
	return PX4_OK;
}

void RGBLED_GPIO::set_color(uint8_t color, uint8_t brightness)
{
	bool red = false;
	bool green = false;
	bool blue = false;

	if (brightness > 0) {
		switch (color) {
		case led_control_s::COLOR_RED: red = true; break;
		case led_control_s::COLOR_GREEN: green = true; break;
		case led_control_s::COLOR_BLUE: blue = true; break;
		case led_control_s::COLOR_YELLOW:
		case led_control_s::COLOR_AMBER: red = green = true; break;
		case led_control_s::COLOR_PURPLE: red = blue = true; break;
		case led_control_s::COLOR_CYAN: green = blue = true; break;
		case led_control_s::COLOR_WHITE: red = green = blue = true; break;
		default: break;
		}
	}

	// GPIO output is on/off; it does not provide PWM brightness control.
	px4_arch_gpiowrite(GPIO_nLED_RED, !red);
	px4_arch_gpiowrite(GPIO_nLED_GREEN, !green);
	px4_arch_gpiowrite(GPIO_nLED_BLUE, !blue);
	_color = color;
	_brightness = brightness;
}

void RGBLED_GPIO::Run()
{
	if (should_exit()) {
		ScheduleClear();
		set_color(led_control_s::COLOR_OFF, 0);
		exit_and_cleanup();
		return;
	}

	LedControlData data;

	if (_controller.update(data) == 1) {
		set_color(data.leds[0].color, data.leds[0].brightness);
	}

	ScheduleDelayed(_controller.maximum_update_interval());
}

int RGBLED_GPIO::print_status()
{
	PX4_INFO("active-low GPIO RGB, color=%u brightness=%u (on/off only)",
		 (unsigned)_color, (unsigned)_brightness);
	return PX4_OK;
}

int RGBLED_GPIO::print_usage(const char *reason)
{
	if (reason != nullptr) {
		PX4_WARN("%s", reason);
	}

	PRINT_MODULE_DESCRIPTION("Drive one active-low GPIO RGB LED from led_control. No PWM dimming. Use led_control for color tests.");
	PRINT_MODULE_USAGE_NAME("rgbled_gpio", "driver");
	PRINT_MODULE_USAGE_COMMAND("start");
	PRINT_MODULE_USAGE_DEFAULT_COMMANDS();
	return PX4_OK;
}

extern "C" __EXPORT int rgbled_gpio_main(int argc, char *argv[])
{
	return RGBLED_GPIO::main(argc, argv);
}
