add_library(MCUMGR INTERFACE)

zephyr_include_directories(MCUMGR INTERFACE 
    cborattr/include
    cmd/img_mgmt/include
    cmd/os_mgmt/include
    mgmt/include
    smp/include
    smp/port/zephyr/include
)

zephyr_library()
zephyr_library_sources(
    cborattr/src/cborattr.c
    cmd/img_mgmt/port/zephyr/src/zephyr_img_mgmt.c
    cmd/img_mgmt/src/img_mgmt.c
    cmd/img_mgmt/src/img_mgmt_state.c
    cmd/img_mgmt/src/img_mgmt_util.c
    cmd/img_mgmt/src/stubs.c
    cmd/os_mgmt/src/os_mgmt.c
    cmd/os_mgmt/src/stubs.c
    cmd/os_mgmt/port/zephyr/src/zephyr_os_mgmt.c
    mgmt/src/mgmt.c
    smp/port/zephyr/src/zephyr_smp.c
    smp/src/smp.c
)
zephyr_library_link_libraries(MCUMGR)
target_link_libraries(MCUMGR INTERFACE zephyr_interface)
