#pragma once

#include <stdint.h>
#include <Windows.h>
//#include <d3d11.h>
#include <d3d11_1.h>
#include <iostream>
#include <mfapi.h>

//#define USE_NV12
#define USE_AMD 0

#define SAFE_RELEASE(x) if(x) { (x)->Release(); x = nullptr; }
#define IF_FAILED_RETURN(X) if(FAILED(hr = (X))){ return hr; }
#define IF_FAILED_THROW(X) if(FAILED(hr = (X))){ throw hr; }

template<class T> inline void SAFE_DELETE_ARRAY(T * &p)
{
    if (p)
    {
        delete[] p;
        p = NULL;
    }
}

BYTE GetR(const int, int const);
BYTE GetG(const int, const int, const int);
BYTE GetB(const int, const int);

namespace Render {
  extern ID3D11Device*           device;
  extern ID3D11DeviceContext*    ctx;
  extern ID3D11RenderTargetView* render_target_view;
  extern uint32_t                render_width;
  extern uint32_t                render_height;

  bool create(HWND hWnd);
  bool InitD3D11Video(HWND hWnd);

  HRESULT InitDXVA2(HWND hwnd);

  void destroy();
  void swapChain();

  struct Texture {
    ID3D11Texture2D*           texture = nullptr;
    ID3D11Texture2D*           d3d_texture = nullptr;
    ID3D11Texture2D*           nv12_texture = nullptr;
    ID3D11ShaderResourceView*  luminanceView;
    ID3D11ShaderResourceView*  chrominanceView;
    ID3D11ShaderResourceView* shader_resource_view = nullptr;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    uint32_t xres = 0;
    uint32_t yres = 0;
    uint32_t real_yres = 0;
    uint32_t frame_count = 0;
    bool create(uint32_t xres, uint32_t yres, DXGI_FORMAT new_format, bool is_dynamic);
    bool CreateTexture(uint32_t width, uint32_t height, DXGI_FORMAT new_format);
    bool CreateNV12Texture(uint32_t width, uint32_t height);
    bool CreateVideoProcessor();
    bool InitVideoProcessor();
    bool RenderTexture();
    bool RenderTexture2();
    void activate(int slot) const;
    void destroy();
    bool updateFromIYUV(const uint8_t* new_data, size_t data_size);
    bool updateYV12(const uint8_t* data, size_t data_size);
    bool updateBGRA(const uint8_t* data, size_t data_size);
    bool updateNV12(const uint8_t* data, size_t data_size);
    HRESULT ProcessNV12ToBmpFile(LPCSTR, BYTE*, const INT, const UINT, const UINT);
    HRESULT CreateBmpFile(LPCSTR, BYTE*, const UINT, const UINT, const UINT);
  };
}
