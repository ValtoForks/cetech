//==============================================================================
// includes
//==============================================================================

#include <celib/allocator.h>
#include <celib/api_system.h>
#include <celib/memory.h>
#include <celib/module.h>
#include <celib/hashlib.h>
#include <cetech/renderer/renderer.h>
#include <cetech/renderer/gfx.h>
#include <cetech/resource/resource.h>
#include <cetech/material/material.h>
#include <cetech/debugui/debugui.h>
#include <cetech/ecs/ecs.h>
#include <cetech/render_graph/render_graph.h>
#include <cetech/camera/camera.h>
#include <cetech/debugdraw/debugdraw.h>
#include <cetech/mesh/mesh_renderer.h>
#include <string.h>
#include <celib/fmath.inl>
#include <celib/macros.h>


#include "cetech/default_rg/default_rg.h"


#define _G render_graph_global

//==============================================================================
// GLobals
//==============================================================================

static struct _G {
    struct ce_alloc *alloc;
} _G;

//==============================================================================
// output pass
//==============================================================================
struct PosTexCoord0Vertex {
    float m_x;
    float m_y;
    float m_z;
    float m_u;
    float m_v;
};

static ct_render_vertex_decl_t ms_decl;

static void init_decl() {
    ct_gfx_a0->vertex_decl_begin(&ms_decl,
                                 ct_gfx_a0->get_renderer_type());
    ct_gfx_a0->vertex_decl_add(&ms_decl,
                               CT_RENDER_ATTRIB_POSITION, 3,
                               CT_RENDER_ATTRIB_TYPE_FLOAT, false, false);

    ct_gfx_a0->vertex_decl_add(&ms_decl,
                               CT_RENDER_ATTRIB_TEXCOORD0, 2,
                               CT_RENDER_ATTRIB_TYPE_FLOAT, false, false);

    ct_gfx_a0->vertex_decl_end(&ms_decl);
}


void screenspace_quad(float _textureWidth,
                      float _textureHeight,
                      float _texelHalf,
                      bool _originBottomLeft,
                      float _width,
                      float _height) {
    if (3 == ct_gfx_a0->get_avail_transient_vertex_buffer(3, &ms_decl)) {
        ct_render_transient_vertex_buffer_t vb;
        ct_gfx_a0->alloc_transient_vertex_buffer(&vb, 3, &ms_decl);
        struct PosTexCoord0Vertex *vertex = (struct PosTexCoord0Vertex *) vb.data;

        const float minx = -_width;
        const float maxx = _width;
        const float miny = 0.0f;
        const float maxy = _height * 2.0f;

        const float texelHalfW = _texelHalf / _textureWidth;
        const float texelHalfH = _texelHalf / _textureHeight;
        const float minu = -1.0f + texelHalfW;
        const float maxu = 1.0f + texelHalfH;

        const float zz = 0.0f;

        float minv = texelHalfH;
        float maxv = 2.0f + texelHalfH;

        if (_originBottomLeft) {
            float temp = minv;
            minv = maxv;
            maxv = temp;

            minv -= 1.0f;
            maxv -= 1.0f;
        }

        vertex[0].m_x = minx;
        vertex[0].m_y = miny;
        vertex[0].m_z = zz;
        vertex[0].m_u = minu;
        vertex[0].m_v = minv;

        vertex[1].m_x = maxx;
        vertex[1].m_y = miny;
        vertex[1].m_z = zz;
        vertex[1].m_u = maxu;
        vertex[1].m_v = minv;

        vertex[2].m_x = maxx;
        vertex[2].m_y = maxy;
        vertex[2].m_z = zz;
        vertex[2].m_u = maxu;
        vertex[2].m_v = maxv;

        ct_gfx_a0->set_transient_vertex_buffer(0, &vb, 0, 3);
    }
}


static void output_pass_on_setup(void *inst,
                                 struct ct_rg_builder *builder) {

    builder->create(builder,
                    RG_OUTPUT_TEXTURE,
                    (struct ct_rg_attachment) {
                            .format = CT_RENDER_TEXTURE_FORMAT_RGBA8,
                            .ratio = CT_RENDER_BACKBUFFER_RATIO_EQUAL
                    }
    );

    builder->read(builder, _COLOR);
    builder->add_pass(builder, inst, 0);
}

static uint64_t copy_material = 0;

static void output_pass_on_pass(void *inst,
                                uint8_t viewid,
                                uint64_t layer,
                                struct ct_rg_builder *builder) {
    ct_gfx_a0->set_view_clear(viewid,
                              CT_RENDER_CLEAR_COLOR |
                              CT_RENDER_CLEAR_DEPTH,
                              0x66CCFFff, 1.0f, 0);

    uint16_t size[2] = {};
    builder->get_size(builder, size);

    ct_gfx_a0->set_view_rect(viewid,
                             0, 0,
                             size[0], size[1]);

    float proj[16];
    ce_mat4_ortho(proj, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 100.0f, 0.0f,
                  ct_gfx_a0->get_caps()->homogeneousDepth);

    ct_gfx_a0->set_view_transform(viewid, NULL, proj);

    if (!copy_material) {
        copy_material = ct_material_a0->create(0xe27880f9fbb28b8d);
    }

    ct_render_texture_handle_t th;
    th = builder->get_texture(builder, _COLOR);

    ct_material_a0->set_texture_handler(copy_material,
                                        _DEFAULT,
                                        "s_input_texture",
                                        th);

    screenspace_quad(size[0], size[1], 0,
                     ct_gfx_a0->get_caps()->originBottomLeft, 1.f, 1.0f);

    ct_material_a0->submit(copy_material, _DEFAULT, viewid);
}

static void gbuffer_pass_on_setup(void *inst,
                                  struct ct_rg_builder *builder) {

    builder->create(builder, _COLOR,
                    (struct ct_rg_attachment) {
                            .format = CT_RENDER_TEXTURE_FORMAT_RGBA8,
                            .ratio = CT_RENDER_BACKBUFFER_RATIO_EQUAL
                    }
    );

    builder->create(builder, _DEPTH,
                    (struct ct_rg_attachment) {
                            .format = CT_RENDER_TEXTURE_FORMAT_D24,
                            .ratio = CT_RENDER_BACKBUFFER_RATIO_EQUAL
                    }
    );

    builder->add_pass(builder, inst, _GBUFFER);
}

struct gbuffer_pass {
    struct ct_rg_pass pass;
    struct ct_entity camera;
    struct ct_world world;
};

static void gbuffer_pass_on_pass(void *inst,
                                 uint8_t viewid,
                                 uint64_t layer,
                                 struct ct_rg_builder *builder) {
    struct gbuffer_pass *pass = inst;

    ct_gfx_a0->set_view_clear(viewid,
                              CT_RENDER_CLEAR_COLOR |
                              CT_RENDER_CLEAR_DEPTH,
                              0x66CCFFff, 1.0f, 0);

    uint16_t size[2] = {};
    builder->get_size(builder, size);

    ct_gfx_a0->set_view_rect(viewid, 0, 0, size[0], size[1]);

    float view_matrix[16];
    float proj_matrix[16];

    ct_camera_a0->get_project_view(pass->world,
                                   pass->camera,
                                   proj_matrix,
                                   view_matrix,
                                   size[0],
                                   size[1]);

    ct_gfx_a0->set_view_transform(viewid, view_matrix,
                                  proj_matrix);
}


static void feed_module(struct ct_rg_module *m1,
                        struct ct_world world,
                        struct ct_entity camera) {
    struct ct_rg_module* gm = m1->add_extension_point(m1, _GBUFFER);

    gm->add_pass(gm, &(struct gbuffer_pass) {
            .camera = camera,
            .world = world,
            .pass = {
                    .on_setup = gbuffer_pass_on_setup,
                    .on_pass = gbuffer_pass_on_pass,
            }
    }, sizeof(struct gbuffer_pass));

    m1->add_pass(m1, &(struct ct_rg_pass) {
            .on_pass = output_pass_on_pass,
            .on_setup = output_pass_on_setup
    }, sizeof(struct ct_rg_pass));
}

static struct ct_default_rg_a0 default_render_graph_api = {
        .feed_module= feed_module,
};

struct ct_default_rg_a0 *ct_default_rg_a0 = &default_render_graph_api;


static void _init(struct ce_api_a0 *api) {
    CE_UNUSED(api);
    _G = (struct _G) {
            .alloc = ce_memory_a0->system,
    };

    init_decl();

    api->register_api("ct_default_rg_a0", &default_render_graph_api);
}

static void _shutdown() {
    _G = (struct _G) {};
}

CE_MODULE_DEF(
        default_render_graph,
        {
            CE_INIT_API(api, ce_id_a0);
            CE_INIT_API(api, ce_memory_a0);
            CE_INIT_API(api, ct_renderer_a0);
            CE_INIT_API(api, ct_rg_a0);
            CE_INIT_API(api, ct_debugui_a0);
            CE_INIT_API(api, ct_ecs_a0);
            CE_INIT_API(api, ct_camera_a0);
            CE_INIT_API(api, ct_mesh_renderer_a0);
            CE_INIT_API(api, ct_dd_a0);
            CE_INIT_API(api, ct_material_a0);
        },
        {
            CE_UNUSED(reload);
            _init(api);
        },
        {
            CE_UNUSED(reload);
            CE_UNUSED(api);

            _shutdown();
        }
)