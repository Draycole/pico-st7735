add_executable(demo main.c)
target_link_libraries(demo st7735)
pico_add_extra_outputs(demo)