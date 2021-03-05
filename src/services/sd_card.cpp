#define __SD_CARD__ 1
#include "sd_card.hpp"

#include "esp_vfs_fat.h"
#include "esp_idf_version.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

bool
SDCard::setup()
{
  ESP_LOGI(TAG, "Setup SD card");

  static const gpio_num_t PIN_NUM_MISO = GPIO_NUM_12;
  static const gpio_num_t PIN_NUM_MOSI = GPIO_NUM_13;
  static const gpio_num_t PIN_NUM_CLK  = GPIO_NUM_14;
  static const gpio_num_t PIN_NUM_CS   = GPIO_NUM_15;

  esp_err_t ret;
  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed = false,
    .max_files = 5,
    .allocation_unit_size = 16 * 1024
  };

  sdmmc_card_t* card;

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(4, 2, 0)

  sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();

  slot_config.gpio_miso = PIN_NUM_MISO;
  slot_config.gpio_mosi = PIN_NUM_MOSI;
  slot_config.gpio_sck  = PIN_NUM_CLK;
  slot_config.gpio_cs   = PIN_NUM_CS;

  ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

#else

  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();

  slot_config.gpio_cs = PIN_NUM_CS;

  spi_bus_config_t bus_cfg = {
    .mosi_io_num     = PIN_NUM_MOSI,
    .miso_io_num     = PIN_NUM_MISO,
    .sclk_io_num     = PIN_NUM_CLK,
    .quadwp_io_num   = GPIO_NUM_NC,
    .quadhd_io_num   = GPIO_NUM_NC,
    .max_transfer_sz = 0, // 0: Default size 4094
    .flags           = SPICOMMON_BUSFLAG_SLAVE,
    .intr_flags      = 0,
  };

  ret = spi_bus_initialize(HSPI_HOST, &bus_cfg, host.slot);
  if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to initialize SPI bus (%s).", esp_err_to_name(ret));
      return false;
  }

  ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);

#endif

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "Failed to mount filesystem.");
    } else {
      ESP_LOGE(TAG, "Failed to setup the SD card (%s).", esp_err_to_name(ret));
    }
    return false;
  }

  // Card has been initialized, print its properties
  sdmmc_card_print_info(stdout, card);

  return true;
}