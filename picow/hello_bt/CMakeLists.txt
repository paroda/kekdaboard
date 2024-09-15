#
## PERIPHERAL
# streamer server
add_executable(picow_ble_streamer
  gatt_streamer_server.c
)
target_link_libraries(picow_ble_streamer
  pico_stdlib
  pico_btstack_ble
  pico_btstack_cyw43
  pico_cyw43_arch_none
)
target_include_directories(picow_ble_streamer PRIVATE
  ${CMAKE_CURRENT_LIST_DIR} # For btstack config
)
pico_btstack_make_gatt_header(picow_ble_streamer PRIVATE "${CMAKE_CURRENT_LIST_DIR}/gatt_streamer_server.gatt")

pico_enable_stdio_usb(picow_ble_streamer 1)
pico_enable_stdio_uart(picow_ble_streamer 0)

pico_add_extra_outputs(picow_ble_streamer)


#
# CENTRAL
# streamer client
add_executable(picow_ble_reader
  le_streamer_client.c
)
target_link_libraries(picow_ble_reader
  pico_stdlib
  pico_btstack_ble
  pico_btstack_cyw43
  pico_cyw43_arch_none
)
target_include_directories(picow_ble_reader PRIVATE
  ${CMAKE_CURRENT_LIST_DIR} # For btstack config
)
target_compile_definitions(picow_ble_reader PRIVATE
  RUNNING_AS_CLIENT=1
)

pico_enable_stdio_usb(picow_ble_reader 1)
pico_enable_stdio_uart(picow_ble_reader 0)

pico_add_extra_outputs(picow_ble_reader)

#
## PERIPHERAL
# streamer server
add_executable(ble_server
  ble_server.c
  ble_test.c
)
target_link_libraries(ble_server
  pico_stdlib
  pico_btstack_ble
  pico_btstack_cyw43
  # pico_cyw43_arch_none
  pico_cyw43_arch_lwip_poll
)
target_include_directories(ble_server PRIVATE
  ${CMAKE_CURRENT_LIST_DIR} # For btstack config
)
pico_btstack_make_gatt_header(ble_server PRIVATE "${CMAKE_CURRENT_LIST_DIR}/ble_server.gatt")

pico_enable_stdio_usb(ble_server 1)
pico_enable_stdio_uart(ble_server 0)

pico_add_extra_outputs(ble_server)

#
# CENTRAL
# streamer client
add_executable(ble_client
  ble_client.c
  ble_test.c
)
target_link_libraries(ble_client
  pico_stdlib
  pico_btstack_ble
  pico_btstack_cyw43
  # pico_cyw43_arch_none
  pico_cyw43_arch_lwip_poll
)
target_include_directories(ble_client PRIVATE
  ${CMAKE_CURRENT_LIST_DIR} # For btstack config
)
target_compile_definitions(ble_client PRIVATE
  RUNNING_AS_CLIENT=1
)

pico_enable_stdio_usb(ble_client 1)
pico_enable_stdio_uart(ble_client 0)

pico_add_extra_outputs(ble_client)
