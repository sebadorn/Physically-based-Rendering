#define ANTI_ALIASING #ANTI_ALIASING#
#define BRDF #BRDF#
#define BVH_STACKSIZE #BVH_STACKSIZE#
#define EPSILON 0.00001f
#define IMG_HEIGHT #IMG_HEIGHT#
#define IMG_WIDTH #IMG_WIDTH#
#define IMPLICIT #IMPLICIT#
#define MAX_ADDED_DEPTH #MAX_ADDED_DEPTH#
#define MAX_DEPTH #MAX_DEPTH#
#define NI_AIR 1.00028f
#define PI_X2 6.28318530718f
#define SAMPLES #SAMPLES#
#define SKY_LIGHT #SKY_LIGHT#
#define SPEC 40
#define SPECTRAL_COLORSYSTEM #SPECTRAL_COLORSYSTEM#


typedef struct {
	// vertices
	global float4 a; // a.w = material index
	global float4 b;
	global float4 c;
	// vertex normals
	global float4 an;
	global float4 bn;
	global float4 cn;
} face_t __attribute__((aligned));

typedef struct {
	float4 origin;
	float4 dir;
	float4 normal;
	float t;
} ray4;

typedef struct {
	global float4 bbMin; // bbMin.w = leftChild
	global float4 bbMax; // bbMax.w = rightChild
	global int4 faces;
} bvhNode __attribute__((aligned));

typedef struct {
	constant float4 scratch; // scratch.w = p (isotropy factor)
	constant float d;
	constant float Ni;
	constant float rough;
	constant ushort spd;
	constant char light
} material __attribute__((aligned));
