#include "Cfg.h"


const char* Cfg::ACCEL_STRUCT = "accel_struct";
const char* Cfg::BVH_MAXFACES = "bvh.max_faces";
const char* Cfg::BVH_SAHFACESLIMIT = "bvh.sah_faces_limit";
const char* Cfg::BVH_SPATIALSPLITS = "bvh.spatial_splits";
const char* Cfg::CAM_CENTER_X = "camera.center.x";
const char* Cfg::CAM_CENTER_Y = "camera.center.y";
const char* Cfg::CAM_CENTER_Z = "camera.center.z";
const char* Cfg::CAM_EYE_X = "camera.eye.x";
const char* Cfg::CAM_EYE_Y = "camera.eye.y";
const char* Cfg::CAM_EYE_Z = "camera.eye.z";
const char* Cfg::CAM_LENSE_APERTURE = "camera.thin_lense.aperture";
const char* Cfg::CAM_LENSE_FOCALLENGTH = "camera.thin_lense.focal_length";
const char* Cfg::CAM_SPEED = "camera.speed";
const char* Cfg::IMPORT_PATH = "import_path";
const char* Cfg::INFO_KERNELTIMES = "info.kernel_times";
const char* Cfg::LOG_LEVEL = "logging.level";
const char* Cfg::OPENCL_BUILDOPTIONS = "opencl.build_options";
const char* Cfg::OPENCL_CHECKERRORS = "opencl.check_errors";
const char* Cfg::OPENCL_LOCALGROUPSIZE = "opencl.localgroupsize";
const char* Cfg::OPENCL_PROGRAM = "opencl.program";
const char* Cfg::PERS_FOV = "camera.perspective.fov";
const char* Cfg::PERS_ZFAR = "camera.perspective.zfar";
const char* Cfg::PERS_ZNEAR = "camera.perspective.znear";
const char* Cfg::RENDER_ANTIALIAS = "render.antialiasing";
const char* Cfg::RENDER_BRDF = "render.brdf";
const char* Cfg::RENDER_INTERVAL = "render.interval";
const char* Cfg::RENDER_MAXADDEDDEPTH = "render.max_added_depth";
const char* Cfg::RENDER_MAXDEPTH = "render.max_depth";
const char* Cfg::RENDER_PHONGTESS = "render.phong_tessellation";
const char* Cfg::RENDER_SAMPLES = "render.samples";
const char* Cfg::RENDER_SHADOWRAYS = "render.shadow_rays";
const char* Cfg::SHADER_NAME = "shader.name";
const char* Cfg::SHADER_PATH = "shader.path";
const char* Cfg::WINDOW_HEIGHT = "window.height";
const char* Cfg::WINDOW_WIDTH = "window.width";


/**
 * Load the config file (JSON).
 * @param {const char*} filepath File path.
 */
void Cfg::loadConfigFile( const char* filepath ) {
	boost::property_tree::json_parser::read_json( filepath, mPropTree );
}
