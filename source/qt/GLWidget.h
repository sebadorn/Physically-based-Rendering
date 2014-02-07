#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <algorithm>
#include <ctime>
#include <GL/glew.h>
#include <GL/glut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <map>
#include <vector>
#include <QGLWidget>

#include "../BVH.h"
#include "../Camera.h"
#include "../CL.h"
#include "../Cfg.h"
#include "../Logger.h"
#include "../ModelLoader.h"
#include "../PathTracer.h"
#include "../utils.h"
#include "Window.h"

#ifndef GL_MULTISAMPLE
	#define GL_MULTISAMPLE 0x809D
#endif

// Number of vertex arrays
#define NUM_VA 4
// Vertex array for path tracer texture
#define VA_TRACER 0
// Vertex array for model overlay (for position comparison)
#define VA_OVERLAY 1
// Vertex array for bounding box of the model
#define VA_BOUNDINGBOX 2
// Vertex array for kd-tree visualization
#define VA_KDTREE 3

using std::map;
using std::string;
using std::vector;


class Camera;
class PathTracer;


class GLWidget : public QGLWidget {

	Q_OBJECT

	public:
		GLWidget( QWidget* parent );
		~GLWidget();
		void cameraUpdate();
		CL* getCLObject();
		bool isRendering();
		void loadModel( string filepath, string filename );
		QSize minimumSizeHint() const;
		void moveCamera( const int key );
		QSize sizeHint() const;
		void startRendering();
		void stopRendering();

		static const GLuint ATTRIB_POINTER_VERTEX = 0;
		Camera* mCamera;

	protected:
		void calculateMatrices();
		void checkGLForErrors();
		void checkFramebufferForErrors();
		void deleteOldModel();
		glm::vec3 getEyeRay( glm::mat4 matrix, glm::vec3 eye, float x, float y, float z );
		void initGlew();
		void initializeGL();
		void initShaders();
		void initTargetTexture();
		void loadShader( GLuint program, GLuint shader, string path );
		void paintGL();
		void paintScene();
		void resizeGL( int width, int height );
		void setShaderBuffersForOverlay( vector<GLfloat> vertices, vector<GLuint> indices );
		void setShaderBuffersForBoundingBox( GLfloat* bbox );
		void setShaderBuffersForKdTree( vector<GLfloat> vertices, vector<GLuint> indices );
		void setShaderBuffersForTracer();
		void showFPS();

	protected slots:
		void toggleViewBoundingBox();
		void toggleViewKdTree();
		void toggleViewOverlay();
		void toggleViewTracer();

	private:
		bool mDoRendering;
		bool mViewBoundingBox;
		bool mViewKdTree;
		bool mViewOverlay;
		bool mViewTracer;

		cl_float mFOV;
		cl_float* mBoundingBox;

		GLuint mFrameCount;
		GLuint mGLProgramTracer;
		GLuint mGLProgramSimple;
		GLuint mIndexBuffer;
		GLuint mKdTreeNumIndices;
		GLuint mPreviousTime;

		QTimer* mTimer;
		KdTree* mKdTree;
		PathTracer* mPathTracer;

		vector<GLuint> mNumIndices;
		map<GLuint, GLuint> mTextureIDs;
		vector<GLuint> mVA;
		GLuint mTargetTexture;

		glm::mat4 mModelMatrix;
		glm::mat4 mModelViewProjectionMatrix;
		glm::mat4 mProjectionMatrix;
		glm::mat4 mViewMatrix;

		vector<cl_uint> mFaces;
		vector<cl_float> mNormals;
		vector<cl_float> mTextureOut;
		vector<cl_float> mVertices;

};

#endif
