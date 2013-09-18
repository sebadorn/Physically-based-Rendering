#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <sstream>
#include <QGLWidget>
#include <unistd.h>

#include "../Camera.h"
#include "../CL.h"
#include "../Logger.h"
#include "../utils.h"
#include "Window.h"

#ifndef GL_MULTISAMPLE
	#define GL_MULTISAMPLE 0x809D
#endif

#define RENDER_INTERVAL 16.666f


class GLWidget : public QGLWidget {

	Q_OBJECT

	public:
		GLWidget( QWidget* parent );
		~GLWidget();
		bool isRendering();
		void loadModel( std::string filepath, std::string filename );
		QSize minimumSizeHint() const;
		QSize sizeHint() const;
		void startRendering();
		void stopRendering();

		Camera* mCamera;

	protected:
		void createBufferColors( GLuint* buffer, aiMesh* mesh );
		void createBufferIndices( aiMesh* mesh );
		void createBufferNormals( GLuint* buffer, aiMesh* mesh );
		void createBufferVertices( GLuint* buffer, aiMesh* mesh );
		void drawScene();
		void initializeGL();
		void initShaders();
		void loadShader( GLuint shader, std::string path );
		void paintGL();
		void resizeGL( int width, int height );
		void showFPS();

	private:
		bool mDoRendering;
		uint mFrameCount;
		uint mPreviousTime;
		GLuint mGLProgram;

		const aiScene* mScene;
		CL* mCl;
		QTimer* mTimer;

		std::vector<GLuint> mIndexCount;
		std::vector<GLuint> mVA;
		Assimp::Importer mImporter;

};

#endif
