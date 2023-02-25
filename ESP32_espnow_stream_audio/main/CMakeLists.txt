cmake_minimum_required(VERSION 3.5)

idf_component_register(SRCS
                       "audio.c"
                       "main.c"
                       "sdRecording.c"
                       INCLUDE_DIRS ".")
