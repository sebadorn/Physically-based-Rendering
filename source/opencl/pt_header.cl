#ANTI_ALIASING#
#BACKFACE_CULLING#
#define MAX_DEPTH #MAX_DEPTH#
#define BVH_STACKSIZE 20
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
constant int MOD_3[6] = { 0, 1, 2, 0, 1, 2 };


typedef struct {
	float4 a, b, c;    // vertices; a.w = material index
	float4 an, bn, cn; // vertex normals
} face_t;

typedef struct {
	float4 origin;
	float4 dir;
	float4 normal;
	float t;
	int faceIndex;
} ray4;

typedef struct {
	float4 split;  // [x, y, z, axis]
	int4 children; // [left, right, isLeftLeaf, isRightLeaf]
} kdNonLeaf;

typedef struct {
	int8 ropes; // [left, right, back, front, bottom, top, facesIndex, numFaces]
	float4 bbMin;
	float4 bbMax;
} kdLeaf;

typedef struct {
	float4 bbMin; // bbMin.w = left child node / kD-tree root node
	float4 bbMax; // bbMax.w = right child node
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
