add_library(MCUMGR INTERFACE)

target_include_directories(MCUMGR INTERFACE 
    cborattr/include
    cmd/fs_mgmt/include
    cmd/img_mgmt/include
    cmd/os_mgmt/include
    mgmt/include
    smp/include
    smp/port/zephyr/include
)

zephyr_library()
zephyr_library_sources(
    cborattr/src/cborattr.c
    cmd/fs_mgmt/port/zephyr/src/zephyr_fs_mgmt.c
    cmd/fs_mgmt/src/fs_mgmt.c
    cmd/fs_mgmt/src/stubs.c
    cmd/img_mgmt/port/zephyr/src/zephyr_img_mgmt.c
    cmd/img_mgmt/src/img_mgmt.c
    cmd/img_mgmt/src/img_mgmt_state.c
    cmd/img_mgmt/src/img_mgmt_util.c
    cmd/img_mgmt/src/stubs.c
    cmd/os_mgmt/port/zephyr/src/zephyr_os_mgmt.c
    cmd/os_mgmt/src/os_mgmt.c
    cmd/os_mgmt/src/stubs.c
    mgmt/src/mgmt.c
    smp/port/zephyr/src/zephyr_smp.c
    smp/src/smp.c
)
zephyr_library_link_libraries(MCUMGR)
target_link_libraries(MCUMGR INTERFACE zephyr_interface NFFS)
