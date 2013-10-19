#include "ModelLoader.h"

using namespace std;
using namespace boost::property_tree;


/**
 * Constructor.
 */
ModelLoader::ModelLoader() {
	// Doc: http://assimp.sourceforge.net/lib_html/postprocess_8h.html
	mAssimpFlags = //aiProcess_JoinIdenticalVertices |
	               aiProcess_Triangulate |
	               aiProcess_GenNormals;
	               //aiProcess_SortByPType |
	               //aiProcess_OptimizeMeshes |
	               //aiProcess_OptimizeGraph |
	               //aiProcess_SplitLargeMeshes;
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
 * @param {aiMesh*}     mesh     The mesh.
 * @param {aiMaterial*} material The material of the mesh.
 * @param {GLuint*}     buffer   ID of the buffer.
 */
void ModelLoader::createBufferColorsAmbient( aiMesh* mesh, aiMaterial* material, GLuint buffer ) {
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
	glVertexAttribPointer( ML_BUFFINDEX_AMBIENT, 3, GL_FLOAT, GL_FALSE, 0, 0 );
	glEnableVertexAttribArray( ML_BUFFINDEX_AMBIENT );
}


/**
 * Create and fill a buffer with diffuse color data.
 * @param {aiMesh*}     mesh     The mesh.
 * @param {aiMaterial*} material The material of the mesh.
 * @param {GLuint*}     buffer   ID of the buffer.
 */
void ModelLoader::createBufferColorsDiffuse( aiMesh* mesh, aiMaterial* material, GLuint buffer ) {
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
	glVertexAttribPointer( ML_BUFFINDEX_DIFFUSE, 3, GL_FLOAT, GL_FALSE, 0, 0 );
	glEnableVertexAttribArray( ML_BUFFINDEX_DIFFUSE );
}


/**
 * Create and fill a buffer with shininess data.
 * @param {aiMesh*}     mesh     The mesh.
 * @param {aiMaterial*} material The material of the mesh.
 * @param {GLuint*}     buffer   ID of the buffer.
 */
void ModelLoader::createBufferColorsShininess( aiMesh* mesh, aiMaterial* material, GLuint buffer ) {
	GLfloat aiShininess;
	material->Get( AI_MATKEY_SHININESS, aiShininess );

	GLfloat shininess[mesh->mNumVertices];
	fill_n( shininess, mesh->mNumVertices, aiShininess );

	glBindBuffer( GL_ARRAY_BUFFER, buffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( shininess ), shininess, GL_STATIC_DRAW );
	glVertexAttribPointer( ML_BUFFINDEX_SHININESS, 1, GL_FLOAT, GL_FALSE, 0, 0 );
	glEnableVertexAttribArray( ML_BUFFINDEX_SHININESS );
}


/**
 * Create and fill a buffer with specular color data.
 * @param {aiMesh*}     mesh     The mesh.
 * @param {aiMaterial*} material The material of the mesh.
 * @param {GLuint*}     buffer   ID of the buffer.
 */
void ModelLoader::createBufferColorsSpecular( aiMesh* mesh, aiMaterial* material, GLuint buffer ) {
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
	glVertexAttribPointer( ML_BUFFINDEX_SPECULAR, 3, GL_FLOAT, GL_FALSE, 0, 0 );
	glEnableVertexAttribArray( ML_BUFFINDEX_SPECULAR );
}


/**
 * Create and fill a buffer with index data.
 * @param {aiMesh*} mesh The mesh.
 */
void ModelLoader::createBufferIndices( aiMesh* mesh ) {
	GLuint indices[mesh->mNumFaces * 3];

	for( uint i = 0; i < mesh->mNumFaces; i++ ) {
		const aiFace* face = &mesh->mFaces[i];
		indices[i * 3] = face->mIndices[0];
		indices[i * 3 + 1] = face->mIndices[1];
		indices[i * 3 + 2] = face->mIndices[2];
	}

	glGenBuffers( 1, &mIndexBuffer );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ), indices, GL_STATIC_DRAW );

	mNumIndices.push_back( mesh->mNumFaces * 3 );
}


/**
 * Create and fill a buffer with vertex normal data.
 * @param {aiMesh*} mesh   The mesh.
 * @param {GLuint*} buffer ID of the buffer.
 */
void ModelLoader::createBufferNormals( aiMesh* mesh, GLuint buffer ) {
	GLfloat normals[mesh->mNumVertices * 3];

	for( uint i = 0; i < mesh->mNumVertices; i++ ) {
		normals[i * 3] = mesh->mNormals[i].x;
		normals[i * 3 + 1] = mesh->mNormals[i].y;
		normals[i * 3 + 2] = mesh->mNormals[i].z;
	}

	glBindBuffer( GL_ARRAY_BUFFER, buffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( normals ), normals, GL_STATIC_DRAW );
	glVertexAttribPointer( ML_BUFFINDEX_NORMALS, 3, GL_FLOAT, GL_FALSE, 0, 0 );
	glEnableVertexAttribArray( ML_BUFFINDEX_NORMALS );
}


/**
 * Create and fill a buffer with opacity data.
 * @param {aiMesh*}     mesh     The mesh.
 * @param {aiMaterial*} material The material of the mesh.
 * @param {GLuint*}     buffer   ID of the buffer.
 */
void ModelLoader::createBufferOpacity( aiMesh* mesh, aiMaterial* material, GLuint buffer ) {
	GLfloat aiOpacity;
	material->Get( AI_MATKEY_OPACITY, aiOpacity );

	GLfloat opacity[mesh->mNumVertices];
	fill_n( opacity, mesh->mNumVertices, aiOpacity );

	glBindBuffer( GL_ARRAY_BUFFER, buffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( opacity ), opacity, GL_STATIC_DRAW );
	glVertexAttribPointer( ML_BUFFINDEX_OPACITY, 1, GL_FLOAT, GL_FALSE, 0, 0 );
	glEnableVertexAttribArray( ML_BUFFINDEX_OPACITY );
}


/**
 * Create and fill a buffer with texture data.
 * @param {aiMesh*} mesh   The mesh.
 * @param {GLuint*} buffer ID of the buffer.
 */
void ModelLoader::createBufferTextures( aiMesh* mesh, GLuint buffer ) {
	GLfloat textures[mesh->mNumVertices * 2];

	for( uint i = 0; i < mesh->mNumVertices; i++ ) {
		textures[i * 2] = mesh->mTextureCoords[0][i].x;
		textures[i * 2 + 1] = mesh->mTextureCoords[0][i].y;
	}

	glBindBuffer( GL_ARRAY_BUFFER, buffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( textures ), textures, GL_STATIC_DRAW );
	glVertexAttribPointer( ML_BUFFINDEX_TEXTURES, 2, GL_FLOAT, GL_FALSE, 0, 0 );
	glEnableVertexAttribArray( ML_BUFFINDEX_TEXTURES );
}


/**
 * Create and fill a buffer with vertex data.
 * @param {aiMesh*} mesh   The mesh.
 * @param {GLuint*} buffer ID of the buffer.
 */
void ModelLoader::createBufferVertices( aiMesh* mesh, GLuint buffer ) {
	GLfloat vertices[mesh->mNumVertices * 3];

	for( uint j = 0; j < mesh->mNumVertices; j++ ) {
		vertices[j * 3] = mesh->mVertices[j].x;
		vertices[j * 3 + 1] = mesh->mVertices[j].y;
		vertices[j * 3 + 2] = mesh->mVertices[j].z;
	}

	glBindBuffer( GL_ARRAY_BUFFER, buffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_STATIC_DRAW );
	glVertexAttribPointer( ML_BUFFINDEX_VERTICES, 3, GL_FLOAT, GL_FALSE, 0, 0 );
	glEnableVertexAttribArray( ML_BUFFINDEX_VERTICES );
}


/**
 * Get the ID of the index buffer.
 * @return {GLuint} ID of the index buffer.
 */
GLuint ModelLoader::getIndexBuffer() {
	return mIndexBuffer;
}


/**
 * Get a list of the lights in the scene.
 * @return {std::vector<light_t>} Vector of the lights.
 */
vector<light_t> ModelLoader::getLights() {
	return mLights;
}


/**
 * Get a list of the number of indices per mesh.
 * @return {std::vector<GLuint>} Vector of number of indices for each mesh.
 */
vector<GLuint> ModelLoader::getNumIndices() {
	return mNumIndices;
}


/**
 * Get the texture ID for each vector array ID.
 * @return {std::map<GLuint, GLuint>}
 */
map<GLuint, GLuint> ModelLoader::getTextureIDs() {
	return mTextureIDs;
}


/**
 * Load (custom-defined) lights of the scene.
 * @param {std::string} filepath Path to the file.
 * @param {std::string} filename Name of the OBJ file it belongs to.
 */
void ModelLoader::loadLights( string filepath, string filename ) {
	string lightsfile = filepath + filename.replace( filename.rfind( ".obj" ), 4, ".light" );

	// Does lights file exist?
	ifstream f( lightsfile.c_str() );
	if( !f.good() ) {
		f.close();
		return;
	}
	f.close();

	ptree lightPropTree;
	json_parser::read_json( filepath + filename, lightPropTree );

	ptree::const_iterator end = lightPropTree.end();

	for( ptree::const_iterator iter = lightPropTree.begin(); iter != end; iter++ ) {
		light_t lightx;

		lightx.position[0] = (*iter).second.get<float>( "position.x" );
		lightx.position[1] = (*iter).second.get<float>( "position.y" );
		lightx.position[2] = (*iter).second.get<float>( "position.z" );
		lightx.position[3] = (*iter).second.get<float>( "position.w" );

		lightx.diffuse[0] = (*iter).second.get<float>( "diffuse.r" );
		lightx.diffuse[1] = (*iter).second.get<float>( "diffuse.g" );
		lightx.diffuse[2] = (*iter).second.get<float>( "diffuse.b" );
		lightx.diffuse[3] = (*iter).second.get<float>( "diffuse.a" );

		lightx.specular[0] = (*iter).second.get<float>( "specular.r" );
		lightx.specular[1] = (*iter).second.get<float>( "specular.g" );
		lightx.specular[2] = (*iter).second.get<float>( "specular.b" );
		lightx.specular[3] = (*iter).second.get<float>( "specular.a" );

		lightx.constantAttenuation = (*iter).second.get<float>( "attenuation.constant" );
		lightx.linearAttenuation = (*iter).second.get<float>( "attenuation.linear" );
		lightx.quadraticAttenuation = (*iter).second.get<float>( "attenuation.quadratic" );

		lightx.spotCutoff = (*iter).second.get<float>( "spotlight.cutoff" );
		lightx.spotExponent = (*iter).second.get<float>( "spotlight.exponent" );

		lightx.spotDirection[0] = (*iter).second.get<float>( "spotlight.direction.x" );
		lightx.spotDirection[1] = (*iter).second.get<float>( "spotlight.direction.y" );
		lightx.spotDirection[2] = (*iter).second.get<float>( "spotlight.direction.z" );

		mLights.push_back( lightx );
	}
}


/**
 * Load 3D model.
 * @param  {string}              filepath Path to the file, without file name.
 * @param  {string}              filename Name of the file.
 * @return {std::vector<GLuint>}          Vector of all generated vertex array IDs.
 */
void ModelLoader::loadModel( string filepath, string filename ) {
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile( filepath + filename, mAssimpFlags );

	if( !scene ) {
		Logger::logError( string( "[ModelLoader] Import: " ).append( importer.GetErrorString() ) );
		exit( 1 );
	}


	// vector<GLuint> vectorArrayIDs = vector<GLuint>();
	// mNumIndices = vector<GLuint>();
	// mTextureIDs = map<GLuint, GLuint>();

	// GLuint vertexArrayID;
	// glGenVertexArrays( 1, &vertexArrayID );
	// glBindVertexArray( vertexArrayID );

	mIndices = vector<GLuint>();
	mVertices = vector<GLfloat>();
	mNormals = vector<GLfloat>();

	GLfloat bbMin[3];
	GLfloat bbMax[3];

	// GLuint buffers[3];
	// glGenBuffers( 3, &buffers[0] );

	GLfloat x, y, z;

	for( uint i = 0; i < scene->mNumMeshes; i++ ) {
		aiMesh* mesh = scene->mMeshes[i];
		// aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		for( uint j = 0; j < mesh->mNumFaces; j++ ) {
			const aiFace* face = &mesh->mFaces[j];
			mIndices.push_back( face->mIndices[0] );
			mIndices.push_back( face->mIndices[1] );
			mIndices.push_back( face->mIndices[2] );
		}

		for( uint j = 0; j < mesh->mNumVertices; j++ ) {
			x = mesh->mVertices[j].x;
			y = mesh->mVertices[j].y;
			z = mesh->mVertices[j].z;
			mVertices.push_back( x );
			mVertices.push_back( y );
			mVertices.push_back( z );

			if( i == 0 && j == 0 ) {
				bbMin[0] = x; bbMin[1] = y; bbMin[2] = z;
				bbMax[0] = x; bbMax[1] = y; bbMax[2] = z;
			}
			else {
				if( x < bbMin[0] ) { bbMin[0] = x; }
				else if( x > bbMax[0] ) { bbMax[0] = x; }

				if( y < bbMin[1] ) { bbMin[1] = y; }
				else if( y > bbMax[1] ) { bbMax[1] = y; }

				if( z < bbMin[2] ) { bbMin[2] = z; }
				else if( z > bbMax[2] ) { bbMax[2] = z; }
			}
		}

		for( uint j = 0; j < mesh->mNumVertices; j++ ) {
			mNormals.push_back( mesh->mNormals[j].x );
			mNormals.push_back( mesh->mNormals[j].y );
			mNormals.push_back( mesh->mNormals[j].z );
		}
	}

	mBoundingBox.push_back( bbMin[0] );
	mBoundingBox.push_back( bbMin[1] );
	mBoundingBox.push_back( bbMin[2] );
	mBoundingBox.push_back( bbMax[0] );
	mBoundingBox.push_back( bbMax[1] );
	mBoundingBox.push_back( bbMax[2] );

		// this->createBufferColorsAmbient( mesh, material, buffers[2] );
		// this->createBufferColorsDiffuse( mesh, material, buffers[3] );
		// this->createBufferColorsSpecular( mesh, material, buffers[4] );
		// this->createBufferColorsShininess( mesh, material, buffers[5] );
		// this->createBufferOpacity( mesh, material, buffers[6] );

		// if( mesh->HasTextureCoords( 0 ) ) {
		// 	GLuint textureID;
		// 	bool texLoadSuccess = true;

		// 	try {
		// 		textureID = this->loadTexture( filepath, material, mesh->mMaterialIndex );
		// 	}
		// 	catch( int exception ) {
		// 		texLoadSuccess = false;
		// 	}

		// 	if( texLoadSuccess ) {
		// 		mTextureIDs[vertexArrayID] = textureID;
		// 		this->createBufferTextures( mesh, buffers[7] );
		// 	}
		// }

		// this->createBufferIndices( mesh );
	// }

	// glBindVertexArray( 0 );
	// glBindBuffer( GL_ARRAY_BUFFER, 0 );
	// glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

	// this->loadLights( filepath, filename );

	char msg[120];
	snprintf( msg, 120, "[ModelLoader] Imported model \"%s\" of %d meshes.", filename.c_str(), scene->mNumMeshes );
	Logger::logInfo( msg );

	importer.FreeScene();
}


/**
 * Load texture file.
 * @param  {std::string} filepath      Path to the textures.
 * @param  {aiMaterial*} material      Material of the mesh.
 * @param  {int}         materialIndex Index of the material.
 * @return {GLuint}                    ID of the created texture.
 */
GLuint ModelLoader::loadTexture( string filepath, aiMaterial* material, int materialIndex ) {
	ilInit();

	int textureIndex = 0;
	aiString path;
	aiReturn textureFound = material->GetTexture( aiTextureType_DIFFUSE, textureIndex, &path );

	if( textureFound != AI_SUCCESS ) {
		char msg[60];
		snprintf( msg, 60, "[ModelLoader] No diffuse texture found in material %d.", materialIndex );
		Logger::logDebug( msg );
		throw 0;
	}

	// No need to load the file again, we already did that.
	if( mFileToTextureID.count( path.data ) > 0 ) {
		return mFileToTextureID[path.data];
	}


	ILuint imageID;
	ilGenImages( 1, &imageID );

	GLuint textureID;
	glGenTextures( 1, &textureID );

	ilBindImage( imageID );
	ilEnable( IL_ORIGIN_SET );
	ilOriginFunc( IL_ORIGIN_LOWER_LEFT );

	string filename = filepath.append( path.data );
	ILboolean success = ilLoadImage( (ILstring) filename.c_str() );

	if( success ) {
		ilConvertImage( IL_RGBA, IL_UNSIGNED_BYTE );

		glBindTexture( GL_TEXTURE_2D, textureID );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGBA,
			ilGetInteger( IL_IMAGE_WIDTH ), ilGetInteger( IL_IMAGE_HEIGHT ), 0,
			GL_RGBA, GL_UNSIGNED_BYTE, ilGetData()
		);
		glGenerateMipmap( GL_TEXTURE_2D );

		glBindTexture( GL_TEXTURE_2D, 0 );
		mFileToTextureID[path.data] = textureID;

		Logger::logDebug( string( "[ModelLoader] Loaded texture " ).append( path.data ) );
	}

	ilDeleteImages( 1, &imageID );

	if( !success ) {
		Logger::logError( string( "[ModelLoader] Failed to load texture file " ).append( path.data ) );
		throw 1;
	}

	return textureID;
}
