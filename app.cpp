#include "app.h"
#include <vector>

using namespace Render;
// --------------------------------------------------------------------------------------
bool CApp::create(HWND hWnd) {
    //if (!Render::create(hWnd))
    //    return false;
  if (!Render::InitD3D11Video(hWnd))
    return false;

  VideoTexture::createAPI();
  if (!video.create("data/demo.mp4"))
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