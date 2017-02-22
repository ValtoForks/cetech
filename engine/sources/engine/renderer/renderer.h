//! \defgroup Renderer
//! Renderer system
//! \{
#ifndef CETECH_RENDERER_H
#define CETECH_RENDERER_H

//==============================================================================
// Includes
//==============================================================================

#include <engine/world/camera.h>
#include "celib/window/types.h"
#include "celib/math/types.h"

//==============================================================================
// Interface
//==============================================================================


typedef int viewport_t;

//! Render API V1
struct RendererApiV1 {
    //! Create renderer.
    //! \param window Window
    void (*create)(cel_window_t window);

    //! Set debug mode on/off
    //! \param debug True/False
    void (*set_debug)(int debug);

    //! Get renderer window size
    //! \return Renderer window size
    cel_vec2f_t (*get_size)();

    //! Render world
    //! \param world World
    //! \param camera Camera
    //! \param viewport Viewport
    void (*render_world)(world_t world,
                         camera_t camera,
                         viewport_t viewport);
};


#endif //CETECH_RENDERER_H
//! \}
