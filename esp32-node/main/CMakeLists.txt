set(component_srcs "main.c")

idf_component_register(SRCS "${component_srcs}"
                            "u_interface.c"
                            "config.c"  
                            "nrf24.c"
                            "wifi_station.c"  
                           
                            "../../components/esp32-smbus/smbus.c"
                            "../../components/esp32-i2c-lcd1602/i2c-lcd1602.c"
                       INCLUDE_DIRS ".")
