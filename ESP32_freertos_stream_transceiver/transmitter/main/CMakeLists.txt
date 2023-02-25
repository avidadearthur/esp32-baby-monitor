cmake_minimum_required(VERSION 3.5)

set(component_srcs "main.c")

idf_component_register(SRCS
                    "${component_srcs}"
                    "audio.c"
                    "main.c"
                    INCLUDE_DIRS ".")
