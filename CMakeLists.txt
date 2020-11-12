
if(CONFIG_PORTD)

zephyr_interface_library_named(PORTD)

target_include_directories(PORTD INTERFACE include)

zephyr_include_directories(include)

zephyr_library()
zephyr_library_sources(
    src/aspp.c
    src/common.c
    src/config_parser.c
    src/delimit.c
    src/portd.c
    src/mx_sio.c
    src/sysfakeconf.c
)

zephyr_library_link_libraries(PORTD)
target_link_libraries(PORTD INTERFACE zephyr_interface)

endif()
