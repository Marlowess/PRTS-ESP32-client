#include "../include/controller.h"


#ifdef __cplusplus
  extern "C" {
#endif
void app_main(){
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Creation of two tasks */
    tasksCreation();
}
#ifdef __cplusplus
  }
#endif










