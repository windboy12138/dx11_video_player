# dx11_video_player
Forked from yabadabu/dx11_video_texture; You can use this sample code to render mp4 files base D3D11 VideoProcessor; Caution! Using nv12 format to render 1080p video has issues, need to fix.

# Usage
## Choose input file
In `app.cpp` file to set the input video, now it's **CPU decode method**
```cpp
VideoTexture::createAPI();
if (!video.create("data/1080p.mp4"))
  return false;
```

## Set output format
In `video_texture.cpp` open function, you can set output type to nv12 or other
```cpp
//CHECK_HR(pVideoReaderAttributes->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_IYUV),
//  "Failed to set media sub type on source reader output media type.");

CHECK_HR(pVideoReaderAttributes->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12),
         "Failed to set media sub type on source reader output media type.");
//CHECK_HR(pReaderOutputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_IYUV), "Failed to set media sub type on source reader output media type.");
CHECK_HR(pReaderOutputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12), "Failed to set media sub type on source reader output media type.");
```

## Create correct texture
Texture format depends on your video output type, in `video_texture.cpp` update function, you can choose one texture create method and update method;
```cpp
//if (!target_texture->create(width, texture_height, DXGI_FORMAT_R8_UNORM, true))
//if (!target_texture->CreateTexture(width, height, DXGI_FORMAT_420_OPAQUE))
if (!target_texture->CreateNV12Texture(width, height))
{
    return;
}

//target_texture->updateFromIYUV(byteBuffer, buffCurrLen);
//target_texture->updateYV12(byteBuffer, buffCurrLen);
//target_texture->updateBGRA(byteBuffer, buffCurrLen);
target_texture->updateNV12(byteBuffer, buffCurrLen);
```
**I recommend you to choose NV12 format, but for now it has issue with 1080p video render, if you have any idea about it, please make an issue**
