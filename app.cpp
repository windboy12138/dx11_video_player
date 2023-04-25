#include "app.h"
#include <vector>

using namespace Render;
// --------------------------------------------------------------------------------------
bool CApp::create(HWND hWnd) 
{
  if (!Render::InitD3D11Video(hWnd))
    return false;

  VideoTexture::createAPI();
  if (!video.create("data/4k2.mp4"))
    return false;

  return true;
}

void CApp::render_video()
{
    video.render_video();
}

void CApp::update(float dt) {
  camera_time += dt;
  video.update(dt);
}

void CApp::destroy() {
  video.destroy();
  VideoTexture::destroyAPI();
  Render::destroy();
}