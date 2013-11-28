#ifndef PATH_TRACER_H
#define PATH_TRACER_H

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
			vector<light_t> lights,
			vector<kdNode_t> kdNodes, cl_uint rootIndex
		);
		void resetSampleCount();
		void setCamera( Camera* camera );
		void setFOV( cl_float fov );
		void setWidthAndHeight( cl_uint width, cl_uint height );
		void updateLights( vector<light_t> lights );

	protected:
		void clInitRays();
		void clFindIntersections( cl_float timeSinceStart );
		void clAccumulateColors( cl_float timeSinceStart );
		glm::vec3 getJitter();
		cl_float getTimeSinceStart();
		void initKernelArgs();
		void kdNodesToVectors(
			vector<kdNode_t> kdNodes,
			vector<cl_float>* kdData1, vector<cl_int>* kdData2, vector<cl_int>* kdData3,
			vector<cl_int>* kdRopes
		);
		void updateEyeBuffer();

	private:
		cl_uint mHeight;
		cl_uint mWidth;
		cl_float mFOV;

		cl_uint mKdRootNodeIndex;
		cl_uint mNumBounces;
		cl_uint mSampleCount;

		vector<cl_float> mTextureOut;

		cl_kernel mKernelColors;
		cl_kernel mKernelIntersections;
		cl_kernel mKernelRays;

		cl_mem mBufScFaces;
		cl_mem mBufScVertices;
		cl_mem mBufScNormals;
		cl_mem mBufScFacesVN;

		cl_mem mBufKdNodes;
		cl_mem mBufKdNodeData1;
		cl_mem mBufKdNodeData2;
		cl_mem mBufKdNodeData3;
		cl_mem mBufKdNodeRopes;

		cl_mem mBufMaterialsColorDiffuse;
		cl_mem mBufMaterialToFace;

		cl_mem mBufAccColors;
		cl_mem mBufColorMasks;
		cl_mem mBufEye;
		cl_mem mBufLights;
		cl_mem mBufNormals;
		cl_mem mBufOrigins;
		cl_mem mBufRays;
		cl_mem mBufTextureIn;
		cl_mem mBufTextureOut;

		Camera* mCamera;
		CL* mCL;
		boost::posix_time::ptime mTimeSinceStart;

};

#endif
