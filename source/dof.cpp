#include "dof.h"

#include <string>
#include <sstream>
#include <vector>
using namespace std;

#include "settings.h"
#include "renderstate_manager.h"

DOF::DOF(IDirect3DDevice9 *device, int width, int height, Type type, float baseRadius) 
	: Effect(device), width(width), height(height) {
	
	// Setup the defines for compiling the effect
    vector<D3DXMACRO> defines;

    // Setup pixel size macro
	stringstream sp;
	sp << "float2(1.0 / " << width << ", 1.0 / " << height << ")";
	string pixelSizeText = sp.str();
	D3DXMACRO pixelSizeMacro = { "PIXEL_SIZE", pixelSizeText.c_str() };
	defines.push_back(pixelSizeMacro);

	string radiusText = format("%f", baseRadius);
	D3DXMACRO radiusMacro = { "DOF_BASE_BLUR_RADIUS", radiusText.c_str() };
	defines.push_back(radiusMacro);
	
    D3DXMACRO null = { NULL, NULL };
    defines.push_back(null);

	DWORD flags = D3DXFX_NOT_CLONEABLE | D3DXSHADER_OPTIMIZATION_LEVEL3;

	// Load effect from file
	string shaderFn;
	switch(type) {
		case BOKEH: shaderFn = "DOFBokeh.fx"; break;
		case BASIC: shaderFn = "DOFPseudo.fx"; break;
	}
	shaderFn = getAssetFileName(shaderFn);
	SDLOG(0, "%s load\n", shaderFn.c_str());	
	ID3DXBuffer* errors;
	HRESULT hr = D3DXCreateEffectFromFile(device, shaderFn.c_str(), &defines.front(), NULL, flags, NULL, &effect, &errors);
	if(hr != D3D_OK) SDLOG(0, "ERRORS:\n %s\n", errors->GetBufferPointer());
	
	// Create buffers
	device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &buffer1Tex, NULL);
    buffer1Tex->GetSurfaceLevel(0, &buffer1Surf);
	device->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &buffer2Tex, NULL);
    buffer2Tex->GetSurfaceLevel(0, &buffer2Surf);

	// get handles
	depthTexHandle = effect->GetParameterByName(NULL, "depthTex");
	thisframeTexHandle = effect->GetParameterByName(NULL, "thisframeTex");
    lastpassTexHandle = effect->GetParameterByName(NULL, "lastpassTex");
}

DOF::~DOF() {
	SAFERELEASE(effect);
	SAFERELEASE(buffer1Surf);
	SAFERELEASE(buffer1Tex);
	SAFERELEASE(buffer2Surf);
	SAFERELEASE(buffer2Tex);
}

void DOF::go(IDirect3DTexture9 *frame, IDirect3DTexture9 *depth, IDirect3DSurface9 *dst) {
	device->SetVertexDeclaration(vertexDeclaration);
	
    cocPass(frame, depth, buffer1Surf);
	dofPass(buffer1Tex, buffer2Surf);
	hBlurPass(buffer2Tex, buffer1Surf);
	vBlurPass(buffer1Tex, dst);
}

void DOF::cocPass(IDirect3DTexture9 *frame, IDirect3DTexture9 *depth, IDirect3DSurface9 *dst) {
	device->SetRenderTarget(0, dst);

    // Setup variables.
    effect->SetTexture(depthTexHandle, depth);
    effect->SetTexture(thisframeTexHandle, frame);

    // Do it!
    UINT passes;
	effect->Begin(&passes, 0);
	effect->BeginPass(0);
	quad(width, height);
	effect->EndPass();
	effect->End();
}

void DOF::dofPass(IDirect3DTexture9 *prev, IDirect3DSurface9* dst) {
	device->SetRenderTarget(0, dst);

    // Setup variables.
    effect->SetTexture(lastpassTexHandle, prev);
	
    // Do it!
    UINT passes;
	effect->Begin(&passes, 0);
	effect->BeginPass(1);
	quad(width, height);
	effect->EndPass();
	effect->End();
}

void DOF::hBlurPass(IDirect3DTexture9 *prev, IDirect3DSurface9* dst) {
	device->SetRenderTarget(0, dst);
	
    // Setup variables.
    effect->SetTexture(lastpassTexHandle, prev);
	
    // Do it!
    UINT passes;
	effect->Begin(&passes, 0);
	effect->BeginPass(2);
	quad(width, height);
	effect->EndPass();
	effect->End();
}

void DOF::vBlurPass(IDirect3DTexture9 *prev, IDirect3DSurface9* dst) {
	device->SetRenderTarget(0, dst);
	
    // Setup variables.
    effect->SetTexture(lastpassTexHandle, prev);
	
    // Do it!
    UINT passes;
	effect->Begin(&passes, 0);
	effect->BeginPass(3);
	quad(width, height);
	effect->EndPass();
	effect->End();
}
