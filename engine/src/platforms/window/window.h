#pragma once

#include "common/platform/defines.h"

#include "common/container/container_types.h"
#include "common/math/math_types.h"
#include "platforms/window/window_types.h"

namespace cetech {
    namespace window {
        enum WindowFlags {
            WINDOW_NOFLAG = 0,
            WINDOW_FULLSCREEN = 1,

        };

        enum WindowPos {
            WINDOWPOS_CENTERED = -1,
            WINDOWPOS_UNDEFINED = -2
        };

        Window make_window(const char* title,
                           const int32_t x,
                           const int32_t y,
                           const int32_t width,
                           const int32_t height,
                           WindowFlags flags);

        void destroy_window(const Window& w);

        void set_title(const Window& w, const char* title);
        const char* get_title(const Window& w);
    }
}