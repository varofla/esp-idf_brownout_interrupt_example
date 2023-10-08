#include "esp_private/rtc_ctrl.h"
#include "esp_private/spi_flash_os.h"

#include "hal/brownout_hal.h"
#include "hal/brownout_ll.h"

#include "sdkconfig.h"

#if CONFIG_ESP_BROWNOUT_DET
#error "Do not enable \"Brownout Detector\" on SDK Configuration!!"
#endif

IRAM_ATTR static void s_brownout_handler(void *arg) {
  brownout_ll_intr_clear();

  // Stop the other core.
#if !CONFIG_ESP_SYSTEM_SINGLE_CORE_MODE
  const uint32_t core_id = esp_cpu_get_core_id();
  const uint32_t other_core_id = (core_id == 0) ? 1 : 0;
  esp_cpu_stall(other_core_id);
#endif

  ((void (*)(void))arg)();

  for (;;) {
    ;
  }
}

void brownout_with_interrupt_init(void (*brownout_isr)(void)) {
  brownout_hal_config_t cfg = {
      .threshold = 2, // 7~2 -> 2.51, 2.64, 2.76, 2.92, 3.10, 3.27
      .enabled = true,
      .reset_enabled = false,
      .flash_power_down = false,
      .rf_power_down = true,
  };

  brownout_hal_config(&cfg);
  brownout_ll_intr_clear();

#if CONFIG_IDF_TARGET_ESP32C6 || CONFIG_IDF_TARGET_ESP32H2
  esp_intr_alloc(ETS_LP_RTC_TIMER_INTR_SOURCE, ESP_INTR_FLAG_IRAM, s_brownout_handler, brownout_isr, NULL);
#else
  rtc_isr_register(s_brownout_handler, brownout_isr, RTC_CNTL_BROWN_OUT_INT_ENA_M, RTC_INTR_FLAG_IRAM);
#endif

  brownout_ll_intr_enable(true);
}

void brownout_without_interrupt_init(void) {
  brownout_hal_config_t cfg = {
      .threshold = 7,
      .enabled = true,
      .reset_enabled = true,
      .flash_power_down = true,
      .rf_power_down = true,
  };

  brownout_hal_config(&cfg);
}

void brownout_disable(void) {
  brownout_hal_config_t cfg = {
      .enabled = false,
  };

  brownout_hal_config(&cfg);
  brownout_ll_intr_enable(false);
  rtc_isr_deregister(s_brownout_handler, NULL);
}
