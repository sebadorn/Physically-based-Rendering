#ifndef CAMERA_H
#define CAMERA_H

#include <cmath>
#include <glm/glm.hpp>
#include <vector>

#include "Cfg.h"
#include "qt/GLWidget.h"
#include "MathHelp.h"

using std::vector;


struct camera_t {
	glm::vec3 eye;
	glm::vec3 center;
	glm::vec3 up;
	glm::vec3 right;
	glm::vec2 rot;
};


class GLWidget;


class Camera {

	public:
		Camera( GLWidget* parent );
		void cameraMoveBackward();
		void cameraMoveDown();
		void cameraMoveForward();
		void cameraMoveLeft();
		void cameraMoveRight();
		void cameraMoveUp();
		void cameraReset();
		glm::vec3 getAdjustedCenter_glmVec3();
		glm::vec3 getCenter_glmVec3();
		vector<float> getEye();
		glm::vec3 getEye_glmVec3();
		float getRotX();
		float getRotY();
		glm::vec3 getUp_glmVec3();
		void setSpeed( float speed );
		void updateCameraRot( int moveX, int moveY );

	protected:
		void updateParent();

	private:
		GLWidget* mParent;
		float mCameraSpeed;
		camera_t mCamera;

};

#endif
