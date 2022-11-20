add_executable(hello_shift_register
  main.c
)

# pull in common dependencies and additional hardware support
target_link_libraries(hello_shift_register pico_stdlib)

# create map/bin/hex file etc.
pico_add_extra_outputs(hello_shift_register)
