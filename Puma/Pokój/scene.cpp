#include "scene.h"

#include "gk2_window.h"
#include "gk2_utils.h"
#include <array>

using namespace std;
using namespace gk2;
using namespace DirectX;

const unsigned int Scene::BS_MASK = 0xffffffff;

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
	m_lightPosCB = make_shared<ConstantBuffer<XMFLOAT4>>(m_device);
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
	UpdateCamera();
}

void Scene::CreateScene()
{
	MeshLoader loader(m_device);
	
	CreateRoom();

	cylinder = loader.GetCylinder(100, 100);
	cylinder.setWorldMatrix(
		XMMatrixScaling(1.0f, 2.0f, 1.0f)
		* XMMatrixRotationX(XM_PIDIV2)
		* XMMatrixTranslation(-4.0f, 0.5f, -4.0f));
	cylinder.setColor(XMFLOAT4(1.0, 0.0, 0.0, 1.0));

	lightSource = loader.GetSphere(100, 100);
	lightSource.setWorldMatrix(XMMatrixTranslation(4.0f, 3.0f, 4.0f));
	lightSource.setColor(XMFLOAT4(1.0, 1.0, 0.0, 1.0));

	plate = loader.GetQuad();
	/*
	XMMATRIX plateWorldMatrix = 
		XMMatrixScaling(1.5f, 2.2f, 1.0f)
		* XMMatrixRotationX(XM_PIDIV4)
		* XMMatrixTranslation(0.0f, 0.7f, 2.0f);
		*/
	// TODO, when palte is rotated, the plate color is not updated corectly
	XMMATRIX plateWorldMatrix =
		XMMatrixScaling(1.5f, 2.2f, 1.0f)
		* XMMatrixTranslation(0.0f, 1.2f, 2.0f);

	plate.setWorldMatrix(plateWorldMatrix);
	plate.setColor(XMFLOAT4(0.5, 0.3, 0.3, 0.7));
	XMVECTOR det;
	m_mirrorMtx = 
		XMMatrixInverse(&det, plateWorldMatrix)
		* XMMatrixScaling(1, 1, -1) * plateWorldMatrix;
}

void Scene::CreateRoom() {
	MeshLoader loader(m_device);
	XMFLOAT4 wallColor(0.5, 0.5, 0.5, 1.0);

	float a = 10.0f;

	floor = loader.GetQuad();
	floor.setWorldMatrix(
		XMMatrixScaling(10.0f, 10.0f, 10.0f)
		* XMMatrixRotationX(XM_PIDIV2)
		* XMMatrixTranslation(0.0f, 0.0f, 0.0f));
	floor.setColor(wallColor);
	
	ceiling = loader.GetQuad();
	ceiling.setWorldMatrix(
		XMMatrixScaling(10.0f, 10.0f, 10.0f)
		* XMMatrixRotationX(1.5*XM_PI)
		* XMMatrixTranslation(0.0f, 10.0f, 0.0f));
	ceiling.setColor(wallColor);

	auto angle = 0;
	walls[0] = loader.GetQuad();
	walls[0].setWorldMatrix(
		XMMatrixScaling(10.0f, 10.0f, 10.0f)
		* XMMatrixRotationY(angle)
		* XMMatrixTranslation(0.0f, a/2, a/2));
	walls[0].setColor(wallColor);


	walls[1] = loader.GetQuad();
	walls[1].setWorldMatrix(
		XMMatrixScaling(10.0f, 10.0f, 10.0f)
		* XMMatrixRotationY(XM_PI)
		* XMMatrixTranslation(0.0f, a/2, -a/2));
	walls[1].setColor(wallColor);

	walls[2] = loader.GetQuad();
	walls[2].setWorldMatrix(
		XMMatrixScaling(10.0f, 10.0f, 10.0f)
		* XMMatrixRotationY(XM_PIDIV2)
		* XMMatrixTranslation(a/2, a / 2, 0));
	walls[2].setColor(wallColor);

	walls[3] = loader.GetQuad();
	walls[3].setWorldMatrix(
		XMMatrixScaling(10.0f, 10.0f, 10.0f)
		* XMMatrixRotationY(-XM_PIDIV2)
		* XMMatrixTranslation(-a / 2, a / 2, 0));
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

	//Setup depth stencil state for writing
	m_dssWrite = m_device.CreateDepthStencilState(dssDesc);

	////  DSS TEST ///
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;

	//Setup depth stencil state for testing
	m_dssTest = m_device.CreateDepthStencilState(dssDesc);

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
}

bool Scene::LoadContent()
{
	InitializeConstantBuffers();
	InitializeCamera();
	InitializeRenderStates();
	CreateScene();
	m_phongEffect = make_shared<PhongEffect>(m_device, m_layout);
	m_phongEffect->SetProjMtxBuffer(m_projCB);
	m_phongEffect->SetViewMtxBuffer(m_viewCB);
	m_phongEffect->SetWorldMtxBuffer(m_worldCB);
	m_phongEffect->SetLightPosBuffer(m_lightPosCB);
	m_phongEffect->SetSurfaceColorBuffer(m_surfaceColorCB);

	m_lightShadowEffect = make_shared<LightShadowEffect>(m_device, m_layout);
	m_lightShadowEffect->SetProjMtxBuffer(m_projCB);
	m_lightShadowEffect->SetViewMtxBuffer(m_viewCB);
	m_lightShadowEffect->SetWorldMtxBuffer(m_worldCB);
	m_lightShadowEffect->SetLightPosBuffer(m_lightPosCB);
	m_lightShadowEffect->SetSurfaceColorBuffer(m_surfaceColorCB);

	m_particles = make_shared<ParticleSystem>(m_device, XMFLOAT3(-1.3f, -0.6f, -0.14f));
	m_particles->SetViewMtxBuffer(m_viewCB);
	m_particles->SetProjMtxBuffer(m_projCB);
	return true;
}

void Scene::UnloadContent()
{

}

void Scene::UpdateCameraControl() {
	static MouseState prevState;
	MouseState currentState;
	KeyboardState keyboardState;

	float mouseBoost = 1.0f;

	if (m_keyboard->GetState(keyboardState)) {
		float boost = 1.0f;
		if (keyboardState.isKeyDown(DIK_SPACE)) {
			boost = 3.0f;
			mouseBoost = 3.0f;
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

void Scene::Update(float dt)
{
	cameraFPS.update();
	UpdateCameraControl();
	
	m_particles->Update(m_context, dt, m_camera.GetPosition());
}

void Scene::DrawScene(bool mirrored)
{
	//m_surfaceColorCB->Update(m_context, BACKGROUND_COLOR);

	if (!mirrored) {
		m_context->OMSetBlendState(m_bsAlpha.get(), nullptr, BS_MASK);
		m_surfaceColorCB->Update(m_context, plate.getColor());
	}
	m_worldCB->Update(m_context, plate.getWorldMatrix());
	plate.Render(m_context);
	if (!mirrored) {
		m_context->OMSetBlendState(nullptr, nullptr, BS_MASK);
	}

	m_worldCB->Update(m_context, cylinder.getWorldMatrix());
	m_surfaceColorCB->Update(m_context, cylinder.getColor());
	cylinder.Render(m_context);
	
	m_worldCB->Update(m_context, lightSource.getWorldMatrix());
	m_surfaceColorCB->Update(m_context, lightSource.getColor());
	lightSource.Render(m_context);

	DrawRoom();
}

void Scene::DrawRoom() {

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
	m_context->OMSetDepthStencilState(m_dssWrite.get(), 1); 
	m_worldCB->Update(m_context, plate.getWorldMatrix());
	plate.Render(m_context);
	m_context->OMSetDepthStencilState(m_dssTest.get(), 1); // NEW

	//Setup render state and view matrix for rendering the mirrored world
	DirectX::XMMATRIX ViewMirror = m_mirrorMtx * cameraFPS.getViewMatrix();
	UpdateCamera(ViewMirror);

	//Setup render state for writing to the stencil buffer
	m_context->RSSetState(m_rsCounterClockwise.get());

	DrawScene(true);
	UpdateCamera(cameraFPS.getViewMatrix());

	//Restore rendering state to it's original values
	m_context->RSSetState(nullptr);
	m_context->OMSetDepthStencilState(nullptr, 0);
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
	
	m_phongEffect->Begin(m_context);
	
	DrawMirroredScene();
	DrawScene(false);

	//TODO: Replace with light and shadow map effect. DONE
	//m_lightShadowEffect->Begin(m_context);
	//m_lightShadowEffect->End();

	m_context->OMSetBlendState(m_bsAlpha.get(), nullptr, BS_MASK);
	m_context->OMSetDepthStencilState(m_dssNoWrite.get(), 0);
	//m_particles->Render(m_context);
	m_context->OMSetDepthStencilState(nullptr, 0);
	m_context->OMSetBlendState(nullptr, nullptr, BS_MASK);
	m_swapChain->Present(0, 0);
	
	m_phongEffect->End();
}
