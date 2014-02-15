#ifndef PATH_TRACER_H
#define PATH_TRACER_H

#include <algorithm>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <ctime>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <string>
#include <vector>

#include "Camera.h"
#include "CL.h"
#include "Cfg.h"
#include "KdTree.h"
#include "MtlParser.h"

using std::vector;


struct face_cl {
	cl_float4 a; // a.w = material index
	cl_float4 b;
	cl_float4 c;
	cl_float4 an, bn, cn;
};

struct ray4 {
	cl_float4 origin;
	cl_float4 dir;
	cl_float4 normal;
	cl_float t;
	cl_int faceIndex;
};

struct kdNonLeaf_cl {
	cl_float4 split;  // [x, y, z, (cl_int) axis]
	cl_int4 children; // [left, right, isLeftLeaf, isRightLeaf]
};

struct kdLeaf_cl {
	cl_int8 ropes; // [left, right, bottom, top, back, front, facesIndex, numFaces]
	cl_float4 bbMin;
	cl_float4 bbMax;
};

struct bvhNode_cl {
	cl_float4 bbMin;
	cl_float4 bbMax;
	cl_int leftChild;
	cl_int rightChild;
};

struct material_cl_t {
	cl_float d;
	cl_float Ni;
	cl_float gloss;
	cl_ushort spd;
	cl_char illum;
	cl_char light;
};


class Camera;


class PathTracer {

	public:
		PathTracer();
		~PathTracer();
		vector<cl_float> generateImage();
		CL* getCLObject();
		void initOpenCLBuffers(
			vector<cl_float> vertices, vector<cl_uint> faces, vector<cl_float> normals,
			ModelLoader* ml, BVH* bvh
		);
		void resetSampleCount();
		void setCamera( Camera* camera );
		void setFOV( cl_float fov );
		void setWidthAndHeight( cl_uint width, cl_uint height );

	protected:
		void clInitRays( cl_float timeSinceStart );
		void clPathTracing( cl_float timeSinceStart );
		void clSetColors( cl_float timeSinceStart );
		glm::vec3 getJitter();
		cl_float getTimeSinceStart();
		void initArgsKernelPathTracing();
		void initArgsKernelRays();
		void initKernelArgs();
		void initOpenCLBuffers_BVH( BVH* bvh );
		void initOpenCLBuffers_KdTree(
			ModelLoader* ml, BVH* bvh,
			vector<cl_float> vertices, vector<cl_uint> faces, vector<cl_float> normals
		);
		void initOpenCLBuffers_Materials( ModelLoader* ml );
		void initOpenCLBuffers_Rays();
		void initOpenCLBuffers_Textures();
		void kdNodesToVectors(
			vector<cl_uint> faces, vector<cl_uint> objectFaces, vector<kdNode_t> kdNodes,
			vector<cl_uint>* kdFaces, vector<kdNonLeaf_cl>* kdNonLeaves,
			vector<kdLeaf_cl>* kdLeaves, cl_uint2 offset
		);
		void updateEyeBuffer();

	private:
		cl_uint mBounces;
		cl_uint mHeight;
		cl_uint mWidth;
		cl_float mFOV;

		cl_uint mSampleCount;
		cl_float8 mKdRootBB;

		vector<cl_float> mTextureOut;

		cl_kernel mKernelPathTracing;
		cl_kernel mKernelRays;

		cl_mem mBufBVH;
		cl_mem mBufKdNonLeaves;
		cl_mem mBufKdLeaves;
		cl_mem mBufKdFaces;

		cl_mem mBufFaces;
		cl_mem mBufMaterials;
		cl_mem mBufSPDs;

		cl_mem mBufEye;
		cl_mem mBufRays;
		cl_mem mBufTextureIn;
		cl_mem mBufTextureOut;

		Camera* mCamera;
		CL* mCL;
		boost::posix_time::ptime mTimeSinceStart;

};

#endif
