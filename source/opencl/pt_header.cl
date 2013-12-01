#BACKFACE_CULLING#
#SHADOWS#
#define EPSILON 0.00001f
#define IMG_HEIGHT #IMG_HEIGHT#
#define IMG_WIDTH #IMG_WIDTH#
#define PI_X2 6.28318530718f
#define WORKGROUPSIZE #WORKGROUPSIZE#

__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

typedef struct hit_t {
	float4 position;
	float distance;
	int normalIndex;
	int nodeIndex;
	int faceIndex;
} hit_t;
