#include "ModelLoader.h"

using namespace std;


/**
 * Constructor.
 */
ModelLoader::ModelLoader() {
	// Doc: http://assimp.sourceforge.net/lib_html/postprocess_8h.html
	mAssimpFlags = aiProcess_JoinIdenticalVertices |
	               aiProcess_Triangulate |
	               aiProcess_GenNormals |
	               aiProcess_SortByPType |
	               aiProcess_OptimizeMeshes |
	               aiProcess_OptimizeGraph |
	               aiProcess_SplitLargeMeshes;
}


/**
 * Constructor.
 * @param {uint} assimpFlags Flags to set for the ASSIMP importer.
 */
ModelLoader::ModelLoader( uint assimpFlags ) {
	mAssimpFlags = assimpFlags;
}


/**
 * Create and fill a buffer with ambient color data.
 * @param {aiMesh*}     mesh        The mesh.
 * @param {aiMaterial*} material    The material of the mesh.
 * @param {GLuint*}     buffer      ID of the buffer.
 * @param {GLuint}      bufferIndex Index of the buffer.
 */
void ModelLoader::createBufferColorsAmbient( aiMesh* mesh, aiMaterial* material, GLuint buffer, GLuint bufferIndex ) {
	aiColor3D aiAmbient;
	material->Get( AI_MATKEY_COLOR_AMBIENT, aiAmbient );

	GLfloat ambient[mesh->mNumVertices * 3];

	for( uint i = 0; i < mesh->mNumVertices; i++ ) {
		ambient[i * 3] = aiAmbient[0];
		ambient[i * 3 + 1] = aiAmbient[1];
		ambient[i * 3 + 2] = aiAmbient[2];
	}

	glBindBuffer( GL_ARRAY_BUFFER, buffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( ambient ), ambient, GL_STATIC_DRAW );
	glVertexAttribPointer( bufferIndex, 3, GL_FLOAT, GL_FALSE, 0, 0 );
}


/**
 * Create and fill a buffer with diffuse color data.
 * @param {aiMesh*}     mesh        The mesh.
 * @param {aiMaterial*} material    The material of the mesh.
 * @param {GLuint*}     buffer      ID of the buffer.
 * @param {GLuint}      bufferIndex Index of the buffer.
 */
void ModelLoader::createBufferColorsDiffuse( aiMesh* mesh, aiMaterial* material, GLuint buffer, GLuint bufferIndex ) {
	aiColor3D aiDiffuse;
	material->Get( AI_MATKEY_COLOR_DIFFUSE, aiDiffuse );

	GLfloat diffuse[mesh->mNumVertices * 3];

	for( uint i = 0; i < mesh->mNumVertices; i++ ) {
		diffuse[i * 3] = aiDiffuse[0];
		diffuse[i * 3 + 1] = aiDiffuse[1];
		diffuse[i * 3 + 2] = aiDiffuse[2];
	}

	glBindBuffer( GL_ARRAY_BUFFER, buffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( diffuse ), diffuse, GL_STATIC_DRAW );
	glVertexAttribPointer( bufferIndex, 3, GL_FLOAT, GL_FALSE, 0, 0 );
}


/**
 * Create and fill a buffer with specular color data.
 * @param {aiMesh*}     mesh        The mesh.
 * @param {aiMaterial*} material    The material of the mesh.
 * @param {GLuint*}     buffer      ID of the buffer.
 * @param {GLuint}      bufferIndex Index of the buffer.
 */
void ModelLoader::createBufferColorsSpecular( aiMesh* mesh, aiMaterial* material, GLuint buffer, GLuint bufferIndex ) {
	aiColor3D aiSpecular;
	material->Get( AI_MATKEY_COLOR_SPECULAR, aiSpecular );

	GLfloat specular[mesh->mNumVertices * 3];

	for( uint i = 0; i < mesh->mNumVertices; i++ ) {
		specular[i * 3] = aiSpecular[0];
		specular[i * 3 + 1] = aiSpecular[1];
		specular[i * 3 + 2] = aiSpecular[2];
	}

	glBindBuffer( GL_ARRAY_BUFFER, buffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( specular ), specular, GL_STATIC_DRAW );
	glVertexAttribPointer( bufferIndex, 3, GL_FLOAT, GL_FALSE, 0, 0 );
}


/**
 * Create and fill a buffer with index data.
 * @param {aiMesh*} mesh The mesh.
 */
void ModelLoader::createBufferIndices( aiMesh* mesh ) {
	uint indices[mesh->mNumFaces * 3];

	for( uint i = 0; i < mesh->mNumFaces; i++ ) {
		const aiFace* face = &mesh->mFaces[i];
		indices[i * 3] = face->mIndices[0];
		indices[i * 3 + 1] = face->mIndices[1];
		indices[i * 3 + 2] = face->mIndices[2];
	}

	GLuint indexBuffer;
	glGenBuffers( 1, &indexBuffer );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexBuffer );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ), indices, GL_STATIC_DRAW );

	mNumIndices.push_back( mesh->mNumFaces * 3 );
}


/**
 * Create and fill a buffer with vertex normal data.
 * @param {aiMesh*} mesh        The mesh.
 * @param {GLuint*} buffer      ID of the buffer.
 * @param {GLuint}  bufferIndex Index of the buffer.
 */
void ModelLoader::createBufferNormals( aiMesh* mesh, GLuint buffer, GLuint bufferIndex ) {
	GLfloat normals[mesh->mNumVertices * 3];

	for( uint i = 0; i < mesh->mNumVertices; i++ ) {
		normals[i * 3] = mesh->mNormals[i].x;
		normals[i * 3 + 1] = mesh->mNormals[i].y;
		normals[i * 3 + 2] = mesh->mNormals[i].z;
	}

	glBindBuffer( GL_ARRAY_BUFFER, buffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( normals ), normals, GL_STATIC_DRAW );
	glVertexAttribPointer( bufferIndex, 3, GL_FLOAT, GL_FALSE, 0, 0 );
}


/**
 * Create and fill a buffer with texture data.
 * @param {aiMesh*} mesh        The mesh.
 * @param {GLuint*} buffer      ID of the buffer.
 * @param {GLuint}  bufferIndex Index of the buffer.
 */
void ModelLoader::createBufferTextures( aiMesh* mesh, GLuint buffer, GLuint bufferIndex ) {
	GLfloat textures[mesh->mNumVertices * 2];

	for( uint i = 0; i < mesh->mNumVertices; i++ ) {
		textures[i * 2] = mesh->mTextureCoords[0][i].x;
		textures[i * 2 + 1] = mesh->mTextureCoords[0][i].y;
	}

	glBindBuffer( GL_ARRAY_BUFFER, buffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( textures ), textures, GL_STATIC_DRAW );
	glVertexAttribPointer( bufferIndex, 2, GL_FLOAT, GL_FALSE, 0, 0 );
}


/**
 * Create and fill a buffer with vertex data.
 * @param {aiMesh*} mesh        The mesh.
 * @param {GLuint*} buffer      ID of the buffer.
 * @param {GLuint}  bufferIndex Index of the buffer.
 */
void ModelLoader::createBufferVertices( aiMesh* mesh, GLuint buffer, GLuint bufferIndex ) {
	GLfloat vertices[mesh->mNumVertices * 3];

	for( uint j = 0; j < mesh->mNumVertices; j++ ) {
		vertices[j * 3] = mesh->mVertices[j].x;
		vertices[j * 3 + 1] = mesh->mVertices[j].y;
		vertices[j * 3 + 2] = mesh->mVertices[j].z;
	}

	glBindBuffer( GL_ARRAY_BUFFER, buffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_STATIC_DRAW );
	glVertexAttribPointer( bufferIndex, 3, GL_FLOAT, GL_FALSE, 0, 0 );
}


/**
 * Get the indices of the created buffers.
 * @return {vector<bufferindices_t>} Vector of structs with fields for each created buffer.
 */
vector<bufferindices_t> ModelLoader::getBufferIndices() {
	return mBufferIndices;
}


/**
 * Get a list of the number of indices per mesh.
 * @return {std::vector<GLuint>} Vector of number of indices for each mesh.
 */
vector<GLuint> ModelLoader::getNumIndices() {
	return mNumIndices;
}


/**
 * Load 3D model.
 * @param  {string}              filepath Path to the file, without file name.
 * @param  {string}              filename Name of the file.
 * @return {std::vector<GLuint>}          Vector of all generated vertex array IDs.
 */
vector<GLuint> ModelLoader::loadModelIntoBuffers( string filepath, string filename ) {
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile( filepath + filename, mAssimpFlags );

	if( !scene ) {
		Logger::logError( string( "[ModelLoader] Import: " ).append( importer.GetErrorString() ) );
		exit( 1 );
	}

	GLuint genBuffers = 5;

	vector<GLuint> vectorArrayIDs = vector<GLuint>();
	mNumIndices = vector<GLuint>();
	mBufferIndices = vector<bufferindices_t>();

	for( uint i = 0; i < scene->mNumMeshes; i++ ) {
		aiMesh* mesh = scene->mMeshes[i];
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		bufferindices_t bi;

		GLuint vertexArrayID;
		GLuint bufferIndex = -1;
		GLuint buffers[genBuffers];

		glGenVertexArrays( 1, &vertexArrayID );
		glBindVertexArray( vertexArrayID );
		vectorArrayIDs.push_back( vertexArrayID );

		glGenBuffers( genBuffers, &buffers[0] );

		bi.vertices = ++bufferIndex;
		this->createBufferVertices( mesh, buffers[0], bi.vertices );

		bi.normals = ++bufferIndex;
		this->createBufferNormals( mesh, buffers[1], bi.normals );

		bi.color_ambient = ++bufferIndex;
		this->createBufferColorsAmbient( mesh, material, buffers[2], bi.color_ambient );

		bi.color_diffuse = ++bufferIndex;
		this->createBufferColorsDiffuse( mesh, material, buffers[3], bi.color_diffuse );

		bi.color_specular = ++bufferIndex;
		this->createBufferColorsSpecular( mesh, material, buffers[4], bi.color_specular );

		if( mesh->HasTextureCoords( 0 ) ) {
			bi.textures = ++bufferIndex;
			this->createBufferTextures( mesh, buffers[5], bi.textures );
			bi.hasTexture = true;
		}
		else {
			bi.hasTexture = false;
		}

		this->createBufferIndices( mesh );

		mBufferIndices.push_back( bi );
	}

	glBindVertexArray( 0 );

	stringstream sstm;
	sstm << "[ModelLoader] Imported model \"" << filename << "\" of " << scene->mNumMeshes << " meshes.";
	Logger::logInfo( sstm.str() );

	importer.FreeScene();

	return vectorArrayIDs;
}
