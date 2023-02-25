#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

export IDF_PATH=./esp-idf/

PROJECT_NAME := esp32_freertos_stream_audio

include $(IDF_PATH)/make/project.mk

format:
	find main -iname '*.h' -o -iname '*.c' | xargs clang-format -i
