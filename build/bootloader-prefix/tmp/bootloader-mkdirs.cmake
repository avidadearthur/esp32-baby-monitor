# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/jeffeehsiung/esp/esp-idf/components/bootloader/subproject"
  "/Users/jeffeehsiung/Desktop/Esp32/espnow/build/bootloader"
  "/Users/jeffeehsiung/Desktop/Esp32/espnow/build/bootloader-prefix"
  "/Users/jeffeehsiung/Desktop/Esp32/espnow/build/bootloader-prefix/tmp"
  "/Users/jeffeehsiung/Desktop/Esp32/espnow/build/bootloader-prefix/src/bootloader-stamp"
  "/Users/jeffeehsiung/Desktop/Esp32/espnow/build/bootloader-prefix/src"
  "/Users/jeffeehsiung/Desktop/Esp32/espnow/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/jeffeehsiung/Desktop/Esp32/espnow/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/jeffeehsiung/Desktop/Esp32/espnow/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
