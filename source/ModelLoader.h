#ifndef MODELLOADER_H
#define MODELLOADER_H

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <sstream>
#include <string>
#include <vector>

#include "Logger.h"


typedef struct {
	GLuint color_ambient;
	GLuint color_diffuse;
	GLuint color_specular;
	GLuint textures;
	GLuint normals;
	GLuint vertices;
	bool hasTexture;
} bufferindices_t;


class ModelLoader {

	public:
		ModelLoader();
		ModelLoader( uint assimpFlags );
		std::vector<bufferindices_t> getBufferIndices();
		std::vector<GLuint> getNumIndices();
		std::vector<GLuint> loadModelIntoBuffers( std::string filepath, std::string filename );

	protected:
		void createBufferColorsAmbient( aiMesh* mesh, aiMaterial* material, GLuint buffer, GLuint bufferIndex );
		void createBufferColorsDiffuse( aiMesh* mesh, aiMaterial* material, GLuint buffer, GLuint bufferIndex );
		void createBufferColorsSpecular( aiMesh* mesh, aiMaterial* material, GLuint buffer, GLuint bufferIndex );
		void createBufferIndices( aiMesh* mesh );
		void createBufferNormals( aiMesh* mesh, GLuint buffer, GLuint bufferIndex );
		void createBufferTextures( aiMesh* mesh, GLuint buffer, GLuint bufferIndex );
		void createBufferVertices( aiMesh* mesh, GLuint buffer, GLuint bufferIndex );

	private:
		uint mAssimpFlags;
		std::vector<GLuint> mNumIndices;
		std::vector<bufferindices_t> mBufferIndices;

};

#endif
