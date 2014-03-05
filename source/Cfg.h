#ifndef CFG_H
#define CFG_H

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>


class Cfg {

	public:
		static Cfg& get() {
			static Cfg instance;
			return instance;
		}
		void loadConfigFile( const char* filepath );
		template<typename T> T value( const char* key ) {
			return mPropTree.get<T>( key );
		}

		static const char* BVH_MAXFACES;
		static const char* BVH_SAHFACESLIMIT;
		static const char* CAM_CENTER_X;
		static const char* CAM_CENTER_Y;
		static const char* CAM_CENTER_Z;
		static const char* CAM_EYE_X;
		static const char* CAM_EYE_Y;
		static const char* CAM_EYE_Z;
		static const char* CAM_SPEED;
		static const char* IMPORT_PATH;
		static const char* INFO_KERNELTIMES;
		static const char* LOG_LEVEL;
		static const char* RENDER_ANTIALIAS;
		static const char* RENDER_INTERVAL;
		static const char* RENDER_MAXADDEDDEPTH;
		static const char* RENDER_MAXDEPTH;
		static const char* RENDER_SAMPLES;
		static const char* OPENCL_BUILDOPTIONS;
		static const char* OPENCL_CHECKERRORS;
		static const char* OPENCL_PROGRAM;
		static const char* OPENCL_WORKGROUPSIZE;
		static const char* PERS_FOV;
		static const char* PERS_ZFAR;
		static const char* PERS_ZNEAR;
		static const char* SHADER_PATH;
		static const char* SHADER_NAME;
		static const char* SPECTRAL_COLORSYSTEM;
		static const char* WINDOW_HEIGHT;
		static const char* WINDOW_WIDTH;

	private:
		Cfg() {};
		Cfg( Cfg const& );
		void operator=( Cfg const& );

		boost::property_tree::ptree mPropTree;

};

#endif
