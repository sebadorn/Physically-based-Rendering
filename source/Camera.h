#ifndef CAMERA_H
#define CAMERA_H

#include <cmath>
#include <glm/glm.hpp>

#include "utils.h"

#define CAM_MOVE_SPEED 0.5f


typedef struct {
	float eyeX, eyeY, eyeZ;
	float centerX, centerY, centerZ;
	float upX, upY, upZ;
	float rotX, rotY;
} camera_t;


class Camera {

	public:
		Camera();
		void cameraMoveBackward();
		void cameraMoveDown();
		void cameraMoveForward();
		void cameraMoveLeft();
		void cameraMoveRight();
		void cameraMoveUp();
		void cameraReset();
		glm::vec3 getAdjustedCenter_glmVec3();
		glm::vec3 getCenter_glmVec3();
		glm::vec3 getEye_glmVec3();
		glm::vec3 getUp_glmVec3();
		void updateCameraRot( int moveX, int moveY );

	private:
		camera_t mCamera;

};

#endif
