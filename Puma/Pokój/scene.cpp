#define _CRT_SECURE_NO_DEPRECATE
#include "scene.h"

#include "gk2_window.h"
#include "gk2_utils.h"
#include <array>
#include "gk2_math.h"

#include "common.h"
#include "Mtxlib.h"

using namespace std;
using namespace gk2;
using namespace DirectX;

const unsigned int Scene::VB_STRIDE = sizeof(VertexPosNormal);
const unsigned int Scene::VB_OFFSET = 0;
const unsigned int Scene::BS_MASK = 0xffffffff;

const XMFLOAT4 LIGHT_POS = XMFLOAT4(-3.0f, 3.0f, 3.0f, 1.0f);
const XMFLOAT4 LIGHT2_POS = XMFLOAT4(3.0f, 2.0f, -3.0f, 1.0f);
const XMFLOAT4 LIGHT_COLOR = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

float BACKGROUND_COLOR_BUFF[4] = { 0.7f, 0.7f, 0.7f, 1.0f };
XMFLOAT4 BACKGROUND_COLOR(BACKGROUND_COLOR_BUFF);

Scene::Scene(HINSTANCE hInstance)
	: ApplicationBase(hInstance), m_camera(0.01f, 100.0f)
{

}

Scene::~Scene()
{

}

void* Scene::operator new(size_t size)
{
	return Utils::New16Aligned(size);
}

void Scene::operator delete(void* ptr)
{
	Utils::Delete16Aligned(ptr);
}

void Scene::InitializeConstantBuffers()
{
	m_projCB = make_shared<CBMatrix>(m_device);
	m_viewCB = make_shared<CBMatrix>(m_device);
	m_worldCB = make_shared<CBMatrix>(m_device);
	m_lightPosCB = make_shared<ConstantBuffer<XMFLOAT4, 2>>(m_device);
	m_lightColorCB = make_shared<ConstantBuffer<XMFLOAT4, 3>>(m_device);
	m_surfaceColorCB = make_shared<ConstantBuffer<XMFLOAT4>>(m_device);
	m_cameraPosCB = make_shared<ConstantBuffer<XMFLOAT4>>(m_device);
}

void Scene::InitializeCamera()
{
	auto s = getMainWindow()->getClientSize();
	auto ar = static_cast<float>(s.cx) / s.cy;
	m_projMtx = XMMatrixPerspectiveFovLH(XM_PIDIV4, ar, 0.01f, 100.0f);
	m_projCB->Update(m_context, m_projMtx);
	m_camera.Zoom(5);
	cameraFPS.update();
	cameraFPS.moveBackward(80.0f);
	cameraFPS.update();
	UpdateCamera();
}

void Scene::LoadMeshPart(string filename, int partIdx)
{
	int count;
	vector<VertexPosNormal> vertices;
	vector<unsigned short> indices;

	FILE *file = fopen(filename.c_str(), "r");

	// Load vertex positions
	fscanf(file, "%d", &count);
	for (int i = 0; i < count; ++i) {
		float x, y, z;
		fscanf(file, "%f %f %f", &x, &y, &z);
		m_meshVertexPos[partIdx].push_back(XMFLOAT3(x, y, z));
	}

	// Load vertices
	fscanf(file, "%d", &count);
	for (int i = 0; i < count; ++i) {
		int idx;
		float x, y, z;
		fscanf(file, "%d %f %f %f", &idx, &x, &y, &z);
		VertexPosNormal vertex;
		vertex.Pos = XMFLOAT3(m_meshVertexPos[partIdx][idx].x, m_meshVertexPos[partIdx][idx].y, m_meshVertexPos[partIdx][idx].z);
		vertex.Normal = XMFLOAT3(x, y, z);
		vertices.push_back(vertex);
		m_meshVertices[partIdx].push_back(vertex);
	}

	// Load triangles
	fscanf(file, "%d", &count);
	for (int i = 0; i < count; ++i) {
		int a, b, c;
		fscanf(file, "%d %d %d", &a, &b, &c);
		indices.push_back(a);
		indices.push_back(b);
		indices.push_back(c);
		m_meshTriangles[partIdx].push_back({ a, b, c });
	}

	// Load edges
	fscanf(file, "%d", &count);
	for (int i = 0; i < count; ++i) {
		int a, b, c, d;
		fscanf(file, "%d %d %d %d", &a, &b, &c, &d);
		m_meshEdges[partIdx].push_back({ a, b, c, d });
	}

	fclose(file);

	m_vbMesh[partIdx] = m_device.CreateVertexBuffer(vertices);
	m_ibMesh[partIdx] = m_device.CreateIndexBuffer(indices);
}

void Scene::InitializeMesh()
{
	LoadMeshPart("assets/mesh1.txt", 0);
	LoadMeshPart("assets/mesh2.txt", 1);
	LoadMeshPart("assets/mesh3.txt", 2);
	LoadMeshPart("assets/mesh4.txt", 3);
	LoadMeshPart("assets/mesh5.txt", 4);
	LoadMeshPart("assets/mesh6.txt", 5);
}

void Scene::CreateScene()
{
	MeshLoader loader(m_device);
	
	CreateRoom();

	cylinder = loader.GetCylinder(100, 100);
	cylinder.setWorldMatrix(
		XMMatrixScaling(1.0f, 2.0f, 1.0f)
		* XMMatrixRotationX(XM_PIDIV4)
		* XMMatrixTranslation(2.0f, -1.0f, -2.0f));
	cylinder.setColor(XMFLOAT4(1.0, 0.0, 0.0, 1.0));

	lightSource = loader.GetSphere(100, 100);
	lightSource.setWorldMatrix(XMMatrixTranslation(LIGHT_POS.x, LIGHT_POS.y, LIGHT_POS.z));
	lightSource.setColor(XMFLOAT4(LIGHT_COLOR));

	plate = loader.GetQuad();
	/*
	XMMATRIX plateWorldMatrix = 
		XMMatrixScaling(1.5f, 2.2f, 1.0f)
		* XMMatrixRotationX(XM_PIDIV4)
		* XMMatrixTranslation(0.0f, 0.7f, 2.0f);
		*/
	// TODO, when palte is rotated, the plate color is not updated corectly
	/*
	XMMATRIX plateWorldMatrix =
		XMMatrixScaling(1.5f, 2.2f, 1.0f)
		* XMMatrixTranslation(0.0f, 1.2f, 2.0f);
		*/
	
	XMMATRIX plateWorldMatrix =
		XMMatrixScaling(1.5f, 1.5f, 1.0f)
		* XMMatrixRotationX(XM_PIDIV4)
		* XMMatrixRotationY(-XM_PIDIV2)
		* XMMatrixTranslation(-1.5f, 0.25f, 0.0f);

	plate.setWorldMatrix(plateWorldMatrix);
	plate.setColor(XMFLOAT4(0.6, 0.3, 0.0, 0.7));
	
	XMVECTOR det;
	m_mirrorMtx = 
		XMMatrixInverse(&det, plateWorldMatrix)
		* XMMatrixScaling(1, 1, -1) * plateWorldMatrix;
}

void Scene::CreateRoom() {
	MeshLoader loader(m_device);
	XMFLOAT4 wallColor(0.8, 0.8, 0.8, 1.0);
	//XMFLOAT4 wallColor(0.32, 0.83, 1.0, 1.0);

	float a = 10.0f;

	floor = loader.GetQuad();
	floor.setWorldMatrix(
		XMMatrixScaling(10.0f, 10.0f, 10.0f)
		* XMMatrixRotationX(XM_PIDIV2)
		* XMMatrixTranslation(0.0f, -1.0f, 0.0f));
	floor.setColor(wallColor);
	
	ceiling = loader.GetQuad();
	ceiling.setWorldMatrix(
		XMMatrixScaling(10.0f, 10.0f, 10.0f)
		* XMMatrixRotationX(1.5*XM_PI)
		* XMMatrixTranslation(0.0f, 9.0f, 0.0f));
	ceiling.setColor(wallColor);

	auto angle = 0;
	walls[0] = loader.GetQuad();
	walls[0].setWorldMatrix(
		XMMatrixScaling(10.0f, 10.0f, 10.0f)
		* XMMatrixRotationY(angle)
		* XMMatrixTranslation(0.0f, a/2 - 1.0f, a/2));
	walls[0].setColor(wallColor);


	walls[1] = loader.GetQuad();
	walls[1].setWorldMatrix(
		XMMatrixScaling(10.0f, 10.0f, 10.0f)
		* XMMatrixRotationY(XM_PI)
		* XMMatrixTranslation(0.0f, a/2 - 1.0f, -a/2));
	walls[1].setColor(wallColor);

	walls[2] = loader.GetQuad();
	walls[2].setWorldMatrix(
		XMMatrixScaling(10.0f, 10.0f, 10.0f)
		* XMMatrixRotationY(XM_PIDIV2)
		* XMMatrixTranslation(a/2, a / 2 - 1.0f, 0));
	walls[2].setColor(wallColor);

	walls[3] = loader.GetQuad();
	walls[3].setWorldMatrix(
		XMMatrixScaling(10.0f, 10.0f, 10.0f)
		* XMMatrixRotationY(-XM_PIDIV2)
		* XMMatrixTranslation(-a / 2, a / 2 - 1.0f, 0));
	walls[3].setColor(wallColor);
}

void Scene::InitializeRenderStates()
{
	// <Mirror>
	D3D11_DEPTH_STENCIL_DESC dssDesc = m_device.DefaultDepthStencilDesc();

	////  DSS WRITE ///
	// Front Face
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS; // Always pass FrontFace
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE; // When pass. replace with the stencil buffer
	dssDesc.StencilEnable = true;

	// Back Faces never pass
	dssDesc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	//dssDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	//Setup depth stencil state for writing
	m_dssWrite = m_device.CreateDepthStencilState(dssDesc);

	////  DSS TEST ///
	//dssDesc.DepthFunc = D3D11_COMPARISON_LESS;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;

	//Setup depth stencil state for testing
	m_dssTest = m_device.CreateDepthStencilState(dssDesc);

	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	m_dssTestNoWrite = m_device.CreateDepthStencilState(dssDesc);

	D3D11_RASTERIZER_DESC rsDesc = m_device.DefaultRasterizerDesc();
	rsDesc.FrontCounterClockwise = true;

	//Set rasterizer state front face to ccw
	m_rsCounterClockwise = m_device.CreateRasterizerState(rsDesc);

	// </Mirror>


	rsDesc = m_device.DefaultRasterizerDesc();
	rsDesc.CullMode = D3D11_CULL_NONE;
	m_rsCullNone = m_device.CreateRasterizerState(rsDesc);

	auto bsDesc = m_device.DefaultBlendDesc();
	/*
	bsDesc.RenderTarget[0].BlendEnable = true;
	bsDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bsDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bsDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	*/
	
	bsDesc.RenderTarget[0].BlendEnable = true;
	bsDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bsDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bsDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bsDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bsDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	bsDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	
	m_bsAlpha = m_device.CreateBlendState(bsDesc);

	dssDesc = m_device.DefaultDepthStencilDesc();
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	m_dssNoWrite = m_device.CreateDepthStencilState(dssDesc);

	// <Shadow>
	D3D11_DEPTH_STENCIL_DESC dss2Desc = m_device.DefaultDepthStencilDesc();
	//Setup depth stencil state for writing
	dss2Desc.StencilEnable = true;
	dss2Desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dss2Desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
	dss2Desc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;
	dss2Desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	m_dsShadowWriteFront = m_device.CreateDepthStencilState(dss2Desc);
	dss2Desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_DECR;
	m_dsShadowWriteBack = m_device.CreateDepthStencilState(dss2Desc);

	//Setup depth stencil state for testing
	dss2Desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dss2Desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	dss2Desc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	dss2Desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	m_dsShadowTest = m_device.CreateDepthStencilState(dss2Desc);
	dss2Desc.FrontFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;
	m_dsShadowTestComplement = m_device.CreateDepthStencilState(dss2Desc);

	auto bs2Desc = m_device.DefaultBlendDesc();
	bs2Desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;
	bs2Desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	bs2Desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	//bs2Desc.RenderTarget[0].BlendOp = false;
	bs2Desc.RenderTarget[0].RenderTargetWriteMask = 0;
	m_bsNoColorWrite = m_device.CreateBlendState(bs2Desc);
	// </Shadow>
}

bool Scene::LoadContent()
{
	InitializeConstantBuffers();
	InitializeCamera();
	InitializeRenderStates();
	InitializeMesh();
	CreateScene();
	m_phongEffect = make_shared<PhongEffect>(m_device, m_layout);
	m_phongEffect->SetProjMtxBuffer(m_projCB);
	m_phongEffect->SetViewMtxBuffer(m_viewCB);
	m_phongEffect->SetWorldMtxBuffer(m_worldCB);
	m_phongEffect->SetLightPosBuffer(m_lightPosCB);
	m_phongEffect->SetLightColorBuffer(m_lightColorCB);
	m_phongEffect->SetSurfaceColorBuffer(m_surfaceColorCB);

	m_particles = make_shared<ParticleSystem>(m_device, XMFLOAT3(-1.3f, -0.6f, -0.14f));
	m_particles->SetViewMtxBuffer(m_viewCB);
	m_particles->SetProjMtxBuffer(m_projCB);
	return true;
}

void Scene::UnloadContent()
{
	for (int i = 0; i < 6; ++i) {
		m_vbMesh[i].reset();
		m_ibMesh[i].reset();
	}

	m_dssWrite.reset();
	m_dssTest.reset();
	m_rsCounterClockwise.reset();
	m_bsAlpha.reset();

	m_worldCB.reset();
	m_viewCB.reset();
	m_projCB.reset();
	m_lightPosCB.reset();
	m_lightColorCB.reset();
	m_surfaceColorCB.reset();
}

void Scene::UpdateCameraControl() {
	static MouseState prevState;
	MouseState currentState;
	KeyboardState keyboardState;

	float mouseBoost = 300.0f;
	//float mouseBoost = 1.0f;

	if (m_keyboard->GetState(keyboardState)) {
		float boost = 1.0f;
		if (keyboardState.isKeyDown(DIK_SPACE)) {
			boost = 3.0f;
			//mouseBoost = 3.0f;
		}
		if (keyboardState.isKeyDown(DIK_W)) {
			cameraFPS.moveForward(boost);
		}
		if (keyboardState.isKeyDown(DIK_S)) {
			cameraFPS.moveBackward(boost);
		}
		if (keyboardState.isKeyDown(DIK_A)) {
			cameraFPS.moveLeft(boost);
		}
		if (keyboardState.isKeyDown(DIK_D)) {
			cameraFPS.moveRight(boost);
		}
		if (keyboardState.isKeyDown(DIK_Q)) {
			cameraFPS.moveUp(boost);
		}
		if (keyboardState.isKeyDown(DIK_E)) {
			cameraFPS.moveDown(boost);
		}
	}

	if (m_mouse->GetState(currentState))
	{
		auto change = true;
		if (prevState.isButtonDown(0))
		{
			auto d = currentState.getMousePositionChange();
			cameraFPS.rotate(d.x * mouseBoost, d.y * mouseBoost);
		}
		else
			change = false;
		prevState = currentState;
		if (change)
			UpdateCamera();
	}
}

void Scene::UpdateCamera() const
{
	const XMMATRIX& view = cameraFPS.getViewMatrix();
	m_viewCB->Update(m_context, view);
	m_cameraPosCB->Update(m_context, m_camera.GetPosition());
}

void Scene::UpdateCamera(const XMMATRIX& view) const
{
	m_viewCB->Update(m_context, view);
	m_cameraPosCB->Update(m_context, m_camera.GetPosition());
}

void Scene::inverse_kinematics(vector3 pos, vector3 normal, float &a1, float &a2, float &a3, float &a4, float &a5)
{
	float l1 = .91f, l2 = .81f, l3 = .33f, dy = .27f, dz = .26f;
	normal.normalize();
	vector3 pos1 = pos + normal * l3;
	float e = sqrtf(pos1.z*pos1.z + pos1.x*pos1.x - dz*dz);
	a1 = atan2(pos1.z, -pos1.x) + atan2(dz, e);
	vector3 pos2(e, pos1.y - dy, .0f);
	a3 = -acosf(min(1.0f, (pos2.x*pos2.x + pos2.y*pos2.y - l1*l1 - l2*l2) / (2.0f*l1*l2)));
	float k = l1 + l2 * cosf(a3), l = l2 * sinf(a3);
	a2 = -atan2(pos2.y, sqrtf(pos2.x*pos2.x + pos2.z*pos2.z)) - atan2(l, k);
	vector4 tmp1(RotateRadMatrix44('y', -a1) * vector4(normal.x, normal.y, normal.z, .0f));
	vector4 tmp2(RotateRadMatrix44('z', -(a2 + a3)) * vector4(tmp1.x, tmp1.y, tmp1.z, .0f));
	vector3 normal1;
	normal1 = vector3(tmp2.x, tmp2.y, tmp2.z);
	a5 = acosf(normal1.x);
	a4 = atan2(normal1.z, normal1.y);
}

void Scene::UpdateRobot(float dtime)
//Update robot parts matrices using inverse kinematics
{
	static float t = 0;
	t += dtime;
	float a1 = XM_PIDIV2 * t;
	float a2 = XM_PIDIV2 / 2 * t;
	float a3 = XM_PIDIV2 / 2 * t;
	float a4 = XM_PIDIV2 / 2 * t;
	float a5 = XM_PIDIV2 / 2 * t;

	vector4 pos = TranslateMatrix44(-1.5, 0.25, 0) * RotateRadMatrix44(vector3(1, 1, 0), t) * vector4(0, 0, 0.5, 1);
	inverse_kinematics(vector3(pos.x, pos.y, pos.z), vector3(1, 1, 0), a1, a2, a3, a4, a5);

	m_meshMtx[0] = XMMatrixIdentity();
	m_meshMtx[1] = XMMatrixRotationY(a1) * m_meshMtx[0];
	m_meshMtx[2] = XMMatrixTranslation(0, -0.27, 0) * XMMatrixRotationZ(a2) * XMMatrixTranslation(0, 0.27, 0) * m_meshMtx[1];
	m_meshMtx[3] = XMMatrixTranslation(0.91f, -0.27f, 0.26f) * XMMatrixRotationZ(a3) * XMMatrixTranslation(-0.91f, 0.27f, -0.26f) * m_meshMtx[2];
	m_meshMtx[4] = XMMatrixTranslation(0, -0.27, 0.26f) * XMMatrixRotationX(a4) * XMMatrixTranslation(0, 0.27, -0.26f) * m_meshMtx[3];
	m_meshMtx[5] = XMMatrixTranslation(1.72f, -0.27f, 0) * XMMatrixRotationZ(a5) * XMMatrixTranslation(-1.72f, 0.27f, 0) * m_meshMtx[4];

	// Update particles
	m_particles->SetEmitterPosition(XMFLOAT3(pos.x, pos.y, pos.z));
	m_particles->Update(m_context, dtime, m_camera.GetPosition());
}

void Scene::Update(float dt)
{
	cameraFPS.update();
	UpdateCameraControl();
	UpdateRobot(dt);
	UpdateShadowGeometry();
}

void Scene::SetLight() const
//Setup one white positional light at the camera
//Setup two additional positional lights, green and blue.
{
	XMFLOAT4 position[2];
	ZeroMemory(position, sizeof(XMFLOAT4) * 2);
	position[0] = LIGHT_POS;
	position[1] = LIGHT2_POS;
	m_lightPosCB->Update(m_context, position);

	XMFLOAT4 colors[3];
	ZeroMemory(colors, sizeof(XMFLOAT4) * 3);
	colors[0] = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f); //ambient color
	colors[1] = XMFLOAT4(1.0f, 0.8f, 1.0f, 200.0f); //surface [ka, kd, ks, m]
	colors[2] = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); //white light color
	m_lightColorCB->Update(m_context, colors);
}

void Scene::OffLight() const
{
	XMFLOAT4 position[2];
	ZeroMemory(position, sizeof(XMFLOAT4) * 2);
	position[0] = LIGHT_POS;
	position[1] = LIGHT2_POS;
	m_lightPosCB->Update(m_context, position);

	XMFLOAT4 colors[3];
	ZeroMemory(colors, sizeof(XMFLOAT4) * 3);
	colors[0] = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f); //ambient color
	colors[1] = XMFLOAT4(1.0f, 0.8f, 1.0f, 200.0f); //surface [ka, kd, ks, m]
	colors[2] = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f); //white light color
	m_lightColorCB->Update(m_context, colors);
}

bool cameraInShadow = false;
XMFLOAT4 v1, v2, v3;
XMFLOAT3 pos1, pos2, inter, inter1, inter2, inter3, inter4, inter5, inter6, dot1, dot2, dot3;
float dotf;

void Scene::UpdateShadowGeometry()
{
	vector<VertexPosNormal> vertices;
	vector<unsigned short> indices;
	vector<vector<VertexTriangle>> triangles;
	m_contourEdges = 0;
	float GO_TO_INFINITY = 1000.0f;
	for (int partIdx = 0; partIdx < 6; ++partIdx) {
		XMMATRIX mat = m_meshMtx[partIdx];
		vector<VertexTriangle> part_triangles;
		for (int i = 0; i < m_meshEdges[partIdx].size(); ++i) {
			auto edge = m_meshEdges[partIdx][i];
			auto v1 = XMVector3Transform(XMLoadFloat3(&m_meshVertexPos[partIdx][edge.vertexIdx1]), mat);
			auto v2 = XMVector3Transform(XMLoadFloat3(&m_meshVertexPos[partIdx][edge.vertexIdx2]), mat);
			auto t1 = m_meshTriangles[partIdx][edge.triangleIdx1];
			auto t2 = m_meshTriangles[partIdx][edge.triangleIdx2];
			auto lightPos = XMLoadFloat4(&LIGHT_POS);
			auto t1v1 = XMVector3Transform(XMLoadFloat3(&m_meshVertices[partIdx][t1.vertexIdx1].Pos), mat);
			auto t1v2 = XMVector3Transform(XMLoadFloat3(&m_meshVertices[partIdx][t1.vertexIdx2].Pos), mat);
			auto t1v3 = XMVector3Transform(XMLoadFloat3(&m_meshVertices[partIdx][t1.vertexIdx3].Pos), mat);
			auto t2v1 = XMVector3Transform(XMLoadFloat3(&m_meshVertices[partIdx][t2.vertexIdx1].Pos), mat);
			auto t2v2 = XMVector3Transform(XMLoadFloat3(&m_meshVertices[partIdx][t2.vertexIdx2].Pos), mat);
			auto t2v3 = XMVector3Transform(XMLoadFloat3(&m_meshVertices[partIdx][t2.vertexIdx3].Pos), mat);
			auto t1lightDir = (t1v1 + t1v2 + t1v3) / 3 - lightPos;
			auto t2lightDir = (t2v1 + t2v2 + t2v3) / 3 - lightPos;
			auto t1normal = XMVector3Normalize(XMVector3Cross(t1v2 - t1v1, t1v3 - t1v2));
			auto t2normal = XMVector3Normalize(XMVector3Cross(t2v2 - t2v1, t2v3 - t2v2));
			auto t1dot = XMVectorGetX(XMVector3Dot(t1lightDir, t1normal));
			auto t2dot = XMVectorGetX(XMVector3Dot(t2lightDir, t2normal));
			bool first = t1dot >= 0.0 && t2dot < 0;
			bool second = t2dot >= 0.0 && t1dot < 0;
			if (first || second) {
				++m_contourEdges;
				VertexPosNormal vertex1;
				VertexPosNormal vertex2;
				VertexPosNormal vertex3;
				VertexPosNormal vertex4;
				XMStoreFloat3(&vertex1.Pos, v1);
				XMStoreFloat3(&vertex2.Pos, v2);
				XMStoreFloat3(&vertex3.Pos, v1 + XMVector3Normalize(v1 - lightPos) * GO_TO_INFINITY);
				XMStoreFloat3(&vertex4.Pos, v2 + XMVector3Normalize(v2 - lightPos) * GO_TO_INFINITY);
				vertex1.Normal = XMFLOAT3(1, 0, 0);
				vertex2.Normal = XMFLOAT3(1, 0, 0);
				vertex3.Normal = XMFLOAT3(1, 0, 0);
				vertex4.Normal = XMFLOAT3(1, 0, 0);
				int idx = vertices.size();
				vertices.push_back(vertex1);
				vertices.push_back(vertex2);
				vertices.push_back(vertex3);
				vertices.push_back(vertex4);
				if (first) {
					indices.push_back(idx);
					indices.push_back(idx + 1);
					indices.push_back(idx + 2);
					indices.push_back(idx + 2);
					indices.push_back(idx + 1);
					indices.push_back(idx + 3);
					VertexTriangle tri1, tri2;
					tri1.v1 = XMLoadFloat3(&vertex1.Pos);
					tri1.v2 = XMLoadFloat3(&vertex2.Pos);
					tri1.v3 = XMLoadFloat3(&vertex3.Pos);
					tri2.v1 = XMLoadFloat3(&vertex3.Pos);
					tri2.v2 = XMLoadFloat3(&vertex2.Pos);
					tri2.v3 = XMLoadFloat3(&vertex4.Pos);
					part_triangles.push_back(tri1);
					part_triangles.push_back(tri2);
				}
				if (second) {
					indices.push_back(idx + 1);
					indices.push_back(idx);
					indices.push_back(idx + 2);
					indices.push_back(idx + 1);
					indices.push_back(idx + 2);
					indices.push_back(idx + 3);
					VertexTriangle tri1, tri2;
					tri1.v1 = XMLoadFloat3(&vertex2.Pos);
					tri1.v2 = XMLoadFloat3(&vertex1.Pos);
					tri1.v3 = XMLoadFloat3(&vertex3.Pos);
					tri2.v1 = XMLoadFloat3(&vertex2.Pos);
					tri2.v2 = XMLoadFloat3(&vertex3.Pos);
					tri2.v3 = XMLoadFloat3(&vertex4.Pos);
					part_triangles.push_back(tri1);
					part_triangles.push_back(tri2);
				}
			}
		}
		triangles.push_back(part_triangles);
	}
	m_vbShadowVolume = m_device.CreateVertexBuffer(vertices);
	m_ibShadowVolume = m_device.CreateIndexBuffer(indices);

	auto pl = XMPlaneFromPoints(XMLoadFloat3(&XMFLOAT3(0, 2, 0)), XMLoadFloat3(&XMFLOAT3(0, 2, 1)), XMLoadFloat3(&XMFLOAT3(1, 2, 0)));
	dotf = XMVectorGetX(XMPlaneDotCoord(pl, XMLoadFloat3(&XMFLOAT3(0, 3, 0))));

	cameraInShadow = false;
	int plus = 0;
	int zero = 0;
	int minus = 0;
	XMVECTOR pos = XMLoadFloat3(&cameraFPS.getPosition());
	/*
	XMVECTOR pos_inf = pos + XMLoadFloat3(&XMFLOAT3(GO_TO_INFINITY, 0, 0));
	XMStoreFloat3(&pos1, pos);
	XMStoreFloat3(&pos2, pos_inf);*/
	for (int part = 0; part < triangles.size(); ++part) {
		bool cameraInPartShadow = true;
		for (int i = 0; i < triangles[part].size(); ++i) {
			VertexTriangle tri = triangles[part][i];
			XMStoreFloat4(&v1, tri.v1);
			XMStoreFloat4(&v2, tri.v2);
			XMStoreFloat4(&v3, tri.v3);
			auto plane = XMPlaneFromPoints(tri.v1, tri.v2, tri.v3);
			//auto plane = XMPlaneFromPointNormal(tri.v1, tri.v2, tri.v3);
			dotf = XMVectorGetX(XMPlaneDotCoord(plane, pos));
			if (dotf > 0) {
				cameraInPartShadow = false;
				//break;
			}
			/*
			auto inside1 = XMVectorGetX(XMVector3Dot(tri.v2 - tri.v1, intersect - tri.v1));
			auto inside2 = XMVectorGetX(XMVector3Dot(tri.v3 - tri.v2, intersect - tri.v2));
			auto inside3 = XMVectorGetX(XMVector3Dot(tri.v1 - tri.v3, intersect - tri.v3));
			XMStoreFloat3(&inter, intersect);
			XMStoreFloat3(&inter1, tri.v2 - tri.v1);
			XMStoreFloat3(&inter2, intersect - tri.v1);
			XMStoreFloat3(&inter3, tri.v3 - tri.v2);
			XMStoreFloat3(&inter4, intersect - tri.v2);
			XMStoreFloat3(&inter5, tri.v1 - tri.v3);
			XMStoreFloat3(&inter6, intersect - tri.v3);
			XMStoreFloat3(&dot1, XMVector3Dot(tri.v2 - tri.v1, intersect - tri.v1));
			XMStoreFloat3(&dot2, XMVector3Dot(tri.v3 - tri.v2, intersect - tri.v2));
			XMStoreFloat3(&dot3, XMVector3Dot(tri.v1 - tri.v3, intersect - tri.v3));*/
			/*if (inside1 >= 0 && inside2 >= 0 && inside3 >= 0) {
				++plus;
			}
			else {
				++minus;
			}*/
		}
		if (cameraInPartShadow) {
			cameraInShadow = true;
			break;
		}
	}
}

void Scene::DrawShadowGeometry()
{
	//m_surfaceColorCB->Update(m_context, XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));

	m_worldCB->Update(m_context, XMMatrixIdentity());
	auto b = m_vbShadowVolume.get();
	m_context->IASetVertexBuffers(0, 1, &b, &VB_STRIDE, &VB_OFFSET);
	m_context->IASetIndexBuffer(m_ibShadowVolume.get(), DXGI_FORMAT_R16_UINT, 0);
	m_context->DrawIndexed(m_contourEdges * 6, 0, 0);
}

void Scene::DrawScene(bool mirrored)
{
	//m_surfaceColorCB->Update(m_context, BACKGROUND_COLOR);

	m_worldCB->Update(m_context, cylinder.getWorldMatrix());
	m_surfaceColorCB->Update(m_context, cylinder.getColor());
	cylinder.Render(m_context);

	m_worldCB->Update(m_context, lightSource.getWorldMatrix());
	m_surfaceColorCB->Update(m_context, lightSource.getColor());
	lightSource.Render(m_context);

	DrawRoom();
	DrawMesh();
}

void Scene::DrawMesh() const
{
	for (int i = 0; i < 6; ++i) {
		m_worldCB->Update(m_context, m_meshMtx[i]);

		auto b = m_vbMesh[i].get();
		m_context->IASetVertexBuffers(0, 1, &b, &VB_STRIDE, &VB_OFFSET);
		m_context->IASetIndexBuffer(m_ibMesh[i].get(), DXGI_FORMAT_R16_UINT, 0);
		m_context->DrawIndexed(m_meshTriangles[i].size() * 3, 0, 0);
	}
}

void Scene::DrawRoom()
{
	m_worldCB->Update(m_context, floor.getWorldMatrix());
	m_surfaceColorCB->Update(m_context, floor.getColor());
	floor.Render(m_context);

	m_worldCB->Update(m_context, ceiling.getWorldMatrix());
	m_surfaceColorCB->Update(m_context, ceiling.getColor());
	ceiling.Render(m_context);

	for (auto i = 0; i < 4; ++i) {
		m_worldCB->Update(m_context, walls[i].getWorldMatrix());
		m_surfaceColorCB->Update(m_context, walls[i].getColor());
		walls[i].Render(m_context);
	}
}

void Scene::DrawMirroredScene()
{
	m_phongEffect->Begin(m_context);

	m_context->OMSetDepthStencilState(m_dssWrite.get(), 1); 
	m_worldCB->Update(m_context, plate.getWorldMatrix());
	plate.Render(m_context);
	m_context->OMSetDepthStencilState(m_dssTest.get(), 1); // NEW

	//Setup render state and view matrix for rendering the mirrored world
	DirectX::XMMATRIX ViewMirror = m_mirrorMtx * cameraFPS.getViewMatrix();
	UpdateCamera(ViewMirror);
	m_context->RSSetState(m_rsCounterClockwise.get());

	DrawScene(true);

	m_phongEffect->End();

	// Particles will not render with counter-clockwise
	m_context->RSSetState(nullptr);

	// Render Particles
	m_context->OMSetBlendState(m_bsAlpha.get(), nullptr, BS_MASK);
	m_context->OMSetDepthStencilState(m_dssTestNoWrite.get(), 1);
	m_particles->Render(m_context);
	m_context->OMSetDepthStencilState(nullptr, 0);
	m_context->OMSetBlendState(nullptr, nullptr, BS_MASK);


	UpdateCamera(cameraFPS.getViewMatrix());
}

void Scene::DrawPlate(bool lit)
{
	if (lit) {
		m_context->OMSetBlendState(m_bsAlpha.get(), nullptr, BS_MASK);
		m_surfaceColorCB->Update(m_context, plate.getColor());
		m_worldCB->Update(m_context, plate.getWorldMatrix());
		plate.Render(m_context);
		m_context->OMSetBlendState(nullptr, nullptr, BS_MASK);
	}
	else {
		m_context->OMSetBlendState(m_bsAlpha.get(), nullptr, BS_MASK);
		m_surfaceColorCB->Update(m_context, XMFLOAT4(0.0, 0.0, 0.0, 0.5));
		m_worldCB->Update(m_context, plate.getWorldMatrix());
		plate.Render(m_context);
		m_context->OMSetBlendState(nullptr, nullptr, BS_MASK);
	}
}

void Scene::Render()
{
	if (m_context == nullptr)
		return;

	float clearColor[4] = { 0.7f, 0.7f, 0.7f, 1.0f };
	m_context->ClearRenderTargetView(m_backBuffer.get(), clearColor);
	m_context->ClearDepthStencilView(m_depthStencilView.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	m_surfaceColorCB->Update(m_context, XMFLOAT4(clearColor));

	ResetRenderTarget();
	m_projCB->Update(m_context, m_projMtx);
	UpdateCamera();

	SetLight();
	DrawMirroredScene();

	m_phongEffect->Begin(m_context);

	// Shadows step #1: draw unlit
	OffLight();
	DrawPlate(true);
	DrawScene(false);
	// Shadows step #2: create stencil buffer
	//m_context->OMSetBlendState(m_bsNoColorWrite.get(), nullptr, BS_MASK);
	m_context->ClearDepthStencilView(m_depthStencilView.get(), D3D11_CLEAR_STENCIL, 1.0f, cameraInShadow ? 1 : 0);
	m_context->OMSetRenderTargets(0, nullptr, m_depthStencilView.get());
	m_context->OMSetDepthStencilState(m_dsShadowWriteFront.get(), 0);
	DrawShadowGeometry();
	m_context->RSSetState(m_rsCounterClockwise.get());
	m_context->OMSetDepthStencilState(m_dsShadowWriteBack.get(), 0);
	DrawShadowGeometry();
	m_context->RSSetState(nullptr);
	// Shadows step #3: draw lit
	//m_context->OMSetBlendState(nullptr, nullptr, BS_MASK);
	ID3D11RenderTargetView *backbuffer = m_backBuffer.get();
	m_context->OMSetRenderTargets(1, &backbuffer, m_depthStencilView.get());
	SetLight();
	m_context->OMSetDepthStencilState(m_dsShadowTestComplement.get(), 0);
	DrawPlate(false);
	m_context->OMSetDepthStencilState(m_dsShadowTest.get(), 0);
	DrawScene(false);
	m_context->OMSetDepthStencilState(nullptr, 0);

	m_phongEffect->End();

	// Draw Partciles
	m_context->OMSetBlendState(m_bsAlpha.get(), nullptr, BS_MASK);
	m_context->OMSetDepthStencilState(m_dssNoWrite.get(), 0);
	m_particles->Render(m_context);
	m_context->OMSetDepthStencilState(nullptr, 0);
	m_context->OMSetBlendState(nullptr, nullptr, BS_MASK);
	
	m_swapChain->Present(0, 0);
	

}
