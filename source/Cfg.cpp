#include "Cfg.h"


const char* Cfg::CAM_CENTER_X = "camera.center.x";
const char* Cfg::CAM_CENTER_Y = "camera.center.y";
const char* Cfg::CAM_CENTER_Z = "camera.center.z";
const char* Cfg::CAM_EYE_X = "camera.eye.x";
const char* Cfg::CAM_EYE_Y = "camera.eye.y";
const char* Cfg::CAM_EYE_Z = "camera.eye.z";
const char* Cfg::CAM_SPEED = "camera.speed";
const char* Cfg::IMPORT_PATH = "import_path";
const char* Cfg::INFO_KERNELTIMES = "info.kernel_times";
const char* Cfg::KDTREE_DEPTH = "kdtree.depth";
const char* Cfg::KDTREE_MINFACES = "kdtree.min_faces";
const char* Cfg::KDTREE_OPTIMIZEROPES = "kdtree.optimize_ropes";
const char* Cfg::LOGGING = "logging";
const char* Cfg::RENDER_ANTIALIAS = "render.antialiasing";
const char* Cfg::RENDER_BACKFACECULLING = "render.backface_culling";
const char* Cfg::RENDER_BOUNCES = "render.bounces";
const char* Cfg::RENDER_INTERVAL = "render.interval";
const char* Cfg::RENDER_SPECULARHIGHLIGHT = "render.specular_highlight";
const char* Cfg::OPENCL_BUILDOPTIONS = "opencl.build_options";
const char* Cfg::OPENCL_CHECKERRORS = "opencl.check_errors";
const char* Cfg::OPENCL_PROGRAM = "opencl.program";
const char* Cfg::OPENCL_WORKGROUPSIZE = "opencl.workgroupsize";
const char* Cfg::PERS_FOV = "camera.perspective.fov";
const char* Cfg::PERS_ZFAR = "camera.perspective.zfar";
const char* Cfg::PERS_ZNEAR = "camera.perspective.znear";
const char* Cfg::SHADER_PATH = "shader.path";
const char* Cfg::SHADER_NAME = "shader.name";
const char* Cfg::SPECTRAL_COLORSYSTEM = "spectral.color_system";
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
