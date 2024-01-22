# ACCESS POINT

add_executable(picow_ap
  picow_access_point.c
  dhcpserver/dhcpserver.c
)

target_include_directories(picow_ap PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/dhcpserver
)

target_link_libraries(picow_ap
  pico_cyw43_arch_lwip_poll
  pico_stdlib
  pico_fota_bootloader_lib
)

target_link_options(picow_ap PRIVATE
  "-L${CMAKE_CURRENT_SOURCE_DIR}/../pico_fota_bootloader/linker_common"
)

pico_set_linker_script(picow_ap
  ${CMAKE_CURRENT_SOURCE_DIR}/../pico_fota_bootloader/linker_common/application.ld
)

pico_enable_stdio_usb(picow_ap 1)
pico_enable_stdio_uart(picow_ap 0)

pico_add_extra_outputs(picow_ap)


# CLIENT

add_executable(picow_client
  picow_client.c
)

target_include_directories(picow_client PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(picow_client
  pico_cyw43_arch_lwip_poll
  pico_stdlib
)

pico_enable_stdio_usb(picow_client 1)
pico_enable_stdio_uart(picow_client 0)

pico_add_extra_outputs(picow_client)
