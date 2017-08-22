#include <cetech/debugui/private/bgfx_imgui/imgui.h>

#define  _to_imvec2(a) (*((ImVec2*) (a)))
#define  _to_imvec4(a) (*((ImVec4*) (a)))

namespace imgui_wrap {

    void TextColoredV(const _vec4 col,
                      const char *fmt,
                      va_list args) {
        ImGui::TextColoredV(_to_imvec4(col), fmt, args);
    }

    void TextColored(const _vec4 col,
                     const char *fmt,
                     ...) {
        va_list args;
        va_start(args, fmt);
        TextColoredV(col, fmt, args);
        va_end(args);
    }


    bool Button(const char *label,
                const _vec2 size) {
        return ImGui::Button(label, _to_imvec2(size));
    }

    bool InvisibleButton(const char *str_id,
                         const _vec2 size) {
        return ImGui::InvisibleButton(str_id, _to_imvec2(size));
    }


    void Image2(struct ct_texture user_texture_id,
               const _vec2 size,
               const _vec2 uv0,
               const _vec2 uv1,
               const _vec4 tint_col,
               const _vec4 border_col) {
        return ImGui::Image((bgfx::TextureHandle){user_texture_id.idx},
                            _to_imvec2(size),
                            _to_imvec2(uv0),
                            _to_imvec2(uv1),
                            _to_imvec4(tint_col),
                            _to_imvec4(border_col));
    }

    bool ImageButton(ImTextureID user_texture_id,
                     const _vec2 size,
                     const _vec2 uv0,
                     const _vec2 uv1,
                     int frame_padding,
                     const _vec4 bg_col,
                     const _vec4 tint_col) {
        return ImGui::ImageButton(user_texture_id,
                                  _to_imvec2(size),
                                  _to_imvec2(uv0),
                                  _to_imvec2(uv1),
                                  frame_padding,
                                  _to_imvec4(bg_col),
                                  _to_imvec4(tint_col));
    }

    bool RadioButton2(const char *label,
                   int *v,
                   int v_button) {
        return ImGui::RadioButton(
                label,
                v,
                v_button);
    }

    bool Combo2(const char *label,
                int *current_item,
                const char *items_separated_by_zeros,
                int height_in_items) {
        return ImGui::Combo(
                label,
                current_item,
                items_separated_by_zeros,
                height_in_items);
    }

    bool Combo3(const char *label,
                int *current_item,
                bool (*items_getter)(void *data,
                                     int idx,
                                     const char **out_text),
                void *data,
                int items_count,
                int height_in_items) {
        return ImGui::Combo(
                label,
                current_item,
                items_getter,
                data,
                items_count,
                height_in_items);
    }

    bool ColorButton(const _vec4 col,
                     bool small_height,
                     bool outline_border) {
        return ImGui::ColorButton(
                _to_imvec4(col),
                           small_height,
                           outline_border);
    }

    void ColorEditMode(ImGuiColorEditMode mode) {
        ImGui::ColorEditMode(mode);
    }

    void PlotLines(const char *label,
                   const float *values,
                   int values_count,
                   int values_offset,
                   const char *overlay_text,
                   float scale_min,
                   float scale_max,
                   _vec2 graph_size,
                   int stride) {
        ImGui::PlotLines(
                label,
                values,
                values_count,
                values_offset,
                overlay_text,
                scale_min,
                scale_max,
                (ImVec2 &) graph_size,
                stride);
    }

    void PlotLines2(const char *label,
                    float (*values_getter)(void *data,
                                           int idx),
                    void *data,
                    int values_count,
                    int values_offset,
                    const char *overlay_text,
                    float scale_min,
                    float scale_max,
                    _vec2 graph_size) {
        return ImGui::PlotLines(
                label,
                values_getter, data,
                values_count,
                values_offset,
                overlay_text,
                scale_min,
                scale_max,
                (ImVec2 &) graph_size);
    }

    void PlotHistogram(const char *label,
                       const float *values,
                       int values_count,
                       int values_offset,
                       const char *overlay_text,
                       float scale_min,
                       float scale_max,
                       _vec2 graph_size,
                       int stride) {
        return ImGui::PlotHistogram(
                label,
                values,
                values_count,
                values_offset,
                overlay_text,
                scale_min,
                scale_max,
                (ImVec2 &) graph_size,
                stride);
    }

    void PlotHistogram2(const char *label,
                        float (*values_getter)(void *data,
                                               int idx),
                        void *data,
                        int values_count,
                        int values_offset,
                        const char *overlay_text,
                        float scale_min,
                        float scale_max,
                        _vec2 graph_size) {
        return ImGui::PlotHistogram(
                label,
                values_getter,
                data,
                values_count,
                values_offset,
                overlay_text,
                scale_min,
                scale_max,
                (ImVec2 &) graph_size);
    }

    void ProgressBar(float fraction,
                     const _vec2 size_arg,
                     const char *overlay) {
        return ImGui::ProgressBar(
                fraction,
                (ImVec2 &) size_arg,
                overlay);
    }

    bool DragFloat2(const char *label,
                    float v[2],
                    float v_speed,
                    float v_min,
                    float v_max,
                    const char *display_format,
                    float power) {
        return ImGui::DragFloat(
                label,
                v,
                v_speed,
                v_min,
                v_max,
                display_format,
                power);
    }

    bool DragFloat3(const char *label,
                    float v[3],
                    float v_speed,
                    float v_min,
                    float v_max,
                    const char *display_format,
                    float power) {
        return ImGui::DragFloat(
                label,
                v,
                v_speed,
                v_min,
                v_max,
                display_format,
                power);
    }

    bool DragFloat4(const char *label,
                    float v[4],
                    float v_speed,
                    float v_min,
                    float v_max,
                    const char *display_format,
                    float power) {
        return ImGui::DragFloat(
                label,
                v,
                v_speed,
                v_min,
                v_max,
                display_format,
                power);
    }


    bool DragInt2(const char *label,
                  int v[2],
                  float v_speed,
                  int v_min,
                  int v_max,
                  const char *display_format) {
        return ImGui::DragInt(
                label,
                v,
                v_speed,
                v_min,
                v_max,
                display_format);
    }

    bool DragInt3(const char *label,
                  int v[3],
                  float v_speed,
                  int v_min,
                  int v_max,
                  const char *display_format) {
        return ImGui::DragInt(
                label,
                v,
                v_speed,
                v_min,
                v_max,
                display_format);
    }

    bool DragInt4(const char *label,
                  int v[4],
                  float v_speed,
                  int v_min,
                  int v_max,
                  const char *display_format) {
        return ImGui::DragInt(
                label,
                v,
                v_speed,
                v_min,
                v_max,
                display_format);
    }

    bool InputTextMultiline(const char *label,
                            char *buf,
                            size_t buf_size,
                            const _vec2 size,
                            ImGuiInputTextFlags flags,
                            ImGuiTextEditCallback callback,
                            void *user_data) {
        return ImGui::InputTextMultiline(
                label,
                buf,
                buf_size,
                (ImVec2 &) size,
                flags, callback, user_data);
    }


    bool InputFloat2(const char *label,
                     float v[2],
                     int decimal_precision,
                     ImGuiInputTextFlags extra_flags) {
        return ImGui::InputFloat(
                label,
                v,
                decimal_precision, extra_flags);
    }

    bool InputFloat3(const char *label,
                     float v[3],
                     int decimal_precision,
                     ImGuiInputTextFlags extra_flags) {
        return ImGui::InputFloat(
                label,
                v,
                decimal_precision, extra_flags);
    }

    bool InputFloat4(const char *label,
                     float v[4],
                     int decimal_precision,
                     ImGuiInputTextFlags extra_flags) {
        return ImGui::InputFloat(
                label,
                v,
                decimal_precision, extra_flags);
    }

    bool InputInt2(const char *label,
                   int v[2],
                   ImGuiInputTextFlags extra_flags) {
        return ImGui::InputInt(
                label,
                v, extra_flags);
    }

    bool InputInt3(const char *label,
                   int v[3],
                   ImGuiInputTextFlags extra_flags) {
        return ImGui::InputInt3(
                label,
                v, extra_flags);
    }

    bool InputInt4(const char *label,
                   int v[4],
                   ImGuiInputTextFlags extra_flags) {
        return ImGui::InputInt4(
                label, v, extra_flags);
    }


    bool SliderFloat2(const char *label,
                      float v[2],
                      float v_min,
                      float v_max,
                      const char *display_format,
                      float power) {
        return ImGui::SliderFloat(
                label,
                v,
                v_min,
                v_max,
                display_format,
                power);
    }

    bool SliderFloat3(const char *label,
                      float v[3],
                      float v_min,
                      float v_max,
                      const char *display_format,
                      float power) {
        return ImGui::SliderFloat(
                label,
                v,
                v_min,
                v_max,
                display_format,
                power);
    }

    bool SliderFloat4(const char *label,
                      float v[4],
                      float v_min,
                      float v_max,
                      const char *display_format,
                      float power) {
        return ImGui::SliderFloat4(
                label,
                v,
                v_min,
                v_max,
                display_format,
                power);
    }


    bool VSliderFloat(const char *label,
                      const _vec2 size,
                      float *v,
                      float v_min,
                      float v_max,
                      const char *display_format,
                      float power) {
        return ImGui::VSliderFloat(
                label,
                (ImVec2 &) size,
                v,
                v_min,
                v_max,
                display_format,
                power);
    }

    bool VSliderInt(const char *label,
                    const _vec2 size,
                    int *v,
                    int v_min,
                    int v_max,
                    const char *display_format) {
        return ImGui::VSliderInt(
                label,
                (ImVec2 &) size,
                v,
                v_min,
                v_max,
                display_format);
    }


    bool TreeNodeV2(const void *ptr_id,
                    const char *fmt,
                    va_list args) {
        return ImGui::TreeNodeV(
                ptr_id,
                fmt, args);
    }

    bool TreeNode2(const char *str_id,
                   const char *fmt,
                   ...) {
        va_list args;
        va_start(args, fmt);
        bool ret = ImGui::TreeNodeV(str_id, fmt, args);
        va_end(args);

        return ret;
    }

    bool TreeNode3(const void *ptr_id,
                   const char *fmt,
                   ...) {
        va_list args;
        va_start(args, fmt);
        bool ret = TreeNodeV2(ptr_id, fmt, args);
        va_end(args);

        return ret;
    }


    bool TreeNodeEx2(const char *str_id,
                     ImGuiTreeNodeFlags flags,
                     const char *fmt,
                     ...) {
        va_list args;
        va_start(args, fmt);
        bool ret = ImGui::TreeNodeExV(str_id, flags, fmt, args);
        va_end(args);

        return ret;
    }

    bool TreeNodeEx3(const void *ptr_id,
                     ImGuiTreeNodeFlags flags,
                     const char *fmt,
                     ...) {
        va_list args;
        va_start(args, fmt);
        bool ret = ImGui::TreeNodeExV(ptr_id, flags, fmt, args);
        va_end(args);

        return ret;
    }

    bool TreeNodeExV(const char *str_id,
                     ImGuiTreeNodeFlags flags,
                     const char *fmt,
                     va_list args) {
        return ImGui::TreeNodeExV(str_id, flags, fmt, args);
    }

    bool TreeNodeExV2(const void *ptr_id,
                      ImGuiTreeNodeFlags flags,
                      const char *fmt,
                      va_list args) {
        return ImGui::TreeNodeExV(ptr_id, flags, fmt, args);
    }

    void TreePush2(const void *ptr_id) {
        return ImGui::TreePush(ptr_id);
    }

    bool CollapsingHeader(const char *label,
                           ImGuiTreeNodeFlags flags) {
        return ImGui::CollapsingHeader(label, flags);
    }

    bool CollapsingHeader2(const char *label,
                           bool *p_open,
                           ImGuiTreeNodeFlags flags) {
        return ImGui::CollapsingHeader(label, p_open, flags);
    }

    bool Selectable(const char *label,
                    bool selected,
                    ImGuiSelectableFlags flags,
                    const _vec2 size) {
        return ImGui::Selectable(
                label,
                selected, flags, (ImVec2 &) size);
    }

    bool Selectable2(const char *label,
                     bool *p_selected,
                     ImGuiSelectableFlags flags,
                     const _vec2 size) {
        return ImGui::Selectable(
                label,
                p_selected, flags,
                (ImVec2 &) size);
    }

    bool ListBox2(const char *label,
                  int *current_item,
                  bool (*items_getter)(void *data,
                                       int idx,
                                       const char **out_text),
                  void *data,
                  int items_count,
                  int height_in_items) {
        return ImGui::ListBox(
                label,
                current_item,
                items_getter, data,
                items_count,
                height_in_items);
    }

    bool ListBoxHeader(const char *label,
                       const _vec2 size) {
        return ImGui::ListBoxHeader(
                label,
                (ImVec2 &) size);
    }

    bool ListBoxHeader2(const char *label,
                        int items_count,
                        int height_in_items) {
        return ImGui::ListBoxHeader(
                label,
                items_count,
                height_in_items);
    }

    void ListBoxFooter() {
        return ImGui::ListBoxFooter();
    }

    void Value(const char *prefix,
               bool b) {
        return ImGui::Value(
                prefix,
                b);
    }

    void Value2(const char *prefix,
                int v) {
        return ImGui::Value(
                prefix,
                v);
    }

    void Value3(const char *prefix,
                unsigned int v) {
        return ImGui::Value(
                prefix,
                v);
    }

    void Value4(const char *prefix,
                float v,
                const char *float_format) {
        return ImGui::Value(
                prefix,
                v,
                float_format);
    }

    void ValueColor(const char *prefix,
                    const _vec4 v) {
        return ImGui::ValueColor(
                prefix,
                (ImVec4 &) v);
    }

    void ValueColor2(const char *prefix,
                     ImU32 v) {
        return ImGui::ValueColor(prefix, v);
    }

    bool MenuItem2(const char *label,
                   const char *shortcut,
                   bool *p_selected,
                   bool enabled) {
        return ImGui::MenuItem(
                label,
                shortcut,
                p_selected,
                enabled);
    }


    void ColorWheel2(const char* text, uint32_t* rgba, float size) {
        ImGui::ColorWheel(text, rgba,size);
    }


    void GetWindowSize(float size[2]) {
        auto v = ImGui::GetWindowSize();
        size[0] = v.x;
        size[1] = v.y;
    }

    bool BeginDock(const char* label, bool* opened, DebugUIWindowFlags_ extra_flags)
    {
        return ImGui::BeginDock(label, opened, extra_flags);
    }

    void EndDock()
    {
        ImGui::EndDock();
    }
}