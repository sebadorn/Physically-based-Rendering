#ifndef MODELLOADER_H
#define MODELLOADER_H

#include <algorithm>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <IL/il.h>
#include <map>
#include <string>
#include <vector>

#include "Logger.h"

#define ML_BUFFINDEX_VERTICES 0
#define ML_BUFFINDEX_NORMALS 1
#define ML_BUFFINDEX_AMBIENT 2
#define ML_BUFFINDEX_DIFFUSE 3
#define ML_BUFFINDEX_SPECULAR 4
#define ML_BUFFINDEX_TEXTURES 5


class ModelLoader {

	public:
		ModelLoader();
		ModelLoader( uint assimpFlags );
		std::vector<GLuint> getNumIndices();
		std::map<GLuint, GLuint> getTextureIDs();
		std::vector<GLuint> loadModelIntoBuffers( std::string filepath, std::string filename );

	protected:
		void createBufferColorsAmbient( aiMesh* mesh, aiMaterial* material, GLuint buffer, GLuint bufferIndex );
		void createBufferColorsDiffuse( aiMesh* mesh, aiMaterial* material, GLuint buffer, GLuint bufferIndex );
		void createBufferColorsSpecular( aiMesh* mesh, aiMaterial* material, GLuint buffer, GLuint bufferIndex );
		void createBufferIndices( aiMesh* mesh );
		void createBufferNormals( aiMesh* mesh, GLuint buffer, GLuint bufferIndex );
		void createBufferTextures( aiMesh* mesh, GLuint buffer, GLuint bufferIndex );
		void createBufferVertices( aiMesh* mesh, GLuint buffer, GLuint bufferIndex );
		GLuint loadTexture( std::string filepath, aiMaterial* material, int materialIndex );

	private:
		uint mAssimpFlags;
		std::vector<GLuint> mNumIndices;
		std::map<GLuint, GLuint> mTextureIDs;

};

#endif
