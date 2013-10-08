#include "Camera.h"

using namespace utils;


/**
 * Constructor.
 */
Camera::Camera( GLWidget* parent ) {
	mParent = parent;
	mCameraSpeed = Cfg::get().value<float>( Cfg::CAM_SPEED );
	this->cameraReset();
}


/**
 * Move the camera position backward.
 */
void Camera::cameraMoveBackward() {
	mCamera.eyeX += sin( degToRad( mCamera.rotX ) ) * cos( degToRad( mCamera.rotY ) ) * mCameraSpeed;
	mCamera.eyeY -= sin( degToRad( mCamera.rotY ) ) * mCameraSpeed;
	mCamera.eyeZ -= cos( degToRad( mCamera.rotX ) ) * cos( degToRad( mCamera.rotY ) ) * mCameraSpeed;
	mParent->cameraUpdate();
}


/**
 * Move the camera position downward.
 */
void Camera::cameraMoveDown() {
	mCamera.eyeY -= mCameraSpeed;
	mParent->cameraUpdate();
}


/**
 * Move the camera position forward.
 */
void Camera::cameraMoveForward() {
	mCamera.eyeX -= sin( degToRad( mCamera.rotX ) ) * cos( degToRad( mCamera.rotY ) ) * mCameraSpeed;
	mCamera.eyeY += sin( degToRad( mCamera.rotY ) ) * mCameraSpeed;
	mCamera.eyeZ += cos( degToRad( mCamera.rotX ) ) * cos( degToRad( mCamera.rotY ) ) * mCameraSpeed;
	mParent->cameraUpdate();
}


/**
 * Move the camera position to the left.
 */
void Camera::cameraMoveLeft() {
	mCamera.eyeX += cos( degToRad( mCamera.rotX ) ) * mCameraSpeed;
	mCamera.eyeZ += sin( degToRad( mCamera.rotX ) ) * mCameraSpeed;
	mParent->cameraUpdate();
}


/**
 * Move the camera position to the right.
 */
void Camera::cameraMoveRight() {
	mCamera.eyeX -= cos( degToRad( mCamera.rotX ) ) * mCameraSpeed;
	mCamera.eyeZ -= sin( degToRad( mCamera.rotX ) ) * mCameraSpeed;
	mParent->cameraUpdate();
}


/**
 * Move the camera position upward.
 */
void Camera::cameraMoveUp() {
	mCamera.eyeY += mCameraSpeed;
	mParent->cameraUpdate();
}


/**
 * Reset the camera position and rotation.
 */
void Camera::cameraReset() {
	mCamera.eyeX = 0.0f;
	mCamera.eyeY = 0.0f;
	mCamera.eyeZ = 4.0f;
	mCamera.centerX = 0.0f;
	mCamera.centerY = 0.5f;
	mCamera.centerZ = 0.0f;
	mCamera.upX = 0.0f;
	mCamera.upY = 1.0f;
	mCamera.upZ = 0.0f;
	mCamera.rotX = 0.0f;
	mCamera.rotY = 0.0f;
	mParent->cameraUpdate();
}


/**
 * Return the adjusted center coordinates as GLM 3D vector.
 * @return {glm::vec3} Adjusted center coordinates.
 */
glm::vec3 Camera::getAdjustedCenter_glmVec3() {
	return glm::vec3(
		mCamera.eyeX - mCamera.centerX,
		mCamera.eyeY + mCamera.centerY,
		mCamera.eyeZ + mCamera.centerZ
	);
}


/**
 * Return the center coordinates as GLM 3D vector.
 * @return {glm::vec3} Center coordinates.
 */
glm::vec3 Camera::getCenter_glmVec3() {
	return glm::vec3( mCamera.centerX, mCamera.centerY, mCamera.centerZ );
}


/**
 * Return the eye coordinates as GLM 3D vector.
 * @return {glm::vec3} Eye coordinates.
 */
glm::vec3 Camera::getEye_glmVec3() {
	return glm::vec3( mCamera.eyeX, mCamera.eyeY, mCamera.eyeZ );
}


/**
 * Return the eye coordinates.
 * @return {std::vector<float>} Eye coordinates.
 */
std::vector<float> Camera::getEye() {
	std::vector<float> eye;
	eye.push_back( mCamera.eyeX );
	eye.push_back( mCamera.eyeY );
	eye.push_back( mCamera.eyeZ );

	return eye;
}


/**
 * Return the up coordinates as GLM 3D vector.
 * @return {glm::vec3} Up coordinates.
 */
glm::vec3 Camera::getUp_glmVec3() {
	return glm::vec3( mCamera.upX, mCamera.upY, mCamera.upZ );
}


/**
 * Update the viewing direction of the camera, triggered by mouse movement.
 * @param moveX {int} moveX Movement (of the mouse) in 2D X direction.
 * @param moveY {int} moveY Movement (of the mouse) in 2D Y direction.
 */
void Camera::updateCameraRot( int moveX, int moveY ) {
	mCamera.rotX -= moveX;
	mCamera.rotY += moveY;

	if( mCamera.rotX >= 360.0f ) {
		mCamera.rotX = 0.0f;
	}
	else if( mCamera.rotX < 0.0f ) {
		mCamera.rotX = 360.0f;
	}

	if( mCamera.rotY > 90 ) {
		mCamera.rotY = 90.0f;
	}
	else if( mCamera.rotY < -90.0f ) {
		mCamera.rotY = -90.0f;
	}

	mCamera.centerX = sin( degToRad( mCamera.rotX ) )
		- fabs( sin( degToRad( mCamera.rotY ) ) )
		* sin( degToRad( mCamera.rotX ) );
	mCamera.centerY = sin( degToRad( mCamera.rotY ) );
	mCamera.centerZ = cos( degToRad( mCamera.rotX ) )
		- fabs( sin( degToRad( mCamera.rotY ) ) )
		* cos( degToRad( mCamera.rotX ) );

	if( mCamera.centerY == 1.0f ) {
		mCamera.upX = sin( degToRad( mCamera.rotX ) );
	}
	else if( mCamera.centerY == -1.0f ) {
		mCamera.upX = -sin( degToRad( mCamera.rotX ) );
	}
	else {
		mCamera.upX = 0.0f;
	}

	if( mCamera.centerY == 1.0f || mCamera.centerY == -1.0f ) {
		mCamera.upY = 0.0f;
	}
	else {
		mCamera.upY = 1.0f;
	}

	if( mCamera.centerY == 1.0f ) {
		mCamera.upZ = -cos( degToRad( mCamera.rotX ) );
	}
	else if( mCamera.centerY == -1.0f ) {
		mCamera.upZ = cos( degToRad( mCamera.rotX ) );
	}
	else {
		mCamera.upZ = 0.0f;
	}

	mParent->cameraUpdate();
}
