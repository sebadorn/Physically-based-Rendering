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
#include "accelstructures/BVH.h"

using std::vector;


struct camera_cl {
	cl_float3 eye;
	cl_float3 w;
	cl_float3 u;
	cl_float3 v;
	cl_float2 lense;
};

struct face_cl {
	cl_uint4 vertices; // w: material
	cl_uint4 normals;
};

struct light_cl {
	cl_float4 pos;
	cl_float4 rgb;
	cl_float4 data; // x: type
};

struct material_schlick_spd {
	cl_float4 data;
	// data.s0: d
	// data.s1: Ni
	// data.s2: p
	// data.s3: rough
	cl_ushort2 spd;
};

struct material_shirley_ashikhmin_spd {
	cl_float8 data;
	// data.s0: d
	// data.s1: Ni
	// data.s2: nu
	// data.s3: nv
	// data.s4: Rs
	// data.s5: Rd
	cl_ushort2 spd;
};


// BVH

struct bvhNode_cl {
	cl_float4 bbMin; // w: face index
	cl_float4 bbMax; // w: face index or next node to visit
};


class Camera;
class GLWidget;


class PathTracer {

	public:
		PathTracer( GLWidget* parent );
		~PathTracer();
		vector<cl_float> generateImage( vector<cl_float>* textureDebug );
		void initOpenCLBuffers(
			vector<cl_float> vertices, vector<cl_uint> faces, vector<cl_float> normals,
			ModelLoader* ml, AccelStructure* bvh
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
		void initKernelArgs();
		size_t initOpenCLBuffers_BVH( BVH* bvh, ModelLoader* ml, vector<cl_uint> faces );
		size_t initOpenCLBuffers_Faces(
			ModelLoader* ml,
			vector<cl_float> vertices, vector<cl_uint> faces, vector<cl_float> normals
		);
		size_t initOpenCLBuffers_Lights( ModelLoader* ml );
		size_t initOpenCLBuffers_Materials( ModelLoader* ml );
		size_t initOpenCLBuffers_MaterialsSPD( vector<material_t> materials, SpecParser* sp );
		size_t initOpenCLBuffers_Textures();
		void updateEyeBuffer();

	private:
		cl_uint mHeight;
		cl_uint mWidth;
		cl_float mFOV;
		cl_uint mSampleCount;

		vector<cl_float> mTextureOut;

		cl_kernel mKernelPathTracing;
		cl_kernel mKernelRays;

		cl_mem mBufBVH;
		cl_mem mBufBVHFaces;
		cl_mem mBufFaces;
		cl_mem mBufVertices;
		cl_mem mBufNormals;
		cl_mem mBufMaterials;
		cl_mem mBufSPDs;

		camera_cl mStructCam;
		cl_mem mBufTextureIn;
		cl_mem mBufTextureOut;
		cl_mem mBufTextureDebug;

		vector<light_cl> mLights;
		cl_mem mBufLights;

		GLWidget* mGLWidget;
		Camera* mCamera;
		CL* mCL;
		boost::posix_time::ptime mTimeSinceStart;

};

#endif
