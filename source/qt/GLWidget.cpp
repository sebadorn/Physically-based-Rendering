#include <assimp/postprocess.h>
#include <cmath>
#include <GL/glew.h>
#include <GL/glut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <QtGui>
#include <unistd.h>

#include "../CL.h"
#include "../utils.h"
#include "Window.h"
#include "GLWidget.h"

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

#define CAM_MOVE_SPEED 0.5f
#define RENDER_INTERVAL 16.666f
#define SHADER "phong_"


/**
 * Constructor.
 * @param {QWidget*} parent Parent QWidget this QWidget is contained in.
 */
GLWidget::GLWidget( QWidget* parent ) : QGLWidget( QGLFormat( QGL::SampleBuffers ), parent ) {
	QGLFormat format( QGL::SampleBuffers );
	format.setVersion( 3, 2 );
	format.setProfile( QGLFormat::CoreProfile );
	QGLWidget( format, parent );

	CL* mCl = new CL();
	Assimp::Importer mImporter;

	mScene = NULL;
	mDoRendering = false;
	mFrameCount = 0;
	mPreviousTime = 0;
	this->cameraReset();

	mTimer = new QTimer( this );
	connect( mTimer, SIGNAL( timeout() ), this, SLOT( update() ) );

	this->startRendering();
}


/**
 * Destructor.
 */
GLWidget::~GLWidget() {
	this->stopRendering();
}


/**
 * Move the camera position backward.
 */
void GLWidget::cameraMoveBackward() {
	mCamera.eyeX += sin( utils::degToRad( mCamera.rotX ) ) * cos( utils::degToRad( mCamera.rotY ) ) * CAM_MOVE_SPEED;
	mCamera.eyeY -= sin( utils::degToRad( mCamera.rotY ) ) * CAM_MOVE_SPEED;
	mCamera.eyeZ -= cos( utils::degToRad( mCamera.rotX ) ) * cos( utils::degToRad( mCamera.rotY ) ) * CAM_MOVE_SPEED;
}


/**
 * Move the camera position downward.
 */
void GLWidget::cameraMoveDown() {
	mCamera.eyeY -= CAM_MOVE_SPEED;
}


/**
 * Move the camera position forward.
 */
void GLWidget::cameraMoveForward() {
	mCamera.eyeX -= sin( utils::degToRad( mCamera.rotX ) ) * cos( utils::degToRad( mCamera.rotY ) ) * CAM_MOVE_SPEED;
	mCamera.eyeY += sin( utils::degToRad( mCamera.rotY ) ) * CAM_MOVE_SPEED;
	mCamera.eyeZ += cos( utils::degToRad( mCamera.rotX ) ) * cos( utils::degToRad( mCamera.rotY ) ) * CAM_MOVE_SPEED;
}


/**
 * Move the camera position to the left.
 */
void GLWidget::cameraMoveLeft() {
	mCamera.eyeX += cos( utils::degToRad( mCamera.rotX ) ) * CAM_MOVE_SPEED;
	mCamera.eyeZ += sin( utils::degToRad( mCamera.rotX ) ) * CAM_MOVE_SPEED;
}


/**
 * Move the camera position to the right.
 */
void GLWidget::cameraMoveRight() {
	mCamera.eyeX -= cos( utils::degToRad( mCamera.rotX ) ) * CAM_MOVE_SPEED;
	mCamera.eyeZ -= sin( utils::degToRad( mCamera.rotX ) ) * CAM_MOVE_SPEED;
}


/**
 * Move the camera position upward.
 */
void GLWidget::cameraMoveUp() {
	mCamera.eyeY += CAM_MOVE_SPEED;
}


/**
 * Reset the camera position and rotation.
 */
void GLWidget::cameraReset() {
	mCamera.eyeX = 1.0f;
	mCamera.eyeY = 0.0f;
	mCamera.eyeZ = 1.0f;
	mCamera.centerX = 0.0f;
	mCamera.centerY = 0.0f;
	mCamera.centerZ = 0.0f;
	mCamera.upX = 0.0f;
	mCamera.upY = 1.0f;
	mCamera.upZ = 0.0f;
	mCamera.rotX = 0.0f;
	mCamera.rotY = 0.0f;
}


/**
 * Draw an axis.
 */
void GLWidget::drawAxis() {
	glBegin( GL_LINES );
	glColor3d( 255, 0, 0 );
	glVertex3f( 0, 0, 0 );
	glVertex3f( 500, 0, 0 );

	glColor3d( 0, 255, 0 );
	glVertex3f( 0, 0, 0 );
	glVertex3f( 0, 500, 0 );

	glColor3d( 0, 0, 255 );
	glVertex3f( 0, 0, 0 );
	glVertex3f( 0, 0, 500 );
	glEnd();
}


/**
 * Draw the main objects of the scene.
 */
void GLWidget::drawScene() {
	if( mScene == NULL ) {
		return;
	}

	for( uint i = 0; i < mVA.size(); i++ ) {
		glBindVertexArray( mVA[i] );
		glEnableVertexAttribArray( 0 ); // Vertices
		glEnableVertexAttribArray( 1 ); // Normals
		glEnableVertexAttribArray( 2 ); // Color: Ambient
		glEnableVertexAttribArray( 3 ); // Color: Diffuse
		glEnableVertexAttribArray( 4 ); // Color: Specular
		glDrawElements( GL_TRIANGLES, mIndexCount[i], GL_UNSIGNED_INT, 0 );
		glBindVertexArray( 0 );
	}
}


/**
 * Initialize OpenGL and start rendering.
 */
void GLWidget::initializeGL() {
	glClearColor( 0.9f, 0.9f, 0.9f, 0.0f );

	glEnable( GL_DEPTH_TEST );
	glEnable( GL_MULTISAMPLE );
	glEnable( GL_LINE_SMOOTH );

	this->initShader();
}


/**
 * Load and compile the shader.
 */
void GLWidget::initShader() {
	GLenum err = glewInit();

	if( err != GLEW_OK ) {
		std::cerr << "* [GLEW] Init failed: " << glewGetErrorString( err ) << std::endl;
		exit( 1 );
	}

	std::cout << "* [GLEW] Using version " << glewGetString( GLEW_VERSION ) << std::endl;

	std::string shaderString;

	mGLProgram = glCreateProgram();
	GLuint vertexShader = glCreateShader( GL_VERTEX_SHADER );
	GLuint fragmentShader = glCreateShader( GL_FRAGMENT_SHADER );

	std::string path = "source/shader/";
	path.append( SHADER );

	shaderString = utils::loadFileAsString( ( path + "vertex.glsl" ).c_str() );
	const GLchar* shaderSourceVertex = shaderString.c_str();
	const GLint shaderLengthVertex = shaderString.size();
	glShaderSource( vertexShader, 1, &shaderSourceVertex, &shaderLengthVertex );
	glCompileShader( vertexShader );

	GLint status;
	char logBuffer[1000];
	glGetShaderiv( vertexShader, GL_COMPILE_STATUS, &status );
	if( status != GL_TRUE ) {
		glGetShaderInfoLog( vertexShader, 1000, 0, logBuffer );
		std::cerr << "* [Vertex shader]" << std::endl;
		std::cerr << logBuffer << std::endl;
	}

	glAttachShader( mGLProgram, vertexShader );

	shaderString = utils::loadFileAsString( ( path + "fragment.glsl" ).c_str() );
	const GLchar* shaderSourceFragment = shaderString.c_str();
	const GLint shaderLengthFragment = shaderString.size();
	glShaderSource( fragmentShader, 1, &shaderSourceFragment, &shaderLengthFragment );
	glCompileShader( fragmentShader );

	glGetShaderiv( fragmentShader, GL_COMPILE_STATUS, &status );
	if( status != GL_TRUE ) {
		glGetShaderInfoLog( fragmentShader, 1000, 0, logBuffer );
		std::cerr << "* [Fragment shader]" << std::endl;
		std::cerr << logBuffer << std::endl;
	}

	glAttachShader( mGLProgram, fragmentShader );

	glLinkProgram( mGLProgram );
	glUseProgram( mGLProgram );
}


/**
 * Check, if QGLWidget is currently rendering.
 * @return {bool} True, if is rendering, false otherwise.
 */
bool GLWidget::isRendering() {
	return ( mDoRendering && mTimer->isActive() );
}


/**
 * Load 3D model.
 * @param {std::string} filepath Path to the file, without file name.
 * @param {std::string} filename Name of the file.
 */
void GLWidget::loadModel( std::string filepath, std::string filename ) {
	const uint flags = aiProcess_JoinIdenticalVertices |
	                   aiProcess_Triangulate |
	                   aiProcess_GenNormals |
	                   aiProcess_SortByPType |
	                   aiProcess_OptimizeMeshes |
	                   aiProcess_OptimizeGraph |
	                   aiProcess_SplitLargeMeshes;
	// Doc: http://assimp.sourceforge.net/lib_html/postprocess_8h.html
	// more: aiProcess_GenSmoothNormals |
	//       aiProcess_FixInfacingNormals |
	//       aiProcess_FindDegenerates |
	//       aiProcess_FindInvalidData

	mScene = mImporter.ReadFile( filepath + filename, flags );

	if( !mScene ) {
		std::cerr << "* [GLWidget] Error importing model: " << mImporter.GetErrorString() << std::endl;
		exit( 1 );
	}


	mVA = std::vector<GLuint>();
	mIndexCount = std::vector<GLuint>();

	for( uint i = 0; i < mScene->mNumMeshes; i++ ) {
		aiMesh* mesh = mScene->mMeshes[i];

		GLuint vertexArrayID;
		glGenVertexArrays( 1, &vertexArrayID );
		glBindVertexArray( vertexArrayID );
		mVA.push_back( vertexArrayID );

		GLuint buffers[5];
		glGenBuffers( 5, &buffers[0] );

		this->createBufferVertices( buffers, mesh );
		this->createBufferNormals( buffers, mesh );
		this->createBufferColors( buffers, mesh );
		this->createBufferIndices( mesh );
	}

	glBindVertexArray( 0 );


	std::cout << "* [GLWidget] Imported model \"" << filename << "\" of " << mScene->mNumMeshes << " meshes." << std::endl;
	this->startRendering();
}


/**
 * Create and fill a buffer with color data.
 * @param {GLuint*} buffers ID of the buffer.
 * @param {aiMesh*} mesh    The mesh.
 */
void GLWidget::createBufferColors( GLuint* buffers, aiMesh* mesh ) {
	aiMaterial* material = mScene->mMaterials[mesh->mMaterialIndex];


	// Ambient

	aiColor3D aiAmbient;
	material->Get( AI_MATKEY_COLOR_AMBIENT, aiAmbient );

	GLfloat ambient[mesh->mNumVertices * 3];

	for( uint j = 0; j < mesh->mNumVertices; j++ ) {
		ambient[j * 3] = aiAmbient[0];
		ambient[j * 3 + 1] = aiAmbient[1];
		ambient[j * 3 + 2] = aiAmbient[2];
	}

	glBindBuffer( GL_ARRAY_BUFFER, buffers[2] );
	glBufferData( GL_ARRAY_BUFFER, sizeof( ambient ), ambient, GL_STATIC_DRAW );
	glVertexAttribPointer( 2, 3, GL_FLOAT, GL_FALSE, 0, 0 );


	// Diffuse

	aiColor3D aiDiffuse;
	material->Get( AI_MATKEY_COLOR_DIFFUSE, aiDiffuse );

	GLfloat diffuse[mesh->mNumVertices * 3];

	for( uint j = 0; j < mesh->mNumVertices; j++ ) {
		diffuse[j * 3] = aiDiffuse[0];
		diffuse[j * 3 + 1] = aiDiffuse[1];
		diffuse[j * 3 + 2] = aiDiffuse[2];
	}

	glBindBuffer( GL_ARRAY_BUFFER, buffers[3] );
	glBufferData( GL_ARRAY_BUFFER, sizeof( diffuse ), diffuse, GL_STATIC_DRAW );
	glVertexAttribPointer( 3, 3, GL_FLOAT, GL_FALSE, 0, 0 );


	// Specular

	aiColor3D aiSpecular;
	material->Get( AI_MATKEY_COLOR_SPECULAR, aiSpecular );

	GLfloat specular[mesh->mNumVertices * 3];

	for( uint j = 0; j < mesh->mNumVertices; j++ ) {
		specular[j * 3] = aiSpecular[0];
		specular[j * 3 + 1] = aiSpecular[1];
		specular[j * 3 + 2] = aiSpecular[2];
	}

	glBindBuffer( GL_ARRAY_BUFFER, buffers[4] );
	glBufferData( GL_ARRAY_BUFFER, sizeof( specular ), specular, GL_STATIC_DRAW );
	glVertexAttribPointer( 4, 3, GL_FLOAT, GL_FALSE, 0, 0 );
}


/**
 * Create and fill a buffer with index data.
 * @param {aiMesh*} mesh The mesh.
 */
void GLWidget::createBufferIndices( aiMesh* mesh ) {
	uint indices[mesh->mNumFaces * 3];
	mIndexCount.push_back( mesh->mNumFaces * 3 );

	for( uint j = 0; j < mesh->mNumFaces; j++ ) {
		const aiFace* face = &mesh->mFaces[j];
		indices[j * 3] = face->mIndices[0];
		indices[j * 3 + 1] = face->mIndices[1];
		indices[j * 3 + 2] = face->mIndices[2];
	}

	GLuint indexBuffer;
	glGenBuffers( 1, &indexBuffer );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexBuffer );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ), indices, GL_STATIC_DRAW );
}


/**
 * Create and fill a buffer with vertex normal data.
 * @param {GLuint*} buffers ID of the buffer.
 * @param {aiMesh*} mesh    The mesh.
 */
void GLWidget::createBufferNormals( GLuint* buffers, aiMesh* mesh ) {
	GLfloat normals[mesh->mNumVertices * 3];

	for( uint j = 0; j < mesh->mNumVertices; j++ ) {
		normals[j * 3] = mesh->mNormals[j].x;
		normals[j * 3 + 1] = mesh->mNormals[j].y;
		normals[j * 3 + 2] = mesh->mNormals[j].z;
	}

	glBindBuffer( GL_ARRAY_BUFFER, buffers[1] );
	glBufferData( GL_ARRAY_BUFFER, sizeof( normals ), normals, GL_STATIC_DRAW );
	glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, 0, 0 );
}


/**
 * Create and fill a buffer with vertex data.
 * @param {GLuint*} buffer ID of the buffer.
 * @param {aiMesh*} mesh   The mesh.
 */
void GLWidget::createBufferVertices( GLuint* buffers, aiMesh* mesh ) {
	GLfloat vertices[mesh->mNumVertices * 3];

	for( uint j = 0; j < mesh->mNumVertices; j++ ) {
		vertices[j * 3] = mesh->mVertices[j].x;
		vertices[j * 3 + 1] = mesh->mVertices[j].y;
		vertices[j * 3 + 2] = mesh->mVertices[j].z;
	}

	glBindBuffer( GL_ARRAY_BUFFER, buffers[0] );
	glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_STATIC_DRAW );
	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, 0 );
}


/**
 * Set a minimum width and height for the QWidget.
 * @return {QSize} Minimum width and height.
 */
QSize GLWidget::minimumSizeHint() const {
	return QSize( 50, 50 );
}


/**
 * Draw the scene.
 */
void GLWidget::paintGL() {
	if( !mDoRendering ) {
		return;
	}

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glm::mat4 viewMatrix = glm::lookAt(
		glm::vec3( mCamera.eyeX, mCamera.eyeY, mCamera.eyeZ ),
		glm::vec3( mCamera.eyeX - mCamera.centerX, mCamera.eyeY + mCamera.centerY, mCamera.eyeZ + mCamera.centerZ ),
		glm::vec3( mCamera.upX, mCamera.upY, mCamera.upZ )
	);
	glm::mat4 projectionMatrix = glm::perspective(
		50.0f, 1000.0f / 600.0f, 0.1f, 400.0f
	);
	glm::mat4 modelMatrix = glm::mat4( 1.0f );
	glm::mat3 normalMatrix = glm::mat3( viewMatrix * modelMatrix );

	GLuint matrixView = glGetUniformLocation( mGLProgram, "viewMatrix" );
	GLuint matrixProjection = glGetUniformLocation( mGLProgram, "projectionMatrix" );
	GLuint matrixModel = glGetUniformLocation( mGLProgram, "modelMatrix" );
	GLuint matrixNormal = glGetUniformLocation( mGLProgram, "normalMatrix" );

	glUniformMatrix4fv( matrixView, 1, GL_FALSE, &viewMatrix[0][0] );
	glUniformMatrix4fv( matrixProjection, 1, GL_FALSE, &projectionMatrix[0][0] );
	glUniformMatrix4fv( matrixModel, 1, GL_FALSE, &modelMatrix[0][0] );
	glUniformMatrix3fv( matrixNormal, 1, GL_FALSE, &normalMatrix[0][0] );

	// this->drawAxis();
	this->drawScene();
	this->showFPS();
}


/**
 * Handle resizing of the QWidget by updating the viewport and perspective.
 * @param {int} width  New width of the QWidget.
 * @param {int} height New height of the QWidget.
 */
void GLWidget::resizeGL( int width, int height ) {
	glViewport( 0, 0, width, height );
}


/**
 * Calculate the current framerate and show it in the status bar.
 */
void GLWidget::showFPS() {
	mFrameCount++;

	uint currentTime = glutGet( GLUT_ELAPSED_TIME );
	uint timeInterval = currentTime - mPreviousTime;

	if( timeInterval > 1000 ) {
		float fps = mFrameCount / (float) timeInterval * 1000.0f;
		mPreviousTime = currentTime;
		mFrameCount = 0;

		char statusText[40];
		snprintf( statusText, 40, "%.2f FPS (%d\u00D7%dpx)", fps, width(), height() );
		( (Window*) parentWidget() )->updateStatus( statusText );
	}
}


/**
 * Set size of the QWidget.
 * @return {QSize} Width and height of the QWidget.
 */
QSize GLWidget::sizeHint() const {
	return QSize( 1000, 600 );
}


/**
 * Start or resume rendering.
 */
void GLWidget::startRendering() {
	if( !mDoRendering ) {
		mDoRendering = true;
		mTimer->start( RENDER_INTERVAL );
	}
}


/**
 * Stop rendering.
 */
void GLWidget::stopRendering() {
	if( mDoRendering ) {
		mDoRendering = false;
		mTimer->stop();
		( (Window*) parentWidget() )->updateStatus( "Stopped." );
	}
}


/**
 * Update the viewing direction of the camera, triggered by mouse movement.
 * @param moveX {int} moveX Movement (of the mouse) in 2D X direction.
 * @param moveY {int} moveY Movement (of the mouse) in 2D Y direction.
 */
void GLWidget::updateCameraRot( int moveX, int moveY ) {
	mCamera.rotX -= moveX;
	mCamera.rotY += moveY;

	if( mCamera.rotX >= 360.0f ) {
		mCamera.rotX = 0.0f;
	}
	else if( mCamera.rotX < 0.0f ) {
		mCamera.rotX = 360.0f;
	}

	if( mCamera.rotY > 90 ) {
		mCamera.rotY = 90.0f;
	}
	else if( mCamera.rotY < -90.0f ) {
		mCamera.rotY = -90.0f;
	}

	mCamera.centerX = sin( utils::degToRad( mCamera.rotX ) )
		- fabs( sin( utils::degToRad( mCamera.rotY ) ) )
		* sin( utils::degToRad( mCamera.rotX ) );
	mCamera.centerY = sin( utils::degToRad( mCamera.rotY ) );
	mCamera.centerZ = cos( utils::degToRad( mCamera.rotX ) )
		- fabs( sin( utils::degToRad( mCamera.rotY ) ) )
		* cos( utils::degToRad( mCamera.rotX ) );

	if( mCamera.centerY == 1.0f ) {
		mCamera.upX = sin( utils::degToRad( mCamera.rotX ) );
	}
	else if( mCamera.centerY == -1.0f ) {
		mCamera.upX = -sin( utils::degToRad( mCamera.rotX ) );
	}
	else {
		mCamera.upX = 0.0f;
	}

	if( mCamera.centerY == 1.0f || mCamera.centerY == -1.0f ) {
		mCamera.upY = 0.0f;
	}
	else {
		mCamera.upY = 1.0f;
	}

	if( mCamera.centerY == 1.0f ) {
		mCamera.upZ = -cos( utils::degToRad( mCamera.rotX ) );
	}
	else if( mCamera.centerY == -1.0f ) {
		mCamera.upZ = cos( utils::degToRad( mCamera.rotX ) );
	}
	else {
		mCamera.upZ = 0.0f;
	}
}
