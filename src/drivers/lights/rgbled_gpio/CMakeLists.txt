px4_add_module(
	MODULE drivers__rgbled_gpio
	MAIN rgbled_gpio
	SRCS rgbled_gpio.cpp
	DEPENDS led px4_work_queue
)
