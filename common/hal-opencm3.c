#include "hal.h"

#define SERIAL_BAUD 38400

#include <libopencm3/cm3/dwt.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#if defined(LM3S)
#include <libopencm3/lm3s/gpio.h>
#include <libopencm3/lm3s/usart.h>

#define SERIAL_USART USART0_BASE
#elif defined(STM32F215RET6)
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/rng.h>
/* TODO: Determine the right  */
#define SERIAL_GPIO GPIOA
#define SERIAL_USART USART1
#define SERIAL_PINS (GPIO9 | GPIO10)
#define STM32
#define CW_BOARD
#elif defined(STM32F207ZG)
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/rng.h>

#define SERIAL_GPIO GPIOA
#define SERIAL_USART USART1
#define SERIAL_PINS (GPIO9 | GPIO10)
#define STM32
#define NUCLEO_BOARD
#else
#error Unsupported libopencm3 board
#endif

#define _RCC_CAT(A, B) A ## _ ## B
#define RCC_ID(NAME) _RCC_CAT(RCC, NAME)

static uint32_t _clock_freq;

#ifdef STM32F2
extern uint32_t rcc_apb1_frequency;
extern uint32_t rcc_apb2_frequency;
#endif

static void clock_setup(enum clock_mode clock)
{
#if defined(LM3S)
  /* Nothing todo */
  /* rcc_clock_setup_in_xtal_8mhz_out_50mhz(); */
#elif defined(STM32F2)
  /* Some STM32 Platform */
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_USART1);
  rcc_periph_clock_enable(RCC_RNG);
  rcc_periph_clock_enable(RCC_GPIOH);
  /* All of them use an external oscillator with bypass. */
  rcc_osc_off(RCC_HSE);
  rcc_osc_bypass_enable(RCC_HSE);
  rcc_osc_on(RCC_HSE);
  rcc_wait_for_osc_ready(RCC_HSE);

#if defined(CW_BOARD)
  rcc_ahb_frequency = 7372800;
  rcc_apb1_frequency = 7372800;
  rcc_apb2_frequency = 7372800;
  _clock_freq = 7372800;
  rcc_set_hpre(RCC_CFGR_HPRE_DIV_NONE);
  rcc_set_ppre1(RCC_CFGR_PPRE_DIV_NONE);
  rcc_set_ppre2(RCC_CFGR_PPRE_DIV_NONE);
  rcc_set_sysclk_source(RCC_CFGR_SW_HSE);
  rcc_wait_for_sysclk_status(RCC_HSE);
#elif defined(NUCLEO_BOARD)
  /* NUCLEO-STM32F2 Board */
  switch (clock) {
  case CLOCK_BENCHMARK:
    rcc_ahb_frequency = 30000000;
    rcc_apb1_frequency = 30000000;
    rcc_apb2_frequency = 30000000;
    _clock_freq = 30000000;
    rcc_set_hpre(RCC_CFGR_HPRE_DIV_4);
    rcc_set_ppre1(RCC_CFGR_PPRE_DIV_NONE);
    rcc_set_ppre2(RCC_CFGR_PPRE_DIV_NONE);
    rcc_osc_off(RCC_PLL);
    /* Configure the PLL oscillator (use CUBEMX tool). */
    rcc_set_main_pll_hse(5, 200, 4, 4);
    /* Enable PLL oscillator and wait for it to stabilize. */
    rcc_osc_on(RCC_PLL);
    rcc_wait_for_osc_ready(RCC_PLL);
    flash_dcache_enable();
    flash_icache_enable();
    flash_set_ws(FLASH_ACR_LATENCY_0WS);
    flash_prefetch_enable();
    break;
  case CLOCK_FAST:
  default:
    rcc_ahb_frequency = 120000000;
    rcc_apb1_frequency = 30000000;
    rcc_apb2_frequency = 60000000;
    _clock_freq = 120000000;
    rcc_set_hpre(RCC_CFGR_HPRE_DIV_NONE);
    rcc_set_ppre1(RCC_CFGR_PPRE_DIV_4);
    rcc_set_ppre2(RCC_CFGR_PPRE_DIV_2);
    rcc_osc_off(RCC_PLL);
    /* Configure the PLL oscillator (use CUBEMX tool). */
    rcc_set_main_pll_hse(8, 240, 2, 5);
    /* Enable PLL oscillator and wait for it to stabilize. */
    rcc_osc_on(RCC_PLL);
    rcc_wait_for_osc_ready(RCC_PLL);
    flash_dcache_enable();
    flash_icache_enable();
    flash_set_ws(FLASH_ACR_LATENCY_3WS);
    flash_prefetch_enable();
    break;
  }
  rcc_set_sysclk_source(RCC_CFGR_SW_PLL);
  rcc_wait_for_sysclk_status(RCC_PLL);
#else
#error Unsupported STM32F2 Board
#endif
#else
#error Unsupported platform
#endif
}

void usart_setup()
{
#if defined(LM3S)
  /* Nothing todo for the simulator... */
#elif defined(STM32)
  /* The should be pretty much the same for all STM32 Boards */
  gpio_mode_setup(SERIAL_GPIO, GPIO_MODE_AF, GPIO_PUPD_PULLUP, SERIAL_PINS);
  gpio_set_output_options(SERIAL_GPIO, GPIO_OTYPE_OD, GPIO_OSPEED_100MHZ, SERIAL_PINS);
  gpio_set_af(SERIAL_GPIO, GPIO_AF7, SERIAL_PINS);
  usart_set_baudrate(SERIAL_USART, SERIAL_BAUD);
  usart_set_databits(SERIAL_USART, 8);
  usart_set_stopbits(SERIAL_USART, USART_STOPBITS_1);
  usart_set_mode(SERIAL_USART, USART_MODE_TX_RX);
  usart_set_parity(SERIAL_USART, USART_PARITY_NONE);
  usart_set_flow_control(SERIAL_USART, USART_FLOWCONTROL_NONE);
  usart_disable_rx_interrupt(SERIAL_USART);
  usart_disable_tx_interrupt(SERIAL_USART);
  usart_enable(SERIAL_USART);
#endif
}

void systick_setup()
{
  /* Systick is always the same on libopencm3 */
  systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
  systick_set_reload(0xFFFFFFu);
  systick_interrupt_enable();
  systick_counter_enable();
}

void hal_setup(const enum clock_mode clock)
{
  clock_setup(clock);
  usart_setup();
  systick_setup();
  rng_enable();
}

void hal_send_str(const char* in)
{
  const char* cur = in;
  while (*cur) {
    usart_send_blocking(SERIAL_USART, *cur);
    cur += 1;
  }
  usart_send_blocking(SERIAL_USART, '\n');
}

static volatile unsigned long long overflowcnt = 0;

void sys_tick_handler(void)
{
  ++overflowcnt;
}

uint64_t hal_get_time()
{
  while (true) {
    unsigned long long before = overflowcnt;
    unsigned long long result = (before + 1) * 16777216llu - systick_get_value();
    if (overflowcnt == before) {
      return result;
    }
  }
}
