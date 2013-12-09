#ANTI_ALIASING#
#BACKFACE_CULLING#
#SHADOWS#
#define BOUNCES #BOUNCES#
#define EPSILON 0.000001f
#define IMG_HEIGHT #IMG_HEIGHT#
#define IMG_WIDTH #IMG_WIDTH#
#define PI_X2 6.28318530718f
#define WORKGROUPSIZE #WORKGROUPSIZE#
#define WORKGROUPSIZE_HALF #WORKGROUPSIZE_HALF#

__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

typedef struct hit_t {
	float4 position;
	float t;
	int nodeIndex;
	int faceIndex;
} hit_t;
