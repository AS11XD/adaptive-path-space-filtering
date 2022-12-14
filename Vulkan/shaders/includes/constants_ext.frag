#define PI 3.14159265359
#define RAY_OFFSET_EPS 0.0001
#define N_DOT_V_OFFSET 0.00001

#define RS_COLOR 0
#define RS_G_BUFFER 1
#define RS_POSITION 2
#define RS_NORMAL 3
#define RS_MATERIAL 4
#define RS_DEPTH 5
#define RS_BRIGHTNESS 6

#define L_DIRECTIONAL 0
#define L_POINT 1

#define RM_DIFFUSE_COLOR 0
#define RM_SPECULAR_COLOR 1
#define RM_RTX_PREPASS 2
#define RM_RAY_TRACING 3
#define RM_PATH_TRACING 4
#define RM_SAMPLE_VALIDATION 5
#define RM_PATH_SPACE_FILTERING 6
#define RM_ADAPTIVE_PATH_SPACE_FILTERING_1 7
#define RM_ADAPTIVE_PATH_SPACE_FILTERING_2 8

#define BRDFSS_UNIFORM 0
#define BRDFSS_DIFFUSE 1
#define BRDFSS_SPECULAR_GGX 2
#define BRDFSS_DIFFUSE_SPECULAR 3

#define PSFM_DEFAULT 0
#define PSFM_HASH_MAP_OCCUPATION 1
#define PSFM_DEBUG_HASH_CELLS 2
#define PSFM_DEBUG_HASH_FINGERPRINTS 3

#define VFT_ONE_OVER_X_SQUARED 0
#define VFT_SMOOTH_LINEAR 1
#define VFT_STEP 2
#define VFT_LINEAR 3
#define VFT_ONE 4
#define VFT_ZERO 5