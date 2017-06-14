//! \addtogroup Plugin
//! \{
#ifndef CETECH_API_H
#define CETECH_API_H

#ifdef __cplusplus
extern "C" {
#endif

//==============================================================================
// Defines
//==============================================================================

#define IMPORT_API(name) static struct name name = {0};
#define GET_API(_api, name) name = *(struct name*) (_api)->first(#name).api

//==============================================================================
// Api
//==============================================================================


struct api_entry {
    void *entry;
    void *api;
};

//! Plugin expot api struct V0
struct api_v0 {
    void (*register_api)(const char *name,
                         void *api);

    struct api_entry (*first)(const char *name);

    struct api_entry (*next)(struct api_entry *entry);
};

#ifdef __cplusplus
}
#endif

#endif //CETECH_API_H
//! \}