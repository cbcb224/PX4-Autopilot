/****************************************************************************
 *
 *   Copyright (c) 2012-2022 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#include "board_config.h"
#include <stdbool.h>
#include <syslog.h>
#include <nuttx/board.h>
#include <arch/board/board.h>
#include "arm_internal.h"
#include <drivers/drv_board_led.h>
#include <systemlib/px4_macros.h>
#include <px4_arch/io_timer.h>
#include <px4_platform_common/init.h>
#include <px4_platform/gpio.h>
#include <px4_platform/board_dma_alloc.h>
#if !defined(BOOTLOADER)
#include <parameters/flashparams/flashfs.h>
#endif

__BEGIN_DECLS
void led_on(int led);
void led_off(int led);
__END_DECLS

__EXPORT void board_peripheral_reset(int ms)
{
	// Sensors have no switched power rail on this PCB.
	usleep(ms * 1000);
}

__EXPORT void board_on_reset(int status)
{
	for (int i = 0; i < DIRECT_PWM_OUTPUT_CHANNELS; ++i) {
		px4_arch_configgpio(io_timer_channel_get_gpio_output(i));
	}
	if (status >= 0) {
		up_mdelay(100);
	}
}

__EXPORT void stm32_boardinitialize(void)
{
	board_on_reset(-1);
	board_autoled_initialize();
	const uint32_t gpio[] = PX4_GPIO_INIT_LIST;
	px4_gpio_init(gpio, arraySize(gpio));
	stm32_usbinitialize();
}

__EXPORT int board_app_initialize(uintptr_t arg)
{
#if !defined(BOOTLOADER)
	px4_platform_init();
	stm32_spiinitialize();
	board_spi_reset(10, 0xffff);
	if (board_dma_alloc_init() < 0) {
		syslog(LOG_ERR, "[boot] DMA alloc FAILED\n");
	}
	drv_led_start();
	led_off(LED_RED);
	led_off(LED_BLUE);
	led_off(LED_GREEN);
	if (board_hardfault_init(2, true) != 0) {
		led_on(LED_RED);
	}
	// Sector 15 is excluded from both the application link region and BL erase range.
	static sector_descriptor_t params_sector_map[] = {
		{15, 128 * 1024, 0x081E0000},
		{0, 0, 0},
	};
	if (parameter_flashfs_init(params_sector_map, NULL, 0) != OK) {
		syslog(LOG_ERR, "[boot] parameter Flash init FAILED\n");
		led_on(LED_RED);
	}
#ifdef CONFIG_MMCSD
	if (stm32_sdio_initialize() != OK) {
		led_on(LED_RED);
	}
#endif
	px4_platform_configure();
#endif
	return OK;
}
