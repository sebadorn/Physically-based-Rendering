#include "Cfg.h"


const char* Cfg::CAM_CENTER_X = "camera.center.x";
const char* Cfg::CAM_CENTER_Y = "camera.center.y";
const char* Cfg::CAM_CENTER_Z = "camera.center.z";
const char* Cfg::CAM_EYE_X = "camera.eye.x";
const char* Cfg::CAM_EYE_Y = "camera.eye.y";
const char* Cfg::CAM_EYE_Z = "camera.eye.z";
const char* Cfg::CAM_SPEED = "camera.speed";
const char* Cfg::IMPORT_PATH = "import_path";
const char* Cfg::LOGGING = "logging";
const char* Cfg::RENDER_ANTIALIAS = "render.antialiasing";
const char* Cfg::RENDER_INTERVAL = "render.interval";
const char* Cfg::OPENCL_PROGRAM = "opencl.program";
const char* Cfg::PERS_FOV = "render.perspective.fov";
const char* Cfg::PERS_ZFAR = "render.perspective.zfar";
const char* Cfg::PERS_ZNEAR = "render.perspective.znear";
const char* Cfg::SHADER_PATH = "shader.path";
const char* Cfg::SHADER_NAME = "shader.name";
const char* Cfg::WINDOW_HEIGHT = "window.height";
const char* Cfg::WINDOW_TITLE = "window.title";
const char* Cfg::WINDOW_WIDTH = "window.width";


/**
 * Load the config file (JSON).
 * @param {const char*} filepath File path.
 */
void Cfg::loadConfigFile( const char* filepath ) {
	boost::property_tree::json_parser::read_json( filepath, mPropTree );
}
