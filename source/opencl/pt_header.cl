#define ACCEL_STRUCT #ACCEL_STRUCT#
#define ANTI_ALIASING #ANTI_ALIASING#
#define BRDF #BRDF#
#define EPSILON5 0.00001f
#define EPSILON7 0.0000001f
#define EPSILON10 0.0000000001f
#define IMG_HEIGHT #IMG_HEIGHT#
#define IMG_WIDTH #IMG_WIDTH#
#define SHADOW_RAYS #SHADOW_RAYS#
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


// Only used inside kernel.
typedef struct {
	float3 origin;
	float3 dir;
	float3 normal;
	float t;
	int hitFace;
} ray4;

// Only used inside kernel.
typedef struct {
	float3 n1; // Normal of plane 1
	float3 n2; // Normal of plane 2
	float o1;  // Distance of plane 1 to the origin
	float o2;  // Distance of plane 2 to the origin
} rayPlanes;

// Passed from outside.
typedef struct {
	float3 eye;
	float3 w;
	float3 u;
	float3 v;
	float2 lense; // x: focal length; y: aperture
} camera;

typedef struct {
	uint4 vertices; // w: material
	uint4 normals;
} face_t;


// BVH
#if ACCEL_STRUCT == 0

	typedef struct {
		float4 bbMin;
		float4 bbMax;
		int4 faces;   // w: either the right sibling or the parent
	} bvhNode;

	typedef struct {
		global const bvhNode* bvh;
		global const face_t* faces;
		global const float4* vertices;
		global const float4* normals;
		float4 debugColor;
	} Scene;

// kd-tree
#elif ACCEL_STRUCT == 1

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

	typedef struct {
		const kdLeaf kdRootNode;
		global const kdNonLeaf* kdNonLeaves;
		global const kdLeaf* kdLeaves;
		global const uint* kdFaces;
		global const face_t* faces;
		global const float4* vertices;
		global const float4* normals;
		float4 debugColor;
	} Scene;

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
		float4 data;
		// data.s0: d
		// data.s1: Ni
		// data.s2: p
		// data.s3: rough
		MTL_COLOR
	} material;

// Shirley-Ashikhmin
#elif BRDF == 1

	typedef struct {
		float8 data;
		// data.s0: d
		// data.s1: Ni
		// data.s2: nu
		// data.s3: nv
		// data.s4: Rs
		// data.s5: Rd
		MTL_COLOR
	} material;

#endif
