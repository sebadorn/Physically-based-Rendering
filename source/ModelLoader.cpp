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

	for( uint j = 0; j < mesh->mNumVertices; j++ ) {
		ambient[j * 3] = aiAmbient[0];
		ambient[j * 3 + 1] = aiAmbient[1];
		ambient[j * 3 + 2] = aiAmbient[2];
	}

	glBindBuffer( GL_ARRAY_BUFFER, buffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( ambient ), ambient, GL_STATIC_DRAW );
	glVertexAttribPointer( bufferIndex, 3, GL_FLOAT, GL_FALSE, 0, 0 );

	mBufferIndices.color_ambient = bufferIndex;
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

	for( uint j = 0; j < mesh->mNumVertices; j++ ) {
		diffuse[j * 3] = aiDiffuse[0];
		diffuse[j * 3 + 1] = aiDiffuse[1];
		diffuse[j * 3 + 2] = aiDiffuse[2];
	}

	glBindBuffer( GL_ARRAY_BUFFER, buffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( diffuse ), diffuse, GL_STATIC_DRAW );
	glVertexAttribPointer( bufferIndex, 3, GL_FLOAT, GL_FALSE, 0, 0 );

	mBufferIndices.color_diffuse = bufferIndex;
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

	for( uint j = 0; j < mesh->mNumVertices; j++ ) {
		specular[j * 3] = aiSpecular[0];
		specular[j * 3 + 1] = aiSpecular[1];
		specular[j * 3 + 2] = aiSpecular[2];
	}

	glBindBuffer( GL_ARRAY_BUFFER, buffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( specular ), specular, GL_STATIC_DRAW );
	glVertexAttribPointer( bufferIndex, 3, GL_FLOAT, GL_FALSE, 0, 0 );

	mBufferIndices.color_specular = bufferIndex;
}


/**
 * Create and fill a buffer with index data.
 * @param {aiMesh*} mesh The mesh.
 */
void ModelLoader::createBufferIndices( aiMesh* mesh ) {
	uint indices[mesh->mNumFaces * 3];

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

	for( uint j = 0; j < mesh->mNumVertices; j++ ) {
		normals[j * 3] = mesh->mNormals[j].x;
		normals[j * 3 + 1] = mesh->mNormals[j].y;
		normals[j * 3 + 2] = mesh->mNormals[j].z;
	}

	glBindBuffer( GL_ARRAY_BUFFER, buffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( normals ), normals, GL_STATIC_DRAW );
	glVertexAttribPointer( bufferIndex, 3, GL_FLOAT, GL_FALSE, 0, 0 );

	mBufferIndices.normals = bufferIndex;
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

	mBufferIndices.vertices = bufferIndex;
}


/**
 * Get the indices of the created buffers.
 * @return {bufferindices_t} Struct with fields for each created buffer.
 */
bufferindices_t ModelLoader::getBufferIndices() {
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
	const aiScene* scene = mImporter.ReadFile( filepath + filename, mAssimpFlags );

	if( !scene ) {
		Logger::logError( string( "[ModelLoader] Import: " ).append( mImporter.GetErrorString() ) );
		exit( 1 );
	}

	vector<GLuint> vectorArrayIDs = vector<GLuint>();
	mNumIndices = vector<GLuint>();

	for( uint i = 0; i < scene->mNumMeshes; i++ ) {
		aiMesh* mesh = scene->mMeshes[i];
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		GLuint vertexArrayID;
		glGenVertexArrays( 1, &vertexArrayID );
		glBindVertexArray( vertexArrayID );
		vectorArrayIDs.push_back( vertexArrayID );

		GLuint buffers[5];
		glGenBuffers( 5, &buffers[0] );
		GLuint bufferIndex = -1;

		this->createBufferVertices( mesh, buffers[0], ++bufferIndex );
		this->createBufferNormals( mesh, buffers[1], ++bufferIndex );
		this->createBufferColorsAmbient( mesh, material, buffers[2], ++bufferIndex );
		this->createBufferColorsDiffuse( mesh, material, buffers[3], ++bufferIndex );
		this->createBufferColorsSpecular( mesh, material, buffers[4], ++bufferIndex );
		this->createBufferIndices( mesh );
	}

	glBindVertexArray( 0 );


	stringstream sstm;
	sstm << "[ModelLoader] Imported model \"" << filename << "\" of " << scene->mNumMeshes << " meshes.";
	Logger::logInfo( sstm.str() );

	return vectorArrayIDs;
}
