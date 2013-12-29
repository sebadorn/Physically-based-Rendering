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


struct ray4 {
	cl_float4 origin;
	cl_float4 dir;
	cl_float4 normal;
	cl_float t;
	cl_float shadow;
	cl_int nodeIndex;
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


class Camera;


class PathTracer {

	public:
		PathTracer();
		~PathTracer();
		vector<cl_float> generateImage();
		CL* getCLObject();
		void initOpenCLBuffers(
			vector<cl_float> vertices, vector<cl_uint> faces, vector<cl_float> normals,
			vector<cl_uint> facesVN, vector<cl_int> facesMtl, vector<material_t> materials,
			vector<light_t> lights, vector<kdNode_t> kdNodes, kdNode_t* rootNode
		);
		void resetSampleCount();
		void setCamera( Camera* camera );
		void setFOV( cl_float fov );
		void setWidthAndHeight( cl_uint width, cl_uint height );
		void updateLights( vector<light_t> lights );

	protected:
		void clInitRays( cl_float timeSinceStart );
		void clPathTracing( cl_float timeSinceStart );
		void clSetColors( cl_float timeSinceStart );
		void clShadowTest( cl_float timeSinceStart );
		glm::vec3 getJitter();
		cl_float getTimeSinceStart();
		void initArgsKernelPathTracing();
		void initArgsKernelRays();
		void initArgsKernelSetColors();
		void initArgsKernelShadowTest();
		void initKernelArgs();
		void kdNodesToVectors(
			vector<kdNode_t> kdNodes, vector<cl_float>* kdFaces,
			vector<kdNonLeaf_cl>* kdNonLeaves, vector<kdLeaf_cl>* kdLeaves,
			vector<cl_uint> faces, vector<cl_float> vertices
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
		cl_kernel mKernelSetColors;
		cl_kernel mKernelShadowTest;

		cl_mem mBufScNormals;
		cl_mem mBufScFacesVN;

		cl_mem mBufKdNonLeaves;
		cl_mem mBufKdLeaves;
		cl_mem mBufKdNodeFaces;

		cl_mem mBufMaterialsColorDiffuse;
		cl_mem mBufMaterialToFace;

		cl_mem mBufEye;
		cl_mem mBufLights;
		cl_mem mBufNormals;
		cl_mem mBufRays;
		cl_mem mBufTextureIn;
		cl_mem mBufTextureOut;

		Camera* mCamera;
		CL* mCL;
		boost::posix_time::ptime mTimeSinceStart;

};

#endif
