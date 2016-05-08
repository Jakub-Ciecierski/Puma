#ifndef __GK2_ROOM_H_
#define __GK2_ROOM_H_

#include "gk2_applicationBase.h"
#include "gk2_meshLoader.h"
#include "gk2_camera.h"
#include "gk2_phongEffect.h"
#include "gk2_lightShadowEffect.h"
#include "gk2_constantBuffer.h"
#include "particles.h"
#include "camera_fps.h"

#include "common.h"
#include "Mtxlib.h"

using namespace gk2;

struct Edge {
	int vertexIdx1;
	int vertexIdx2;
	int triangleIdx1;
	int triangleIdx2;
};

struct Triangle {
	int vertexIdx1;
	int vertexIdx2;
	int vertexIdx3;
};

struct VertexPosNormal
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
	static const unsigned int LayoutElements = 2;
	static const D3D11_INPUT_ELEMENT_DESC Layout[LayoutElements];
};

class Scene : public ApplicationBase
{
public:
	explicit Scene(HINSTANCE hInstance);
	virtual ~Scene();

	static void* operator new(size_t size);
	static void operator delete(void* ptr);

protected:
	bool LoadContent() override;
	void UnloadContent() override;

	void Update(float dt) override;
	void Render() override;

private:
	//Various D3D constants
	static const unsigned int VB_STRIDE;
	static const unsigned int VB_OFFSET;
	static const unsigned int BS_MASK;

	Mesh floor;
	Mesh ceiling;
	Mesh walls[4];

	Mesh lightSource;
	Mesh cylinder;
	Mesh plate;

	DirectX::XMMATRIX m_projMtx;
	DirectX::XMMATRIX m_mirrorMtx;

	Camera m_camera;
	CameraFPS cameraFPS;

	std::shared_ptr<CBMatrix> m_worldCB;
	std::shared_ptr<CBMatrix> m_viewCB;
	std::shared_ptr<CBMatrix> m_projCB;
	std::shared_ptr<ConstantBuffer<DirectX::XMFLOAT4>> m_lightPosCB;
	std::shared_ptr<ConstantBuffer<DirectX::XMFLOAT4>> m_surfaceColorCB;
	std::shared_ptr<ConstantBuffer<DirectX::XMFLOAT4>> m_cameraPosCB;

	std::shared_ptr<PhongEffect> m_phongEffect;
	std::shared_ptr<LightShadowEffect> m_lightShadowEffect;
	std::shared_ptr<ParticleSystem> m_particles;
	std::shared_ptr<ID3D11InputLayout> m_layout;

	std::shared_ptr<ID3D11RasterizerState> m_rsCullNone;
	std::shared_ptr<ID3D11BlendState> m_bsAlpha;
	std::shared_ptr<ID3D11DepthStencilState> m_dssNoWrite;

	//Depth stencil state used to perform stencil test when drawing mirrored scene
	std::shared_ptr<ID3D11DepthStencilState> m_dssTest;
	std::shared_ptr<ID3D11DepthStencilState> m_dssWrite;
	//Rasterizer state used to define front faces as counter-clockwise, used when drawing mirrored scene
	std::shared_ptr<ID3D11RasterizerState> m_rsCounterClockwise;


	std::vector<DirectX::XMFLOAT3> m_meshVertexPos[6];
	std::vector<Triangle> m_meshTriangles[6];
	std::vector<Edge> m_meshEdges[6];
	DirectX::XMMATRIX m_meshMtx[6];
	std::shared_ptr<ID3D11Buffer> m_vbMesh[6];
	std::shared_ptr<ID3D11Buffer> m_ibMesh[6];

	void LoadMeshPart(std::string filename, int partIdx);
	void InitializeMesh();
	void inverse_kinematics(vector3 pos, vector3 normal, float &a1, float &a2, float &a3, float &a4, float &a5);

	void InitializeConstantBuffers();
	void InitializeCamera();
	void InitializeRenderStates();
	void CreateScene();
	void CreateRoom();
	void UpdateCamera() const;
	void UpdateCamera(const DirectX::XMMATRIX& view) const;
	void UpdateCameraControl();
	void UpdateRobot(float dt);

	void DrawScene(bool mirrored = false);
	void DrawRoom();
	void DrawMesh() const;

	void DrawMirroredScene();
};

#endif __GK2_ROOM_H_