set(component_srcs "main.c")

idf_component_register(SRCS "${component_srcs}"
                            "config.c"  
                            "nrf24.c"
                            "ui.c"
                            "../../components/esp32-smbus/smbus.c"
                            "../../components/esp32-i2c-lcd1602/i2c-lcd1602.c"
                       INCLUDE_DIRS ".")
