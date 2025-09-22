cmake_minimum_required(VERSION 3.12)

# initialize pico_sdk from GIT
# (note this can come from environment, CMake cache etc)
# set(PICO_SDK_FETCH_FROM_GIT on)

# pico_sdk_import.cmake is a single file copied from this SDK
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(pipistrelle-microphone)

# initialize the Pico SDK
pico_sdk_init()


add_executable(analog-pipistrelle-usb-microphone
	usb_descriptors.c
	usb_microphone.c
	main.cpp
        filter.cpp
	microphone.cpp
	analog_microphone.cpp
)

target_compile_definitions(analog-pipistrelle-usb-microphone PRIVATE MICROPHONE=ANALOG)

add_executable(pdm-pipistrelle-usb-microphone
	usb_descriptors.c
	usb_microphone.c
	main.cpp
        filter.cpp
	microphone.cpp
        pdm_receiver.c
        pdm_receiver.pio
	pdm_microphone.cpp
)
pico_generate_pio_header(pdm-pipistrelle-usb-microphone ${CMAKE_CURRENT_LIST_DIR}/pdm_receiver.pio)

target_compile_definitions(pdm-pipistrelle-usb-microphone PRIVATE MICROPHONE=PDM)

foreach(target analog-pipistrelle-usb-microphone pdm-pipistrelle-usb-microphone)

target_include_directories(${target} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src/include
)

target_link_libraries(${target}
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
# pico_enable_stdio_usb(${target} 0)
# pico_enable_stdio_uart(${target} 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(${target})
endforeach()

# add url via pico_set_program_url
# example_auto_set_url(pipistrelle-usb-microphone)
