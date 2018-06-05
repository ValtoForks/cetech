#ifndef CETECH_KERNEL_H
#define CETECH_KERNEL_H

//==============================================================================
// Include
//==============================================================================

#define CONFIG_PLATFORM_ID "kernel.platform"
#define CONFIG_NATIVE_PLATFORM_ID "kernel.native_platform"

#define CONFIG_BUILD_ID "build"
#define CONFIG_SRC_ID "src"
#define CONFIG_CORE_ID "core"
#define CONFIG_COMPILE_ID "compile"
#define CONFIG_EXTRENAL_ID "external"

#define CONFIG_MODULE_DIR_ID "module_dir"

#define CONFIG_BOOT_PKG_ID "core.boot_pkg"
#define CONFIG_DAEMON_ID "daemon"
#define CONFIG_CONTINUE_ID "continue"
#define CONFIG_WAIT_ID "wait"

#define CONFIG_GAME_ID "game"

#define CONFIG_WID_ID "screen.wid"
#define CONFIG_SCREEN_X_ID "screen.x"
#define CONFIG_SCREEN_Y_ID "screen.y"
#define CONFIG_SCREEN_VSYNC_ID "screen.vsync"
#define CONFIG_SCREEN_FULLSCREEN_ID "screen.fullscreen"

enum {
    KERNEL_EBUS = 0x6a0c4eb6
};

enum {
    KERNEL_INVALID_EVNT = 0,
    KERNEL_INIT_EVENT,
    KERNEL_UPDATE_EVENT,
    KERNEL_QUIT_EVENT,
    KERNEL_SHUTDOWN_EVENT,
};

enum {
    KERNEL_ORDER = 1024,
    GAME_ORDER = KERNEL_ORDER + 1024,
    RENDER = GAME_ORDER + 1024,
};

struct ct_app_update_ev {
    float dt;
};

#endif //CETECH_KERNEL_H
