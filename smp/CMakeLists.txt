target_include_directories(MCUMGR INTERFACE 
    include
    port/zephyr/include
)

zephyr_library_sources(
    smp/port/zephyr/src/zephyr_smp.c
    smp/src/smp.c
)
