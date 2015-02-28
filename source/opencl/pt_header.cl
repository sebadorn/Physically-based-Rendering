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
	float4 a; // a.w = material index
	float4 b;
	float4 c;
	// vertex normals
	float4 an;
	float4 bn;
	float4 cn;
} face_t;


// BVH
#if ACCEL_STRUCT == 0

	typedef struct {
		float4 bbMin; // bbMin.w = leftChild
		float4 bbMax; // bbMax.w = rightChild
		int4 faces;   // w: relation with
		              // -> positive: right sibling
		              // -> negative: parent
		              // -> 0: root node
	} bvhNode;

// kD-tree
#elif ACCEL_STRUCT == 1

	typedef struct {
		float4 bbMin; // bbMin.w = leftChild
		float4 bbMax; // bbMax.w = rightChild
	} bvhNode;

	typedef struct {
		float split;
		int4 children; // [left, right, isLeftLeaf, isRightLeaf]
		short axis;
	} kdNonLeaf;

	typedef struct {
		int8 ropes; // [left, right, bottom, top, back, front, facesIndex, numFaces]
		float4 bbMin;
		float4 bbMax;
	} kdLeaf;

#endif


// Color mode: RGB
#if USE_SPECTRAL == 0

	#define MTL_COLOR float4 rgbDiff; float4 rgbSpec;

// Color mode: SPD
#elif USE_SPECTRAL == 1

	#define MTL_COLOR ushort2 spd;

#endif


// Schlick
#if BRDF == 0

	typedef struct {
		float d;
		float Ni;
		float p;
		float rough;
		MTL_COLOR
		char light
	} material;

// Shirley-Ashikhmin
#elif BRDF == 1

	typedef struct {
		float nu;
		float nv;
		float Rs;
		float Rd;
		float d;
		float Ni;
		MTL_COLOR
		char light
	} material;

#endif
