#define ACCEL_STRUCT #ACCEL_STRUCT#
#define ANTI_ALIASING #ANTI_ALIASING#
#define BRDF #BRDF#
#define BVH_STACKSIZE #BVH_STACKSIZE#
#define EPSILON5 0.00001f
#define EPSILON7 0.0000001f
#define EPSILON10 0.0000000001f
#define IMG_HEIGHT #IMG_HEIGHT#
#define IMG_WIDTH #IMG_WIDTH#
#define IMPLICIT #IMPLICIT#
#define MAX_ADDED_DEPTH #MAX_ADDED_DEPTH#
#define MAX_DEPTH #MAX_DEPTH#
#define NI_AIR 1.00028f
#define PHONGTESS #PHONGTESS#
#define PHONGTESS_ALPHA #PHONGTESS_ALPHA#
#define PI_X2 6.28318530718f
#define SAMPLES #SAMPLES#
#define SKY_LIGHT #SKY_LIGHT#
// TODO: Using any other value than 40 doesn't really work.
#define SPEC 40
#define SPECTRAL_COLORSYSTEM #SPECTRAL_COLORSYSTEM#
#define USE_SPECTRAL #USE_SPECTRAL#


typedef struct {
	float4 origin;
	float4 dir;
	float4 normal;
	float t;
} ray4;

typedef struct {
	float3 n1; // Normal of plane 1
	float3 n2; // Normal of plane 2
	float o1;  // Distance of plane 1 to the origin
	float o2;  // Distance of plane 2 to the origin
} rayPlanes;

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


// BVH
#if ACCEL_STRUCT == 0

	typedef struct {
		global float4 bbMin; // bbMin.w = leftChild
		global float4 bbMax; // bbMax.w = rightChild
		global int2 facesInterval; // x = start index; y = number of faces
	} bvhNode __attribute__((aligned));

// kD-tree
#elif ACCEL_STRUCT == 1

	typedef struct {
		global float4 bbMin; // bbMin.w = leftChild
		global float4 bbMax; // bbMax.w = rightChild
	} bvhNode __attribute__((aligned));

	typedef struct {
		global float split;
		global int4 children; // [left, right, isLeftLeaf, isRightLeaf]
		global short axis;
	} kdNonLeaf __attribute__((aligned));

	typedef struct {
		global int8 ropes; // [left, right, bottom, top, back, front, facesIndex, numFaces]
		global float4 bbMin;
		global float4 bbMax;
	} kdLeaf __attribute__((aligned));

#endif


// Color mode: RGB
#if USE_SPECTRAL == 0

	#define MTL_COLOR constant float4 rgbDiff; constant float4 rgbSpec;

// Color mode: SPD
#elif USE_SPECTRAL == 1

	#define MTL_COLOR constant ushort2 spd;

#endif


// Schlick
#if BRDF == 0

	typedef struct {
		constant float d;
		constant float Ni;
		constant float p;
		constant float rough;
		MTL_COLOR
		constant char light
	} material __attribute__((aligned));

// Shirley-Ashikhmin
#elif BRDF == 1

	typedef struct {
		constant float nu;
		constant float nv;
		constant float Rs;
		constant float Rd;
		constant float d;
		constant float Ni;
		MTL_COLOR
		constant char light
	} material __attribute__((aligned));

#endif
