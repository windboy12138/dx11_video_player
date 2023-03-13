#include "app.h"
#include <vector>

using namespace Render;
// --------------------------------------------------------------------------------------
bool CApp::create(HWND hWnd) {
  if (!Render::InitD3D11Video(hWnd))
    return false;

  // -------------------------------------------
  D3D11_INPUT_ELEMENT_DESC layout[] = {
      { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "COLOR",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  };
  if (!pipe_video.create("video.hlsl", layout, ARRAYSIZE(layout)))
    return false;
  if (!pipe_solid.create("solid.hlsl", layout, ARRAYSIZE(layout)))
    return false;

  // -------------------------------------------
  struct Vertex {
    float   x, y, z;
    float   u, v;
    Vector4 color;
  };

  // -------------------------------------------
  // A simple quad with 6 vertices
  {
    Vector4 white(1, 1, 1, 1);
    std::vector< Vertex> vtxs = {
      { 0,0,0, 0,1, white},
      { 0,1,0, 0,0, white},
      { 1,0,0, 1,1, white},
      { 0,1,0, 0,0, white},
      { 1,1,0, 1,0, white},
      { 1,0,0, 1,1, white},
    };
    // render region [-1, 1] is valid
    vtxs = {
      { -1,-1,0, 0,1, white},
      { -1,1,0, 0,0, white},
      { 1,-1,0, 1,1, white},
      { -1,1,0, 0,0, white},
      { 1,1,0, 1,0, white},
      { 1,-1,0, 1,1, white},
    };
    if (!quad.create(vtxs.data(), (uint32_t)vtxs.size(), sizeof(Vertex), Render::eTopology::TRIANGLE_LIST))
      return false;
  }

  // -------------------------------------------
  // A grid
  {
    //int samples = 5;
    //std::vector< Vertex> vtxs;
    //Vector4 color1(0.2f, 0.2f, 0.2f, 1.0f);
    //Vector4 color2(0.3f, 0.3f, 0.3f, 1.0f);
    //for (int i = -samples; i <= samples; ++i) {
    //  Vector4 color = (i % 5) ? color1 : color2;
    //  Vertex v1 = { (float)i, 0.0f, (float)-samples, 0, 0, color };
    //  Vertex v2 = { (float)i, 0.0f, (float)samples, 0, 0, color };
    //  vtxs.push_back(v1);
    //  vtxs.push_back(v2);
    //  Vertex v3 = { (float)-samples, 0.0f, (float)i, 0, 0, color };
    //  Vertex v4 = { (float)samples, 0.0f, (float)i, 0, 0, color };
    //  vtxs.push_back(v3);
    //  vtxs.push_back(v4);
    //}
    //if (!grid.create(vtxs.data(), (uint32_t)vtxs.size(), sizeof(Vertex), Render::eTopology::LINE_LIST))
    //  return false;
  }

  // Ctes for the camera and world matrix
  if (!cte.create(0))
    return false;

  VideoTexture::createAPI();
  if (!video.create("data/4k2.mp4"))
    return false;

  return true;
}

void CApp::render() {
  float ClearColor[4] = { 0, 0, 0.1f, 1.f }; // red,green,blue,alpha
  Render::ctx->ClearRenderTargetView(Render::render_target_view, ClearColor);
  cte.activate();

  {
    // The grid
    //pipe_solid.activate();
    //cte.World = Matrix::Identity;
    //cte.uploadToGPU();
    //grid.activateAndRender();
  }

  {
    // The video
    pipe_video.activate();
    video.getTexture()->activate(0);
    // cte.World = (Matrix::CreateTranslation(-0.5f, 0.0f, 0.0f) * Matrix::CreateScale(video.getAspectRatio(), 1, 1)).Transpose();
    cte.uploadToGPU();
    quad.activateAndRender();
  }

  Render::swapChain();
}

void CApp::render_video()
{
    video.render_video();
}

void CApp::update(float dt) {

  camera_time += dt;
  // Setup camera
  /*float cam_x = 1.0f * cosf(camera_time);
  cam_x = 0.f;
  float radius = 0.8f;
  Vector3 Eye(cam_x, 0.5f, radius);
  Vector3 At(0.0f, 0.5f, 0.0f);
  Vector3 Up(0.0f, 1.0f, 0.0f);
  cte.View = Matrix::CreateLookAt(Eye, At, Up).Transpose();
  cte.Projection = Matrix::CreatePerspectiveFieldOfView(65.0f * XM_PI / 180.0f, Render::render_width / (FLOAT)Render::render_height, 0.01f, 100.0f).Transpose();*/

  video.update(dt);
}

void CApp::destroy() {
  video.destroy();
  VideoTexture::destroyAPI();
  cte.destroy();
  grid.destroy();
  quad.destroy();
  pipe_solid.destroy();
  pipe_video.destroy();
  Render::destroy();
}