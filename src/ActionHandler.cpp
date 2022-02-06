#include "ActionHandler.h"
#include "ModelLoader.h"
#include "parsers/ObjParser.h"
#include "accelstructures/BVH.h"


using std::string;


/**
 * Trigger closing the window after which the application will exit.
 * @param {PathTracer*} pt
 */
void ActionHandler::exit( PathTracer* pt ) {
	glfwSetWindowShouldClose( pt->mWindow, GLFW_TRUE );
}


/**
 * Load a model.
 * @param {PathTracer*} pt
 * @param {const string&}  filepath
 * @param {const string&}  filename
 */
void ActionHandler::loadModel( PathTracer* pt, const string& filepath, const string& filename ) {
	ModelLoader* ml = new ModelLoader();
	ml->loadModel( filepath, filename );

	ObjParser* op = ml->getObjParser();
	AccelStructure* accelStruct = new BVH( op->getObjects(), op->getVertices(), op->getNormals() );

	pt->mModelRenderer = new ModelRenderer();
	pt->mModelRenderer->setup( pt, op );
	pt->mHasModel = true;
}
