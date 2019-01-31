deps_config := \
	/Users/davidhorsley/esp/esp-idf/components/app_trace/Kconfig \
	/Users/davidhorsley/esp/esp-idf/components/aws_iot/Kconfig \
	/Users/davidhorsley/esp/esp-idf/components/bt/Kconfig \
	/Users/davidhorsley/esp/esp-idf/components/driver/Kconfig \
	/Users/davidhorsley/esp/esp-idf/components/esp32/Kconfig \
	/Users/davidhorsley/esp/esp-idf/components/esp_adc_cal/Kconfig \
	/Users/davidhorsley/esp/esp-idf/components/esp_http_client/Kconfig \
	/Users/davidhorsley/esp/esp-idf/components/ethernet/Kconfig \
	/Users/davidhorsley/esp/esp-idf/components/fatfs/Kconfig \
	/Users/davidhorsley/esp/esp-idf/components/freertos/Kconfig \
	/Users/davidhorsley/esp/esp-idf/components/heap/Kconfig \
	/Users/davidhorsley/esp/esp-idf/components/http_server/Kconfig \
	/Users/davidhorsley/esp/esp-idf/components/libsodium/Kconfig \
	/Users/davidhorsley/esp/esp-idf/components/log/Kconfig \
	/Users/davidhorsley/esp/esp-idf/components/lwip/Kconfig \
	/Users/davidhorsley/esp/esp-idf/components/mbedtls/Kconfig \
	/Users/davidhorsley/esp/esp-idf/components/mdns/Kconfig \
	/Users/davidhorsley/esp/esp-idf/components/openssl/Kconfig \
	/Users/davidhorsley/esp/esp-idf/components/pthread/Kconfig \
	/Users/davidhorsley/esp/esp-idf/components/spi_flash/Kconfig \
	/Users/davidhorsley/esp/esp-idf/components/spiffs/Kconfig \
	/Users/davidhorsley/esp/esp-idf/components/tcpip_adapter/Kconfig \
	/Users/davidhorsley/esp/esp-idf/components/vfs/Kconfig \
	/Users/davidhorsley/esp/esp-idf/components/wear_levelling/Kconfig \
	/Users/davidhorsley/esp/esp-idf/Kconfig.compiler \
	/Users/davidhorsley/esp/esp-idf/components/bootloader/Kconfig.projbuild \
	/Users/davidhorsley/esp/esp-idf/components/esptool_py/Kconfig.projbuild \
	/Users/davidhorsley/Documents/Uni/2018/IoT_Tour/Programming/esp-idf-feature-esp-ble-mesh-v0.5/examples/bluetooth/ble_mesh/ble_mesh_node_glowstick/main/Kconfig.projbuild \
	/Users/davidhorsley/esp/esp-idf/components/partition_table/Kconfig.projbuild \
	/Users/davidhorsley/esp/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
