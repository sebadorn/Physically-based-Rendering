#include "ActionHandler.h"
#include "ModelLoader.h"
#include "ObjParser.h"
#include "accelstructures/BVH.h"


using std::string;


/**
 * Trigger closing the window after which the application will exit.
 * @param {VulkanHandler*} vh
 */
void ActionHandler::exit( VulkanHandler* vh ) {
	glfwSetWindowShouldClose( vh->mWindow, GLFW_TRUE );
}


/**
 * Load a model.
 * @param {const string&} filepath
 * @param {const string&} filename
 */
void ActionHandler::loadModel( const string& filepath, const string& filename ) {
	ModelLoader* ml = new ModelLoader();
	ml->loadModel( filepath, filename );

	ObjParser* op = ml->getObjParser();
	AccelStructure* accelStruct = new BVH( op->getObjects(), op->getVertices(), op->getNormals() );
}
