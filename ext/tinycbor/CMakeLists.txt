add_library(TINYCBOR INTERFACE)

target_include_directories(TINYCBOR INTERFACE src)

zephyr_library()
zephyr_library_sources(
    src/cborencoder.c
    src/cborencoder_close_container_checked.c
    src/cborerrorstrings.c
    src/cborparser.c
    src/cborparser_dup_string.c
    #src/open_memstream.c
)

zephyr_library_link_libraries(TINYCBOR)
target_link_libraries(TINYCBOR INTERFACE zephyr_interface)
