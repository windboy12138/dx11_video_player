#pragma once

#include "dx11_basic.h"
#include "video_texture.h"

class CApp {
  VideoTexture     video;
  float            camera_time = 0.0f;

public:
  bool create(HWND hWnd);
  void destroy();
  void update(float dt);
  void render_video();
};
