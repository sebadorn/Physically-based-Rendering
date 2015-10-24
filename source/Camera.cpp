#include "Camera.h"

using std::vector;


/**
 * Constructor.
 * @param {GLWidget*} parent The parent object where this class is used in.
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
	mCamera.eye.x -= sin( MathHelp::degToRad( mCamera.rot.x ) ) * cos( MathHelp::degToRad( mCamera.rot.y ) ) * mCameraSpeed;
	mCamera.eye.y += sin( MathHelp::degToRad( mCamera.rot.y ) ) * mCameraSpeed;
	mCamera.eye.z += cos( MathHelp::degToRad( mCamera.rot.x ) ) * cos( MathHelp::degToRad( mCamera.rot.y ) ) * mCameraSpeed;
	this->updateParent();
}


/**
 * Move the camera position downward.
 */
void Camera::cameraMoveDown() {
	mCamera.eye.y -= mCameraSpeed;
	this->updateParent();
}


/**
 * Move the camera position forward.
 */
void Camera::cameraMoveForward() {
	mCamera.eye.x += sin( MathHelp::degToRad( mCamera.rot.x ) ) * cos( MathHelp::degToRad( mCamera.rot.y ) ) * mCameraSpeed;
	mCamera.eye.y -= sin( MathHelp::degToRad( mCamera.rot.y ) ) * mCameraSpeed;
	mCamera.eye.z -= cos( MathHelp::degToRad( mCamera.rot.x ) ) * cos( MathHelp::degToRad( mCamera.rot.y ) ) * mCameraSpeed;
	this->updateParent();
}


/**
 * Move the camera position to the left.
 */
void Camera::cameraMoveLeft() {
	mCamera.eye.x -= cos( MathHelp::degToRad( mCamera.rot.x ) ) * mCameraSpeed;
	mCamera.eye.z -= sin( MathHelp::degToRad( mCamera.rot.x ) ) * mCameraSpeed;
	this->updateParent();
}


/**
 * Move the camera position to the right.
 */
void Camera::cameraMoveRight() {
	mCamera.eye.x += cos( MathHelp::degToRad( mCamera.rot.x ) ) * mCameraSpeed;
	mCamera.eye.z += sin( MathHelp::degToRad( mCamera.rot.x ) ) * mCameraSpeed;
	this->updateParent();
}


/**
 * Move the camera position upward.
 */
void Camera::cameraMoveUp() {
	mCamera.eye.y += mCameraSpeed;
	this->updateParent();
}


/**
 * Reset the camera position and rotation.
 */
void Camera::cameraReset() {
	mCamera.eye.x = Cfg::get().value<float>( Cfg::CAM_EYE_X );
	mCamera.eye.y = Cfg::get().value<float>( Cfg::CAM_EYE_Y );
	mCamera.eye.z = Cfg::get().value<float>( Cfg::CAM_EYE_Z );
	mCamera.up.x = 0.0f;
	mCamera.up.y = 1.0f;
	mCamera.up.z = 0.0f;
	mCamera.rot.x = 0.0f;
	mCamera.rot.y = 0.0f;
	this->updateCameraRot( 0, 0 );
	mCamera.center.x = Cfg::get().value<float>( Cfg::CAM_CENTER_X );
	mCamera.center.y = Cfg::get().value<float>( Cfg::CAM_CENTER_Y );
	mCamera.center.z = Cfg::get().value<float>( Cfg::CAM_CENTER_Z );
	mCamera.center = glm::normalize( mCamera.center );
}


/**
 * Return the adjusted center coordinates as GLM 3D vector.
 * @return {glm::vec3} Adjusted center coordinates.
 */
glm::vec3 Camera::getAdjustedCenter_glmVec3() {
	return glm::vec3(
		mCamera.eye.x + mCamera.center.x,
		mCamera.eye.y - mCamera.center.y,
		mCamera.eye.z - mCamera.center.z
	);
}


/**
 * Return the center coordinates as GLM 3D vector.
 * @return {glm::vec3} Center coordinates.
 */
glm::vec3 Camera::getCenter_glmVec3() {
	return mCamera.center;
}


/**
 * Return the eye coordinates as GLM 3D vector.
 * @return {glm::vec3} Eye coordinates.
 */
glm::vec3 Camera::getEye_glmVec3() {
	return mCamera.eye;
}


/**
 * Return the eye coordinates.
 * @return {std::vector<float>} Eye coordinates.
 */
vector<float> Camera::getEye() {
	vector<float> eye;
	eye.push_back( mCamera.eye.x );
	eye.push_back( mCamera.eye.y );
	eye.push_back( mCamera.eye.z );

	return eye;
}


/**
 * Get the X rotation of the camera.
 * @return {float} X rotation.
 */
float Camera::getRotX() {
	return mCamera.rot.x;
}


/**
 * Get the Y rotation of the camera.
 * @return {float} Y rotation.
 */
float Camera::getRotY() {
	return mCamera.rot.y;
}


/**
 * Get the movement speed of the camera.
 * @return {float}
 */
float Camera::getSpeed() {
	return mCameraSpeed;
}


/**
 * Return the up coordinates as GLM 3D vector.
 * @return {glm::vec3} Up coordinates.
 */
glm::vec3 Camera::getUp_glmVec3() {
	return mCamera.up;
}


/**
 * Set the camera speed (distance to cover with one step).
 * @param {float} speed New camera speed.
 */
void Camera::setSpeed( float speed ) {
	mCameraSpeed = speed;
}


/**
 * Update the viewing direction of the camera, triggered by mouse movement.
 * @param moveX {int} moveX Movement (of the mouse) in 2D X direction.
 * @param moveY {int} moveY Movement (of the mouse) in 2D Y direction.
 */
void Camera::updateCameraRot( int moveX, int moveY ) {
	mCamera.rot.x -= moveX;
	mCamera.rot.y -= moveY;

	if( mCamera.rot.x >= 360.0f ) {
		mCamera.rot.x = 0.0f;
	}
	else if( mCamera.rot.x < 0.0f ) {
		mCamera.rot.x = 360.0f;
	}

	if( mCamera.rot.y > 90.0f ) {
		mCamera.rot.y = 90.0f;
	}
	else if( mCamera.rot.y < -90.0f ) {
		mCamera.rot.y = -90.0f;
	}

	mCamera.center.x = sin( MathHelp::degToRad( mCamera.rot.x ) )
		- fabs( sin( MathHelp::degToRad( mCamera.rot.y ) ) )
		* sin( MathHelp::degToRad( mCamera.rot.x ) );
	mCamera.center.y = sin( MathHelp::degToRad( mCamera.rot.y ) );
	mCamera.center.z = cos( MathHelp::degToRad( mCamera.rot.x ) )
		- fabs( sin( MathHelp::degToRad( mCamera.rot.y ) ) )
		* cos( MathHelp::degToRad( mCamera.rot.x ) );

	if( mCamera.center.y == 1.0f ) {
		mCamera.up.x = sin( MathHelp::degToRad( mCamera.rot.x ) );
	}
	else if( mCamera.center.y == -1.0f ) {
		mCamera.up.x = -sin( MathHelp::degToRad( mCamera.rot.x ) );
	}
	else {
		mCamera.up.x = 0.0f;
	}

	mCamera.up.y = ( mCamera.center.y == 1.0f || mCamera.center.y == -1.0f ) ? 0.0f : 1.0f;

	if( mCamera.center.y == 1.0f ) {
		mCamera.up.z = -cos( MathHelp::degToRad( mCamera.rot.x ) );
	}
	else if( mCamera.center.y == -1.0f ) {
		mCamera.up.z = cos( MathHelp::degToRad( mCamera.rot.x ) );
	}
	else {
		mCamera.up.z = 0.0f;
	}

	this->updateParent();
}


/**
 * Inform the parent object that the camera has been changed.
 */
void Camera::updateParent() {
	if( mParent != NULL ) {
		mParent->cameraUpdate();
	}
}
