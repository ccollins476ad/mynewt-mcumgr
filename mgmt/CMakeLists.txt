target_include_directories(MCUMGR INTERFACE 
    include
    port/zephyr/include
)

zephyr_library_sources(
    mgmt/src/mgmt.c
    mgmt/port/zephyr/src/buf.c
)
