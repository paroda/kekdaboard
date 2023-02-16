add_library(FatFs_SPI INTERFACE)

target_sources(FatFs_SPI INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/ff15/ffsystem.c
  ${CMAKE_CURRENT_LIST_DIR}/ff15/ffunicode.c
  ${CMAKE_CURRENT_LIST_DIR}/ff15/ff.c
  ${CMAKE_CURRENT_LIST_DIR}/sd_card.c
  ${CMAKE_CURRENT_LIST_DIR}/crc.c
  ${CMAKE_CURRENT_LIST_DIR}/glue.c
  ${CMAKE_CURRENT_LIST_DIR}/f_util.c
  ${CMAKE_CURRENT_LIST_DIR}/ff_stdio.c
  ${CMAKE_CURRENT_LIST_DIR}/rtc.c
)

target_link_libraries(FatFs_SPI INTERFACE
  hardware_spi
  hardware_rtc
  pico_stdlib
)
