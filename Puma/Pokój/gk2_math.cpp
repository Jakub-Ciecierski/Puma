#include "gk2_math.h"

//using namespace gk2;
using namespace DirectX;

float gk2::dot(const DirectX::XMFLOAT3& v1, const DirectX::XMFLOAT3& v2) {
	float val = (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);

	return val;
}

XMFLOAT3 gk2::cross(const DirectX::XMFLOAT3& x, const DirectX::XMFLOAT3& y) {
	XMFLOAT3 ret(x.y * y.z - y.y * x.z,
				x.z * y.x - y.z * x.x,
				x.x * y.y - y.x * x.y);
	return ret;
}

void gk2::normalize(DirectX::XMFLOAT3& v) {
	float length = (v.x * v.x) + (v.y * v.y) + (v.z * v.z);
	length = sqrt(length);

	v.x /= length;
	v.y /= length;
	v.z /= length;
}