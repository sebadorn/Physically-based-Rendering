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
	float4 hit;
	float t;
} ray4;

typedef struct {
	global float4 bbMin; // bbMin.w = leftChild
	global float4 bbMax; // bbMax.w = rightChild
	global int4 faces;
} bvhNode __attribute__((aligned));


#if BRDF == 2

	typedef struct {
		constant float nu;
		constant float nv;
		constant float Rs;
		constant float Rd;
		constant float d;
		constant float Ni;
		constant ushort2 spd;
		constant char light
	} material __attribute__((aligned));

#else

	typedef struct {
		constant float d;
		constant float Ni;
		constant float p;
		constant float rough;
		constant ushort2 spd;
		constant char light
	} material __attribute__((aligned));

#endif
