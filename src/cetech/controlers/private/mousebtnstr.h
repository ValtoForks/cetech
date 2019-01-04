//==============================================================================
// Includes
//==============================================================================

static const char *_btn_to_str[MOUSE_BTN_MAX] = {
        [MOUSE_BTN_UNKNOWN] = "",
        [MOUSE_BTN_LEFT] = "left",
        [MOUSE_BTN_MIDLE] = "midle",
        [MOUSE_BTN_RIGHT] = "right",
};

static const char *_axis_to_str[MOUSE_AXIS_MAX] = {
        [MOUSE_AXIS_UNKNOWN] = "",
        [MOUSE_AXIS_ABSOULTE] = "absolute",
        [MOUSE_AXIS_RELATIVE] = "relative",
        [MOUSE_AXIS_WHEEL] = "wheel",
};