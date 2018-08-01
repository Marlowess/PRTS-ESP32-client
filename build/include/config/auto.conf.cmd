deps_config := \
	/home/stefano/esp/esp-idf/components/app_trace/Kconfig \
	/home/stefano/esp/esp-idf/components/aws_iot/Kconfig \
	/home/stefano/esp/esp-idf/components/bt/Kconfig \
	/home/stefano/esp/esp-idf/components/driver/Kconfig \
	/home/stefano/esp/esp-idf/components/esp32/Kconfig \
	/home/stefano/esp/esp-idf/components/esp_adc_cal/Kconfig \
	/home/stefano/esp/esp-idf/components/ethernet/Kconfig \
	/home/stefano/esp/esp-idf/components/fatfs/Kconfig \
	/home/stefano/esp/esp-idf/components/freertos/Kconfig \
	/home/stefano/esp/esp-idf/components/heap/Kconfig \
	/home/stefano/esp/esp-idf/components/libsodium/Kconfig \
	/home/stefano/esp/esp-idf/components/log/Kconfig \
	/home/stefano/esp/esp-idf/components/lwip/Kconfig \
	/home/stefano/esp/esp-idf/components/mbedtls/Kconfig \
	/home/stefano/esp/esp-idf/components/openssl/Kconfig \
	/home/stefano/esp/esp-idf/components/pthread/Kconfig \
	/home/stefano/esp/esp-idf/components/spi_flash/Kconfig \
	/home/stefano/esp/esp-idf/components/spiffs/Kconfig \
	/home/stefano/esp/esp-idf/components/tcpip_adapter/Kconfig \
	/home/stefano/esp/esp-idf/components/wear_levelling/Kconfig \
	/home/stefano/esp/esp-idf/components/bootloader/Kconfig.projbuild \
	/home/stefano/esp/esp-idf/components/esptool_py/Kconfig.projbuild \
	/home/stefano/git/esp32_2/main/Kconfig.projbuild \
	/home/stefano/esp/esp-idf/components/partition_table/Kconfig.projbuild \
	/home/stefano/esp/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
