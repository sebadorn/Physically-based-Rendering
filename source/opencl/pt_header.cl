#define ANTI_ALIASING #ANTI_ALIASING#
#define BVH_STACKSIZE #BVH_STACKSIZE#
#define EPSILON 0.00001f
#define IMG_HEIGHT #IMG_HEIGHT#
#define IMG_WIDTH #IMG_WIDTH#
#define MAX_ADDED_DEPTH #MAX_ADDED_DEPTH#
#define MAX_DEPTH #MAX_DEPTH#
#define NI_AIR 1.0f
#define PI_X2 6.28318530718f
#define SAMPLES #SAMPLES#
#define SPEC 40
#define SPECTRAL_COLORSYSTEM #SPECTRAL_COLORSYSTEM#
#define WORKGROUPSIZE #WORKGROUPSIZE#
#define WORKGROUPSIZE_HALF #WORKGROUPSIZE_HALF#

constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

typedef struct {
	// vertices
	float4 a; // a.w = material index
	float4 b;
	float4 c;
	// vertex normals
	float4 an;
	float4 bn;
	float4 cn;
} face_t;

typedef struct {
	float4 origin;
	float4 dir;
} rayBase;

typedef struct {
	float4 origin;
	float4 dir;
	float4 normal;
	float t;
} ray4;

typedef struct {
	float4 bbMin; // bbMin.w = leftChild
	float4 bbMax; // bbMax.w = rightChild
	int4 faces;
} bvhNode;

typedef struct {
	float4 scratch; // scratch.w = p (isotropy factor)
	float d;
	float Ni;
	float rough;
	ushort spd;
	char light
} material;
