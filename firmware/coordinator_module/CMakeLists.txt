cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Starter)

spiffs_create_partition_image(
    web_a
    "${CMAKE_CURRENT_SOURCE_DIR}/web"
    FLASH_IN_PROJECT)
