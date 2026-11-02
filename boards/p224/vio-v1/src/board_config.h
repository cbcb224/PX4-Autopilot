/****************************************************************************
 *
 *   Copyright (c) 2022 PX4 Development Team. All rights reserved.
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

#pragma once
#include <px4_platform_common/px4_config.h>
#include <nuttx/compiler.h>
#include <stdint.h>
#include <stm32_gpio.h>

#define GPIO_nLED_RED (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_2MHz|GPIO_OUTPUT_SET|GPIO_PORTC|GPIO_PIN1)
#define GPIO_nLED_GREEN (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_2MHz|GPIO_OUTPUT_SET|GPIO_PORTA|GPIO_PIN0)
#define GPIO_nLED_BLUE (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_2MHz|GPIO_OUTPUT_SET|GPIO_PORTC|GPIO_PIN0)
#define BOARD_HAS_CONTROL_STATUS_LEDS 1
#define BOARD_OVERLOAD_LED LED_RED
#define BOARD_ARMED_STATE_LED LED_BLUE

#define PX4_ADC_GPIO GPIO_ADC12_INP18, GPIO_ADC12_INP4
#define ADC_BATTERY1_VOLTAGE_CHANNEL 18 /* PA4 */
#define ADC_BATTERY1_CURRENT_CHANNEL 4 /* PC4 */
#define ADC_CHANNELS ((1 << ADC_BATTERY1_VOLTAGE_CHANNEL) | (1 << ADC_BATTERY1_CURRENT_CHANNEL))
#define SYSTEM_ADC_BASE STM32_ADC1_BASE
#define BOARD_ADC_POS_REF_V 3.3f
#define BOARD_NUMBER_BRICKS 1
#define BOARD_ADC_BRICK1_VALID 1
#define BOARD_ADC_SERVO_VALID 1

#define DIRECT_PWM_OUTPUT_CHANNELS 9
#define BOARD_NUM_IO_TIMERS 4
#define HRT_TIMER 5
#define HRT_TIMER_CHANNEL 1
#define TONE_ALARM_TIMER 8
#define TONE_ALARM_CHANNEL 2
#define GPIO_TONE_ALARM_IDLE (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_2MHz|GPIO_OUTPUT_CLEAR|GPIO_PORTC|GPIO_PIN7)
#define GPIO_TONE_ALARM GPIO_TIM8_CH2OUT_1

/* PA9 is unconnected on this PCB. USB is intentionally treated as present. */
#define GPIO_OTGFS_VBUS (GPIO_INPUT|GPIO_PULLUP|GPIO_PORTA|GPIO_PIN9)
#define BOARD_ADC_USB_CONNECTED (px4_arch_gpioread(GPIO_OTGFS_VBUS))
#define BOARD_ADC_USB_VALID BOARD_ADC_USB_CONNECTED
#define SDIO_SLOTNO 0
#define SDIO_MINOR 0
#define BOARD_DMA_ALLOC_POOL_SIZE 5120
#define BOARD_HAS_ON_RESET 1
#define BOARD_ENABLE_CONSOLE_BUFFER
#define FLASH_BASED_PARAMS
#define PX4_GPIO_INIT_LIST { PX4_ADC_GPIO, GPIO_TONE_ALARM_IDLE }

__BEGIN_DECLS
#ifndef __ASSEMBLY__
int stm32_sdio_initialize(void);
void stm32_spiinitialize(void);
void stm32_usbinitialize(void);
void board_peripheral_reset(int ms);
#include <px4_platform_common/board_common.h>
#endif
__END_DECLS
