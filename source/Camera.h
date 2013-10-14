#ifndef CAMERA_H
#define CAMERA_H

#include <cmath>
#include <glm/glm.hpp>
#include <vector>

#include "Cfg.h"
#include "qt/GLWidget.h"
#include "utils.h"


typedef struct {
	float eyeX, eyeY, eyeZ;
	float centerX, centerY, centerZ;
	float upX, upY, upZ;
	float rightX, rightY, rightZ;
	float rotX, rotY;
} camera_t;


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
		void copy( Camera* camera );
		glm::vec3 getAdjustedCenter_glmVec3();
		glm::vec3 getCenter_glmVec3();
		std::vector<float> getEye();
		glm::vec3 getEye_glmVec3();
		float getRotX();
		float getRotY();
		glm::vec3 getUp_glmVec3();
		void setSpeed( float speed );
		void updateCameraRot( int moveX, int moveY );

	protected:
		void updateParent();

	private:
		float mCameraSpeed;
		GLWidget* mParent;
		camera_t mCamera;

};

#endif
