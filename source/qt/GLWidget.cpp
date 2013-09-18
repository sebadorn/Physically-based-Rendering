#include "GLWidget.h"

using namespace std;


/**
 * Constructor.
 * @param {QWidget*} parent Parent QWidget this QWidget is contained in.
 */
GLWidget::GLWidget( QWidget* parent ) : QGLWidget( parent ) {
	CL* mCl = new CL();
	Assimp::Importer mImporter;

	mScene = NULL;
	mDoRendering = false;
	mFrameCount = 0;
	mPreviousTime = 0;
	mCamera = new Camera();

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

	this->initShaders();

	Logger::logInfo( string( "[OpenGL] Version " ).append( (char*) glGetString( GL_VERSION ) ) );
	Logger::logInfo( string( "[OpenGL] GLSL " ).append( (char*) glGetString( GL_SHADING_LANGUAGE_VERSION ) ) );
}


/**
 * Load and compile the shader.
 */
void GLWidget::initShaders() {
	GLenum err = glewInit();

	if( err != GLEW_OK ) {
		Logger::logError( string( "[GLEW] Init failed: " ).append( (char*) glewGetErrorString( err ) ) );
		exit( 1 );
	}
	Logger::logInfo( string( "[GLEW] Version " ).append( (char*) glewGetString( GLEW_VERSION ) ) );

	mGLProgram = glCreateProgram();
	GLuint vertexShader = glCreateShader( GL_VERTEX_SHADER );
	GLuint fragmentShader = glCreateShader( GL_FRAGMENT_SHADER );

	this->loadShader( vertexShader, "source/shader/phong.vert" );
	this->loadShader( fragmentShader, "source/shader/phong.frag" );

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
 * @param {string} filepath Path to the file, without file name.
 * @param {string} filename Name of the file.
 */
void GLWidget::loadModel( string filepath, string filename ) {
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
		Logger::logError( string( "[GLWidget] Import: " ).append( mImporter.GetErrorString() ) );
		exit( 1 );
	}


	mVA = vector<GLuint>();
	mIndexCount = vector<GLuint>();

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


	stringstream sstm;
	sstm << "[GLWidget] Imported model \"" << filename << "\" of " << mScene->mNumMeshes << " meshes.";
	Logger::logInfo( sstm.str() );

	this->startRendering();
}


/**
 * Load a GLSL shader and attach it to the program.
 * @param {GLuint}     shader ID of the shader.
 * @param {std:string} path   File path and name.
 */
void GLWidget::loadShader( GLuint shader, string path ) {
	string shaderString = utils::loadFileAsString( path.c_str() );
	const GLchar* shaderSource = shaderString.c_str();
	const GLint shaderLength = shaderString.size();

	glShaderSource( shader, 1, &shaderSource, &shaderLength );
	glCompileShader( shader );

	GLint status;
	char logBuffer[1000];
	glGetShaderiv( shader, GL_COMPILE_STATUS, &status );

	if( status != GL_TRUE ) {
		glGetShaderInfoLog( shader, 1000, 0, logBuffer );
		Logger::logError( string( "[Shader]\n" ).append( logBuffer ) );
		exit( 1 );
	}

	glAttachShader( mGLProgram, shader );
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
		mCamera->getEye_glmVec3(),
		mCamera->getAdjustedCenter_glmVec3(),
		mCamera->getUp_glmVec3()
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
