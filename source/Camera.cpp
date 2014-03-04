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
	mCamera.eyeX -= sin( MathHelp::degToRad( mCamera.rotX ) ) * cos( MathHelp::degToRad( mCamera.rotY ) ) * mCameraSpeed;
	mCamera.eyeY += sin( MathHelp::degToRad( mCamera.rotY ) ) * mCameraSpeed;
	mCamera.eyeZ += cos( MathHelp::degToRad( mCamera.rotX ) ) * cos( MathHelp::degToRad( mCamera.rotY ) ) * mCameraSpeed;
	this->updateParent();
}


/**
 * Move the camera position downward.
 */
void Camera::cameraMoveDown() {
	mCamera.eyeY -= mCameraSpeed;
	this->updateParent();
}


/**
 * Move the camera position forward.
 */
void Camera::cameraMoveForward() {
	mCamera.eyeX += sin( MathHelp::degToRad( mCamera.rotX ) ) * cos( MathHelp::degToRad( mCamera.rotY ) ) * mCameraSpeed;
	mCamera.eyeY -= sin( MathHelp::degToRad( mCamera.rotY ) ) * mCameraSpeed;
	mCamera.eyeZ -= cos( MathHelp::degToRad( mCamera.rotX ) ) * cos( MathHelp::degToRad( mCamera.rotY ) ) * mCameraSpeed;
	this->updateParent();
}


/**
 * Move the camera position to the left.
 */
void Camera::cameraMoveLeft() {
	mCamera.eyeX -= cos( MathHelp::degToRad( mCamera.rotX ) ) * mCameraSpeed;
	mCamera.eyeZ -= sin( MathHelp::degToRad( mCamera.rotX ) ) * mCameraSpeed;
	this->updateParent();
}


/**
 * Move the camera position to the right.
 */
void Camera::cameraMoveRight() {
	mCamera.eyeX += cos( MathHelp::degToRad( mCamera.rotX ) ) * mCameraSpeed;
	mCamera.eyeZ += sin( MathHelp::degToRad( mCamera.rotX ) ) * mCameraSpeed;
	this->updateParent();
}


/**
 * Move the camera position upward.
 */
void Camera::cameraMoveUp() {
	mCamera.eyeY += mCameraSpeed;
	this->updateParent();
}


/**
 * Reset the camera position and rotation.
 */
void Camera::cameraReset() {
	mCamera.eyeX = Cfg::get().value<float>( Cfg::CAM_EYE_X );
	mCamera.eyeY = Cfg::get().value<float>( Cfg::CAM_EYE_Y );
	mCamera.eyeZ = Cfg::get().value<float>( Cfg::CAM_EYE_Z );
	mCamera.centerX = Cfg::get().value<float>( Cfg::CAM_CENTER_X );
	mCamera.centerY = Cfg::get().value<float>( Cfg::CAM_CENTER_Y );
	mCamera.centerZ = Cfg::get().value<float>( Cfg::CAM_CENTER_Z );
	mCamera.upX = 0.0f;
	mCamera.upY = 1.0f;
	mCamera.upZ = 0.0f;
	mCamera.rotX = 0.0f;
	mCamera.rotY = 0.0f;
	this->updateCameraRot( 0, 0 );
}


/**
 * Return the adjusted center coordinates as GLM 3D vector.
 * @return {glm::vec3} Adjusted center coordinates.
 */
glm::vec3 Camera::getAdjustedCenter_glmVec3() {
	return glm::vec3(
		mCamera.eyeX + mCamera.centerX,
		mCamera.eyeY - mCamera.centerY,
		mCamera.eyeZ - mCamera.centerZ
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
vector<float> Camera::getEye() {
	vector<float> eye;
	eye.push_back( mCamera.eyeX );
	eye.push_back( mCamera.eyeY );
	eye.push_back( mCamera.eyeZ );

	return eye;
}


/**
 * Get the X rotation of the camera.
 * @return {float} X rotation.
 */
float Camera::getRotX() {
	return mCamera.rotX;
}


/**
 * Get the Y rotation of the camera.
 * @return {float} Y rotation.
 */
float Camera::getRotY() {
	return mCamera.rotY;
}


/**
 * Return the up coordinates as GLM 3D vector.
 * @return {glm::vec3} Up coordinates.
 */
glm::vec3 Camera::getUp_glmVec3() {
	return glm::vec3( mCamera.upX, mCamera.upY, mCamera.upZ );
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
	mCamera.rotX -= moveX;
	mCamera.rotY -= moveY;

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

	mCamera.centerX = sin( MathHelp::degToRad( mCamera.rotX ) )
		- fabs( sin( MathHelp::degToRad( mCamera.rotY ) ) )
		* sin( MathHelp::degToRad( mCamera.rotX ) );
	mCamera.centerY = sin( MathHelp::degToRad( mCamera.rotY ) );
	mCamera.centerZ = cos( MathHelp::degToRad( mCamera.rotX ) )
		- fabs( sin( MathHelp::degToRad( mCamera.rotY ) ) )
		* cos( MathHelp::degToRad( mCamera.rotX ) );

	if( mCamera.centerY == 1.0f ) {
		mCamera.upX = sin( MathHelp::degToRad( mCamera.rotX ) );
	}
	else if( mCamera.centerY == -1.0f ) {
		mCamera.upX = -sin( MathHelp::degToRad( mCamera.rotX ) );
	}
	else {
		mCamera.upX = 0.0f;
	}

	mCamera.upY = ( mCamera.centerY == 1.0f || mCamera.centerY == -1.0f ) ? 0.0f : 1.0f;

	if( mCamera.centerY == 1.0f ) {
		mCamera.upZ = -cos( MathHelp::degToRad( mCamera.rotX ) );
	}
	else if( mCamera.centerY == -1.0f ) {
		mCamera.upZ = cos( MathHelp::degToRad( mCamera.rotX ) );
	}
	else {
		mCamera.upZ = 0.0f;
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
