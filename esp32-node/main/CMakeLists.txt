set(component_srcs "main.c")

idf_component_register(SRCS "${component_srcs}"
                            "nrf24.c"
                       INCLUDE_DIRS ".")
