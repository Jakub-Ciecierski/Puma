#pragma once

#include <DirectXMath.h>

namespace gk2 {

	float dot(const DirectX::XMFLOAT3& v1, const DirectX::XMFLOAT3& v2);
	
	DirectX::XMFLOAT3 cross(const DirectX::XMFLOAT3& v1, const DirectX::XMFLOAT3& v2);

	void normalize(DirectX::XMFLOAT3& v);
}