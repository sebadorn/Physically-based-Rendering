#ANTI_ALIASING#
#BACKFACE_CULLING#
#define BOUNCES #BOUNCES#
#define EPSILON 0.00001f
#define IMG_HEIGHT #IMG_HEIGHT#
#define IMG_WIDTH #IMG_WIDTH#
#define PI_X2 6.28318530718f
#define SPECTRAL_COLORSYSTEM #SPECTRAL_COLORSYSTEM#
#define SPECULAR_HIGHLIGHT #SPECULAR_HIGHLIGHT#
#define WORKGROUPSIZE #WORKGROUPSIZE#
#define WORKGROUPSIZE_HALF #WORKGROUPSIZE_HALF#

constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
constant int MOD_3[6] = { 0, 1, 2, 0, 1, 2 };

typedef struct {
	float4 origin;
	float4 dir;
	float4 normal;
	float t;
	int nodeIndex;
	int faceIndex;
} ray4 __attribute__( ( aligned( 64 ) ) );

typedef struct {
	float4 split;  // [x, y, z, axis]
	int4 children; // [left, right, isLeftLeaf, isRightLeaf]
} kdNonLeaf __attribute__( ( aligned( 32 ) ) );

typedef struct {
	int8 ropes; // [left, right, back, front, bottom, top, facesIndex, numFaces]
	float4 bbMin;
	float4 bbMax;
} kdLeaf __attribute__( ( aligned( 64 ) ) );

typedef struct {
	float d;
	float Ni;
	float Ns;
	float gloss;
	ushort spd;
	char illum;
	char light
} material;

typedef struct {
	int spdMaterial;
	int spdLight;
	float u;
	float D;
	float lambert;
	float u_light;
	float D_light;
	float lambert_light;
} pathPoint;
