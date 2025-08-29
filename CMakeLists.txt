cmake_minimum_required(VERSION 3.12)

# initialize pico_sdk from GIT
# (note this can come from environment, CMake cache etc)
# set(PICO_SDK_FETCH_FROM_GIT on)

# pico_sdk_import.cmake is a single file copied from this SDK
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(pipistrelle-usb-microphone)

# initialize the Pico SDK
pico_sdk_init()

add_executable(pipistrelle-usb-microphone
	usb_descriptors.c
	usb_microphone.c
	main.cpp
	analog_microphone.cpp
        filter.cpp
)

target_include_directories(pipistrelle-usb-microphone PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src/include
)

target_link_libraries(pipistrelle-usb-microphone 
	pico_stdlib 
	pico_platform 
	pico_multicore 
	hardware_adc 
	hardware_dma 
	hardware_pwm 
	hardware_pio 
	tinyusb_device
	tinyusb_board
)

# enable usb output, disable uart output
pico_enable_stdio_usb(pipistrelle-usb-microphone  0)
pico_enable_stdio_uart(pipistrelle-usb-microphone 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(pipistrelle-usb-microphone)

# add url via pico_set_program_url
# example_auto_set_url(pipistrelle-usb-microphone)
