#include "../include/controller.h"
#include "../include/my_mdns.h"


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

    /*
     * This function creates three tasks:
     * 	1) task to exchange timestamp with server
     * 	2) task to sniff wifi traffic
     * 	3) task to exchange wifi data with server
     *  */
    tasksCreation();
}
#ifdef __cplusplus
  }
#endif










