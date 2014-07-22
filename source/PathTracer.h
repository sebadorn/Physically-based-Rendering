#ifndef PATH_TRACER_H
#define PATH_TRACER_H

#define GLM_FORCE_RADIANS

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
#include "MtlParser.h"
#include "qt/GLWidget.h"
#include "BVH.h"

using std::vector;


struct face_cl {
	// vertices
	cl_float4 a; // a.w = material index
	cl_float4 b;
	cl_float4 c;
	// vertex normals
	cl_float4 an;
	cl_float4 bn;
	cl_float4 cn;
};

struct bvhNode_cl {
	cl_float4 bbMin; // bbMin.w = leftChild
	cl_float4 bbMax; // bbMax.w = rightChild
	cl_int2 facesInterval; // x = start index; y = number of faces
};

struct material_schlick {
	cl_float d;
	cl_float Ni;
	cl_float p;
	cl_float rough;
	cl_ushort2 spd;
	cl_char light;
};

struct material_shirley_ashikhmin {
	cl_float nu;
	cl_float nv;
	cl_float Rs;
	cl_float Rd;
	cl_float d;
	cl_float Ni;
	cl_ushort2 spd;
	cl_char light;
};


class Camera;
class GLWidget;


class PathTracer {

	public:
		PathTracer( GLWidget* parent );
		~PathTracer();
		vector<cl_float> generateImage();
		void initOpenCLBuffers(
			vector<cl_float> vertices, vector<cl_uint> faces, vector<cl_float> normals,
			ModelLoader* ml, BVH* bvh
		);
		void moveSun( const int key );
		void resetSampleCount();
		void setCamera( Camera* camera );
		void setFOV( cl_float fov );
		void setWidthAndHeight( cl_uint width, cl_uint height );

	protected:
		void clPathTracing( cl_float timeSinceStart );
		void clSetColors( cl_float timeSinceStart );
		cl_float getTimeSinceStart();
		void initArgsKernelPathTracing();
		void initKernelArgs();
		size_t initOpenCLBuffers_BVH( BVH* bvh );
		size_t initOpenCLBuffers_Faces(
			ModelLoader* ml,
			vector<cl_float> vertices, vector<cl_uint> faces, vector<cl_float> normals
		);
		size_t initOpenCLBuffers_Materials( ModelLoader* ml );
		size_t initOpenCLBuffers_Rays();
		size_t initOpenCLBuffers_Textures();
		void updateEyeBuffer();

	private:
		cl_uint mHeight;
		cl_uint mWidth;
		cl_float mFOV;
		cl_uint mSampleCount;
		cl_float4 mSunPos;

		vector<cl_float> mTextureOut;

		cl_kernel mKernelPathTracing;
		cl_kernel mKernelRays;

		cl_mem mBufBVH;
		cl_mem mBufBVHFaces;
		cl_mem mBufFaces;
		cl_mem mBufMaterials;
		cl_mem mBufSPDs;

		cl_mem mBufEye;
		cl_mem mBufTextureIn;
		cl_mem mBufTextureOut;

		GLWidget* mGLWidget;
		Camera* mCamera;
		CL* mCL;
		boost::posix_time::ptime mTimeSinceStart;

};

#endif
