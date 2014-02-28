#ANTI_ALIASING#
#BACKFACE_CULLING#
#define MAX_DEPTH #MAX_DEPTH#
#define BVH_STACKSIZE 128 // TODO
#define EPSILON 0.00001f
#define IMG_HEIGHT #IMG_HEIGHT#
#define IMG_WIDTH #IMG_WIDTH#
#define MAX_ADDED_DEPTH #MAX_ADDED_DEPTH#
#define NI_AIR 1.0f
#define PI_X2 6.28318530718f
#define SPEC 40
#define SPECTRAL_COLORSYSTEM #SPECTRAL_COLORSYSTEM#
#define SAMPLES #SAMPLES#
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
	float4 bbMin;
	float4 bbMax;
	int4 faces;
	int leftChild;
	int rightChild;
} bvhNode;

typedef struct {
	float4 scratch; // scratch.w = p (isotropy factor)
	float d;
	float Ni;
	float gloss;
	ushort spd;
	char illum;
	char light
} material;
