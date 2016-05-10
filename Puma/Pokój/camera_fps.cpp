#include "camera_fps.h"
#include <math.h>
#include "gk2_math.h"
#include <iostream>

using namespace DirectX;
using namespace gk2;

CameraFPS::CameraFPS()
{
	position.x = 0.0f;
	position.y = 1.0f;
	position.z = 0.0f;

	m_pitch = 0;
	m_yaw = 0;

	sensitivity = 0.00005;
	movementSpeed = 0.1;
}

CameraFPS::~CameraFPS()
{
}

void CameraFPS::moveForward(float speedBoost) {
	position.x += direction.x * movementSpeed * speedBoost;
	position.y += direction.y * movementSpeed * speedBoost;
	position.z += direction.z * movementSpeed * speedBoost;

}

void CameraFPS::moveBackward(float speedBoost) {
	position.x -= direction.x * movementSpeed * speedBoost;
	position.y -= direction.y * movementSpeed * speedBoost;
	position.z -= direction.z * movementSpeed * speedBoost;
}

void CameraFPS::moveLeft(float speedBoost) {
	position.x -= right.x * movementSpeed * speedBoost;
	position.y -= right.y * movementSpeed * speedBoost;
	position.z -= right.z * movementSpeed * speedBoost;
}

void CameraFPS::moveRight(float speedBoost) {
	position.x += right.x * movementSpeed * speedBoost;
	position.y += right.y * movementSpeed * speedBoost;
	position.z += right.z * movementSpeed * speedBoost;
}

void CameraFPS::moveUp(float speedBoost) {
	position.x += up.x * movementSpeed * speedBoost;
	position.y += up.y * movementSpeed * speedBoost;
	position.z += up.z * movementSpeed * speedBoost;
}

void CameraFPS::moveDown(float speedBoost) {
	position.x -= up.x * movementSpeed * speedBoost;
	position.y -= up.y * movementSpeed * speedBoost;
	position.z -= up.z * movementSpeed * speedBoost;
}

void CameraFPS::rotate(float dYaw, float dPitch) {
	m_pitch += dPitch * sensitivity;
	if (m_pitch < -XM_PIDIV2) {
		m_pitch = -XM_PIDIV2;
	}
	if (m_pitch > XM_PIDIV2) {
		m_pitch = XM_PIDIV2;
	}

	m_yaw += dYaw * sensitivity;
	if (m_yaw > XM_PI)
		m_yaw = -XM_PI + (m_yaw - XM_PI);
	else if (m_yaw < -XM_PI)
		m_yaw = XM_PI - (-XM_PI + m_yaw);
}

void CameraFPS::setSensitivity(float val) {
	this->sensitivity = val;
}

float CameraFPS::getSensitivity() {
	return sensitivity;
}

const XMMATRIX& CameraFPS::getViewMatrix() const {
	return this->viewMtx;
}

void CameraFPS::update() {
	XMVECTOR positionVec = XMLoadFloat3(&position);

	// Calculate the rotation matrix
	XMMATRIX mtxRotation = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0);

	// Calculate the new target
	XMVECTOR forwardVec = XMVector3Transform(XMVectorSet(0, 0, 1, 0), mtxRotation);
	XMVECTOR upVec = XMVector3Transform(XMVectorSet(0, 1, 0, 0), mtxRotation);
	XMVECTOR rightVec = XMVector3Transform(XMVectorSet(1, 0, 0, 0), mtxRotation);
	XMVECTOR vecTarget = positionVec + forwardVec;
	
	XMStoreFloat3(&position, positionVec);
	XMStoreFloat3(&direction, forwardVec);
	XMStoreFloat3(&up, upVec);
	XMStoreFloat3(&right, rightVec);

	// View matrix
	viewMtx = XMMatrixLookAtLH(positionVec, vecTarget, upVec);
}