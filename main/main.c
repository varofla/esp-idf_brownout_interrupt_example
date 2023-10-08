#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "driver/gpio.h"

#define TAG "brownout_example"

void brownout_with_interrupt_init(void (*brownout_isr)());

IRAM_ATTR static void brownout_isr() {
  int8_t read_buff = 0;
  nvs_handle flash_hd;

  nvs_open("storage", NVS_READWRITE, &flash_hd);

  nvs_get_i8(flash_hd, "brownout_det", &read_buff);
  read_buff = !read_buff;
  nvs_set_i8(flash_hd, "brownout_det", read_buff);

  nvs_close(flash_hd);

  while (1) {
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

static bool s_read_nvs(void) {
  int8_t read_buff = 0; // value will default to 0, if not set yet in NVS
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  nvs_handle flash_hd;
  err = nvs_open("storage", NVS_READWRITE, &flash_hd);
  ESP_ERROR_CHECK(err);

  if (err != ESP_OK) {
    return false;
  } else {
    err = nvs_get_i8(flash_hd, "brownout_det", &read_buff);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
      nvs_set_i8(flash_hd, "brownout_det", 0);
    } else if (err != ESP_OK) {
      ESP_ERROR_CHECK(err);
      return false;
    }
    nvs_close(flash_hd);
  }

  ESP_LOGI(TAG, "brownout toggle = %d\n", read_buff);
  return read_buff == 1;
}

static void s_init_led(void) {
  gpio_reset_pin(12);
  gpio_set_direction(12, GPIO_MODE_OUTPUT);
}

static void s_set_led(bool val) {
  gpio_set_level(12, val);
}

void app_main() {
  brownout_with_interrupt_init(brownout_isr);

  bool ret = s_read_nvs();

  s_init_led();
  s_set_led(ret);

  for (;;) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
