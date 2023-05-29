#pragma once

#include "dx11_basic.h"

#define USE_NV12

struct VideoTexture {

  static bool createAPI();
  static void destroyAPI();

  struct InternalData;
  InternalData* internal_data = nullptr;

  bool create(const char* filename);
  void destroy();
  bool update(float dt);

  void pause();
  void resume();
  bool hasFinished();
  bool render_video();
  Render::Texture* getTexture();
  float getAspectRatio() const;
};

