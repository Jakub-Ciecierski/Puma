#pragma once

#include <DirectXMath.h>

# define M_PI_2		1.57079632679489661923

class CameraFPS
{
private:
	DirectX::XMFLOAT3 position;

	DirectX::XMFLOAT3 direction;
	DirectX::XMFLOAT3 right;
	DirectX::XMFLOAT3 up;

	float m_pitch;
	float m_yaw;

	// The delta mouse will be multiplied by that value
	float sensitivity;
	// Movement will be multiplied by this value
	float movementSpeed;

	DirectX::XMMATRIX viewMtx;
public:
	CameraFPS();
	~CameraFPS();

	void moveForward(float speedBoost = 1);
	void moveBackward(float speedBoost = 1);

	void moveLeft(float speedBoost = 1);
	void moveRight(float speedBoost = 1);

	void moveUp(float speedBoost = 1);
	void moveDown(float speedBoost = 1);

	void rotate(float dx, float dy);

	void setSensitivity(float);
	float getSensitivity();

	const DirectX::XMMATRIX& getViewMatrix() const;

	void update();
};