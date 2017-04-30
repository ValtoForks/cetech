#ifndef CELIB_MATH_TYPES_H
#define CELIB_MATH_TYPES_H

//==============================================================================
// Includes
//==============================================================================


//==============================================================================
// Vectors
//==============================================================================

typedef struct {
    union {
        float f[2];
        struct {
            float x;
            float y;
        };
    };
} vec2f_t;


typedef struct {
    union {
        float f[3];
        struct {
            float x;
            float y;
            float z;
        };
    };
} vec3f_t;

typedef struct {
    union {
        float f[4];
        struct {
            float x;
            float y;
            float z;
            float w;
        };
    };
} vec4f_t;

//==============================================================================
// Quaternion
//==============================================================================

typedef vec4f_t quatf_t;


//==============================================================================
// Matrix
//==============================================================================

typedef struct {
    union {
        float f[3 * 3];
        struct {
            vec3f_t x;
            vec3f_t y;
            vec3f_t z;
        };
    };
} mat33f_t;


typedef struct {
    union {
        float f[4 * 4];
        struct {
            vec4f_t x;
            vec4f_t y;
            vec4f_t z;
            vec4f_t w;
        };
    };
} mat44f_t;

#endif //CELIB_MATH_TYPES_H
