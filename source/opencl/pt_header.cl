#ANTI_ALIASING#
#BACKFACE_CULLING#
#SHADOWS#
#define BOUNCES #BOUNCES#
#define EPSILON 0.00001f
#define IMG_HEIGHT #IMG_HEIGHT#
#define IMG_WIDTH #IMG_WIDTH#
#define PI_X2 6.28318530718f
#define WORKGROUPSIZE #WORKGROUPSIZE#
#define WORKGROUPSIZE_HALF #WORKGROUPSIZE_HALF#

// "_Barycentric" or "_MoellerTrumbore"
#define checkFaceIntersection checkFaceIntersection_MoellerTrumbore

constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
constant int MOD_3[6] = { 0, 1, 2, 0, 1, 2 };

typedef struct {
	float4 position;
	float t;
	int nodeIndex;
	int faceIndex;
} hit_t __attribute__( ( aligned( 32 ) ) );

typedef struct {
	float4 split;  // [x, y, z, w]
	int4 children; // [left, right, isLeftLeaf, isRightLeaf]
	int axis;
} kdNonLeaf;

typedef struct {
	int8 ropes; // [left, right, back, front, bottom, top, facesIndex, numFaces]
	float4 bbMin;
	float4 bbMax;
} kdLeaf __attribute__( ( aligned( 64 ) ) );
