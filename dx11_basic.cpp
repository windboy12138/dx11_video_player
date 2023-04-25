#define _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include "dx11_basic.h"
#include "MacroTools.h"
#include "libyuv\planar_functions.h"
#include "libyuv\convert_from.h"
#include "libyuv\convert_from_argb.h"
#include <dxgi.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "libyuv_internal.lib")

BYTE GetR(const int iY, int const iU) {

    int iR = (int)(1.164f * (float)iY + 1.596f * (float)iU);
    iR = iR > 255 ? 255 : iR < 0 ? 0 : iR;

    return (BYTE)iR;
}

BYTE GetG(const int iY, const int iU, const int iV) {

    int iG = (int)(1.164f * (float)iY - 0.813f * (float)iV - 0.392f * (float)iU);
    iG = iG > 255 ? 255 : iG < 0 ? 0 : iG;

    return (BYTE)iG;
}

BYTE GetB(const int iY, const int iV) {

    int iB = (int)(1.164f * (float)iY + 2.017f * (float)iV);
    iB = iB > 255 ? 255 : iB < 0 ? 0 : iB;

    return (BYTE)iB;
}

namespace Render {

  static IDXGISwapChain1* swap_chain = nullptr;
  ID3D11Device* device = nullptr;
  ID3D11DeviceContext* ctx = nullptr;
  IDXGIFactory2* m_Factory = NULL;
  IDXGISwapChain1* m_SwapChain;
  ID3D11VideoDevice* video_device = nullptr;
  ID3D11VideoContext* video_ctx = nullptr;
  ID3D11VideoProcessorEnumerator* m_pD3D11VideoProcessorEnumerator = NULL;
  ID3D11VideoProcessor* m_pD3D11VideoProcessor = NULL;
  HWND m_hwnd = NULL;
  ID3D11Texture2D* dxgi_back_buffer = NULL;
  IMFSample* pRTSample = NULL;
  IMFMediaBuffer* pBuffer = NULL;
  ID3D11RenderTargetView* render_target_view = nullptr;
  ID3D11SamplerState*     sampler_clamp_linear = nullptr;

  uint32_t                render_width = 0;
  uint32_t                render_height = 0;

  bool create(HWND hWnd) {
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect(hWnd, &rc);
    render_width = rc.right - rc.left;
    render_height = rc.bottom - rc.top;

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);
    D3D_FEATURE_LEVEL       featureLevel = D3D_FEATURE_LEVEL_11_0;

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = render_width;
    sd.BufferDesc.Height = render_height;
    //sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    //hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevels, numFeatureLevels,
    //  D3D11_SDK_VERSION, &sd, &swap_chain, &device, &featureLevel, &ctx);
    if (FAILED(hr))
      return false;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (FAILED(hr))
      return false;
    hr = device->CreateRenderTargetView(pBackBuffer, nullptr, &render_target_view);
    pBackBuffer->Release();
    if (FAILED(hr))
      return false;

    ctx->OMSetRenderTargets(1, &render_target_view, nullptr);

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)render_width;
    vp.Height = (FLOAT)render_height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    ctx->RSSetViewports(1, &vp);

    // Create the sample state
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = device->CreateSamplerState(&sampDesc, &sampler_clamp_linear);
    if (FAILED(hr))
      return false;
    ctx->PSSetSamplers(0, 1, &sampler_clamp_linear);

    return true;
  }

  bool InitD3D11Video(HWND hWnd) {
      HRESULT hr;
      if (FAILED(InitDXVA2(hWnd)))
      {
          return false;
      }
      else
      {
          return true;
      }

      RECT rc;
      GetClientRect(hWnd, &rc);
      render_width = rc.right - rc.left;
      render_height = rc.bottom - rc.top;

      D3D_FEATURE_LEVEL featureLevels[] = {
          D3D_FEATURE_LEVEL_11_1,
          D3D_FEATURE_LEVEL_11_0,
      };
      UINT numFeatureLevels = ARRAYSIZE(featureLevels);
      D3D_FEATURE_LEVEL       featureLevel = D3D_FEATURE_LEVEL_11_0;

      DXGI_SWAP_CHAIN_DESC sd = {};
      sd.BufferCount = 2;
      sd.BufferDesc.Width = render_width;
      sd.BufferDesc.Height = render_height;
      sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
      sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;//DXGI_FORMAT_R8G8B8A8_UNORM;
      sd.BufferDesc.RefreshRate.Numerator = 30;
      sd.BufferDesc.RefreshRate.Denominator = 1;
      sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
      sd.OutputWindow = hWnd;
      sd.SampleDesc.Count = 1;
      sd.SampleDesc.Quality = 0;
      sd.Windowed = TRUE;

      hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, featureLevels, numFeatureLevels,
                             D3D11_SDK_VERSION, &device, &featureLevel, &ctx);
      
      //hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, featureLevels, numFeatureLevels,
      //                                   D3D11_SDK_VERSION, &sd, &swap_chain, &device, &featureLevel, &ctx);
      if (FAILED(hr))
          return false;

      hr = device->QueryInterface(__uuidof(ID3D11VideoDevice), reinterpret_cast<void**>(&video_device));
      if (FAILED(hr))
          return false;

      hr = ctx->QueryInterface(__uuidof(ID3D11VideoContext), (void**)&video_ctx);
      if (FAILED(hr))
          return false;

      IDXGIDevice* DxgiDevice = nullptr;
      hr = device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&DxgiDevice));
      if (FAILED(hr))
      {
          return hr;
      }

      IDXGIAdapter* DxgiAdapter = nullptr;
      hr = DxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&DxgiAdapter));
      DxgiDevice->Release();
      DxgiDevice = nullptr;
      if (FAILED(hr))
      {
          return hr;
      }

      hr = DxgiAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&m_Factory));
      DxgiAdapter->Release();
      DxgiAdapter = nullptr;
      if (FAILED(hr))
      {
          return hr;
      }

      DXGI_SWAP_CHAIN_DESC1 SwapChainDesc;
      RtlZeroMemory(&SwapChainDesc, sizeof(SwapChainDesc));

      SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
      SwapChainDesc.BufferCount = 2;
      SwapChainDesc.Width = rc.right;
      SwapChainDesc.Height = rc.bottom;
      SwapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
      SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
      SwapChainDesc.SampleDesc.Count = 1;
      SwapChainDesc.SampleDesc.Quality = 0;

      hr = m_Factory->CreateSwapChainForHwnd(device, hWnd, &SwapChainDesc, nullptr, nullptr, &swap_chain);
      if (FAILED(hr))
      {
          return false;
      }

      return true;
  }

  HRESULT InitDXVA2(HWND hWnd)
  {
      HRESULT hr = S_OK;

      IF_FAILED_RETURN(ctx != NULL ? E_UNEXPECTED : S_OK);

      //DISPLAY_DEVICE dd;
      //memset(&dd, 0, sizeof(DISPLAY_DEVICE));
      //dd.cb = sizeof(dd);
      //int i = 0;
      //char szDeviceName[32];
      //while (EnumDisplayDevices(NULL, i, &dd, 0))
      //{
      //    strcpy(szDeviceName, dd.DeviceName);
      //    if (EnumDisplayDevices(szDeviceName, 0, &dd, 0))
      //    {
      //        dd.DeviceName;
      //    }

      //    i++;
      //}

      DXGI_OUTPUT_DESC m_OutputDesc;
      IDXGIFactory1* pFactory = NULL;
      CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory);
      IDXGIAdapter* pAdapter = NULL;
      IDXGIOutput* DxgiOutput = NULL;
      DXGI_ADAPTER_DESC AdapterDesc;
      for (UINT i = 0; ; ++i)
      {
          if (pFactory->EnumAdapters(i, &pAdapter) == DXGI_ERROR_NOT_FOUND)
          {
              break;
          }
          pAdapter->GetDesc(&AdapterDesc);
          pAdapter->EnumOutputs(0, &DxgiOutput);
          DxgiOutput->GetDesc(&m_OutputDesc);
          if (i == USE_AMD)
          {
              // i == 1 use sub screen graphic card
              // i == 0 use main screen graphic card
              break;
          }
      }
      pFactory->Release();



      D3D_FEATURE_LEVEL featureLevels[] =
      {
          //D3D_FEATURE_LEVEL_9_1,
          //D3D_FEATURE_LEVEL_9_2,
          //D3D_FEATURE_LEVEL_9_3,
          //D3D_FEATURE_LEVEL_10_0,
          //D3D_FEATURE_LEVEL_10_1,
          D3D_FEATURE_LEVEL_11_0,
          D3D_FEATURE_LEVEL_11_1,
      };

      D3D_DRIVER_TYPE gDriverTypes[] =
      {
          D3D_DRIVER_TYPE_UNKNOWN,
          D3D_DRIVER_TYPE_HARDWARE,
          D3D_DRIVER_TYPE_REFERENCE,
          D3D_DRIVER_TYPE_NULL,
          D3D_DRIVER_TYPE_SOFTWARE,
          D3D_DRIVER_TYPE_WARP
      };
          
      UINT uiFeatureLevels = ARRAYSIZE(featureLevels);
      D3D_FEATURE_LEVEL featureLevel;
      UINT uiD3D11CreateFlag = D3D11_CREATE_DEVICE_SINGLETHREADED;
      UINT uiFormatSupport;

#ifdef _DEBUG
      uiD3D11CreateFlag |= D3D11_CREATE_DEVICE_DEBUG;
#endif

      try
      {
          IF_FAILED_THROW(D3D11CreateDevice(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, uiD3D11CreateFlag, featureLevels, uiFeatureLevels, D3D11_SDK_VERSION, &device, &featureLevel, &ctx));
          IF_FAILED_THROW(device->CheckFormatSupport(DXGI_FORMAT_NV12, &uiFormatSupport));
          IF_FAILED_THROW(device->QueryInterface(__uuidof(ID3D11VideoDevice), reinterpret_cast<void**>(&video_device)));
          IF_FAILED_THROW(ctx->QueryInterface(__uuidof(ID3D11VideoContext), (void**)&video_ctx));
      }
      catch (HRESULT) {}

      IDXGIDevice* DxgiDevice = nullptr;
      IDXGIAdapter* DxgiAdapter = nullptr;
      IDXGIFactory2* Factory = nullptr;
      IF_FAILED_RETURN(device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&DxgiDevice)));      
      IF_FAILED_RETURN(DxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&DxgiAdapter)));
      SAFE_RELEASE(DxgiDevice);      
      IF_FAILED_RETURN(DxgiAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&Factory)));
      SAFE_RELEASE(DxgiAdapter);

      RECT rc = { 0 };
      GetClientRect(hWnd, &rc);
      m_hwnd = hWnd;

      //DXGI_SWAP_CHAIN_DESC1 SwapChainDesc;
      //RtlZeroMemory(&SwapChainDesc, sizeof(SwapChainDesc));
      //SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
      //SwapChainDesc.BufferCount = 4;
      //SwapChainDesc.Width = rc.right;
      //SwapChainDesc.Height = rc.bottom;
      //SwapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
      //SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
      //SwapChainDesc.SampleDesc.Count = 1;
      //SwapChainDesc.SampleDesc.Quality = 0;
      //SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;

      //IF_FAILED_RETURN(Factory->CreateSwapChainForHwnd(device, hWnd, &SwapChainDesc, nullptr, nullptr, &swap_chain));
      //IF_FAILED_RETURN(swap_chain->SetFullscreenState(FALSE, NULL));
      //SAFE_RELEASE(Factory);

      // Get the DXGISwapChain1
      DXGI_SWAP_CHAIN_DESC1 scd;
      ZeroMemory(&scd, sizeof(scd));
      scd.SampleDesc.Count = 1;
      scd.SampleDesc.Quality = 0;
      scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
      scd.Scaling = DXGI_SCALING_STRETCH;
      scd.Width = rc.right;
      scd.Height = rc.bottom;
      scd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
      scd.Stereo = 0;
      scd.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
      scd.Flags = 0; //opt in to do direct flip;
      scd.BufferCount = 2;

      IF_FAILED_RETURN(Factory->CreateSwapChainForHwnd(device, hWnd, &scd, nullptr, nullptr, &swap_chain));
      IF_FAILED_RETURN(swap_chain->SetFullscreenState(FALSE, NULL));
      SAFE_RELEASE(Factory);

      UINT resetToken;
      IMFDXGIDeviceManager* m_pDXGIManager;
      hr = MFCreateDXGIDeviceManager(&resetToken, &m_pDXGIManager);
      if (FAILED(hr))
      {
          return hr;
      }

      hr = m_pDXGIManager->ResetDevice(device, resetToken);
      if (FAILED(hr))
      {
          return hr;
      }

      SAFE_RELEASE(ctx);
      device->GetImmediateContext(&ctx);
      
      return hr;
  }

  bool Texture::InitVideoProcessor()
  {
      HRESULT hr = S_OK;
      D3D11_VIDEO_PROCESSOR_CAPS VPCaps;
      UINT uiFormat;

      D3D11_VIDEO_PROCESSOR_CONTENT_DESC descVP;
      descVP.InputFrameFormat = D3D11_VIDEO_FRAME_FORMAT_INTERLACED_TOP_FIELD_FIRST;//D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE;
      descVP.InputFrameRate.Numerator = 30;
      descVP.InputFrameRate.Denominator = 1;
      descVP.InputWidth = xres;
      descVP.InputHeight = real_yres;
      descVP.OutputFrameRate.Numerator = 30;
      descVP.OutputFrameRate.Denominator = 1;
      descVP.OutputWidth = xres;
      descVP.OutputHeight = real_yres;
      descVP.Usage = D3D11_VIDEO_USAGE_PLAYBACK_NORMAL;//D3D11_VIDEO_USAGE_OPTIMAL_QUALITY;

      try
      {
          IF_FAILED_THROW(video_device->CreateVideoProcessorEnumerator(&descVP, &m_pD3D11VideoProcessorEnumerator));
          IF_FAILED_THROW(m_pD3D11VideoProcessorEnumerator->GetVideoProcessorCaps(&VPCaps));

          IF_FAILED_THROW(VPCaps.MaxInputStreams < 1 ? E_FAIL : S_OK);
          IF_FAILED_THROW(VPCaps.MaxStreamStates < 1 ? E_FAIL : S_OK);

          uiFormat = D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT_INPUT;
          IF_FAILED_THROW(m_pD3D11VideoProcessorEnumerator->CheckVideoProcessorFormat(DXGI_FORMAT_B8G8R8A8_UNORM, &uiFormat));

          uiFormat = D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT_OUTPUT;
          IF_FAILED_THROW(m_pD3D11VideoProcessorEnumerator->CheckVideoProcessorFormat(DXGI_FORMAT_NV12, &uiFormat));

          DWORD index;
          //FindBOBProcessorIndex(&index)
          {
              D3D11_VIDEO_PROCESSOR_CAPS caps = {};
              D3D11_VIDEO_PROCESSOR_RATE_CONVERSION_CAPS convCaps = {};
              m_pD3D11VideoProcessorEnumerator->GetVideoProcessorCaps(&caps);

              for (DWORD i = 0; i < caps.RateConversionCapsCount; i++)
              {
                  hr = m_pD3D11VideoProcessorEnumerator->GetVideoProcessorRateConversionCaps(i, &convCaps);
                  if (FAILED(hr))
                  {
                      return false;
                  }

                  // Check the caps to see which deinterlacer is supported
                  if ((convCaps.ProcessorCaps & D3D11_VIDEO_PROCESSOR_PROCESSOR_CAPS_DEINTERLACE_BOB) != 0)
                  {
                      index = i;
                  }
              }
          }

          IF_FAILED_THROW(video_device->CreateVideoProcessor(m_pD3D11VideoProcessorEnumerator, 0, &m_pD3D11VideoProcessor));
      }
      catch (HRESULT) {}

      if (FAILED(hr))
      {
          return false;
      }

      RECT rc = { 0, 0, xres, real_yres };
      video_ctx->VideoProcessorSetStreamFrameFormat(m_pD3D11VideoProcessor, 0, D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE);
      video_ctx->VideoProcessorSetStreamOutputRate(m_pD3D11VideoProcessor, 0, D3D11_VIDEO_PROCESSOR_OUTPUT_RATE_NORMAL, TRUE, NULL);
      video_ctx->VideoProcessorSetStreamSourceRect(m_pD3D11VideoProcessor, 0, TRUE, &rc);

      GetClientRect(m_hwnd, &rc);
      video_ctx->VideoProcessorSetStreamDestRect(m_pD3D11VideoProcessor, 0, TRUE, &rc);
      video_ctx->VideoProcessorSetOutputTargetRect(m_pD3D11VideoProcessor, TRUE, &rc);

      return true;
  }

  bool Texture::CreateVideoProcessor()
  {
      D3D11_VIDEO_PROCESSOR_CONTENT_DESC content_desc;
      ZeroMemory(&content_desc, sizeof(content_desc));
      content_desc.InputFrameFormat = D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE; // D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE
      content_desc.InputFrameRate.Numerator = 30;
      content_desc.InputFrameRate.Denominator = 1;
      content_desc.InputWidth = xres;
      content_desc.InputHeight = real_yres;
      content_desc.OutputWidth = xres;
      content_desc.OutputHeight = real_yres;
      content_desc.OutputFrameRate.Numerator = 30;
      content_desc.OutputFrameRate.Denominator = 1;
      content_desc.Usage = D3D11_VIDEO_USAGE_PLAYBACK_NORMAL; // D3D11_VIDEO_USAGE_OPTIMAL_SPEED

      HRESULT hr;
      hr = video_device->CreateVideoProcessorEnumerator(&content_desc, &m_pD3D11VideoProcessorEnumerator);
      if (FAILED(hr))
          return false;

      hr = video_device->CreateVideoProcessor(m_pD3D11VideoProcessorEnumerator, 0, &m_pD3D11VideoProcessor);
      if (FAILED(hr))
          return false;

      return true;
  }

  bool Texture::RenderTexture()
  {
      if (!m_pD3D11VideoProcessor)
      {
          CreateVideoProcessor();
      }

      device->QueryInterface(__uuidof(ID3D11VideoDevice), reinterpret_cast<void**>(&video_device));

      HRESULT hr = S_OK;
      //hr = swap_chain->GetBuffer(0, IID_PPV_ARGS(&dxgi_back_buffer));
      hr = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&dxgi_back_buffer);
      if (FAILED(hr))
          return false;

      D3D11_VIDEO_PROCESSOR_OUTPUT_VIEW_DESC output_view_desc;
      ZeroMemory(&output_view_desc, sizeof(output_view_desc));
      output_view_desc.ViewDimension = D3D11_VPOV_DIMENSION_TEXTURE2D;
      output_view_desc.Texture2D.MipSlice = 0;
      ID3D11VideoProcessorOutputView* output_view;
      hr = video_device->CreateVideoProcessorOutputView(dxgi_back_buffer, m_pD3D11VideoProcessorEnumerator,
                                                        &output_view_desc, &output_view);
      if (FAILED(hr))
          return false;

      D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC input_view_desc;
      ZeroMemory(&input_view_desc, sizeof(input_view_desc));
      input_view_desc.FourCC = 0;
      input_view_desc.ViewDimension = D3D11_VPIV_DIMENSION_TEXTURE2D;
      input_view_desc.Texture2D.MipSlice = 0;
      input_view_desc.Texture2D.ArraySlice = 0;
      ID3D11VideoProcessorInputView* input_view;
      hr = video_device->CreateVideoProcessorInputView(d3d_texture, m_pD3D11VideoProcessorEnumerator,
                                                       &input_view_desc, &input_view);
      if (FAILED(hr))
          return false;

      // set frame foramt
      D3D11_VIDEO_FRAME_FORMAT FrameFormat = D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE;
      video_ctx->VideoProcessorSetStreamFrameFormat(m_pD3D11VideoProcessor, 0, FrameFormat);

      D3D11_VIDEO_PROCESSOR_COLOR_SPACE colorSpace = {};
      colorSpace.YCbCr_xvYCC = 1;
      video_ctx->VideoProcessorSetStreamColorSpace(m_pD3D11VideoProcessor, 0, &colorSpace);

      video_ctx->VideoProcessorSetOutputColorSpace(m_pD3D11VideoProcessor, &colorSpace);
      ///

      D3D11_VIDEO_PROCESSOR_STREAM stream_data;
      ZeroMemory(&stream_data, sizeof(stream_data));
      stream_data.Enable = TRUE;
      stream_data.OutputIndex = 0;
      stream_data.InputFrameOrField = 0;
      stream_data.PastFrames = 0;
      stream_data.FutureFrames = 0;
      stream_data.ppPastSurfaces = nullptr;
      stream_data.ppFutureSurfaces = nullptr;
      stream_data.pInputSurface = input_view;
      stream_data.ppPastSurfacesRight = nullptr;
      stream_data.ppFutureSurfacesRight = nullptr;
      stream_data.pInputSurfaceRight = nullptr;

      RECT rect = { 0 };
      rect.right = xres;
      rect.bottom = real_yres;
      video_ctx->VideoProcessorSetStreamSourceRect(m_pD3D11VideoProcessor, 0, true, &rect);

      rect.right = render_width - 20;
      rect.bottom = render_height - 20;
      video_ctx->VideoProcessorSetStreamDestRect(m_pD3D11VideoProcessor, 0, true, &rect);

      video_ctx->VideoProcessorSetStreamFrameFormat(m_pD3D11VideoProcessor, 0, D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE);


      hr = video_ctx->VideoProcessorBlt(m_pD3D11VideoProcessor, output_view, 0, 1, &stream_data);
      if (FAILED(hr))
          return false;

      hr = swap_chain->Present(1, 0);
      if (FAILED(hr))
          return false;

      return true;
  }

  bool Texture::RenderTexture2()
  {
      if (!m_pD3D11VideoProcessor)
      {
          InitVideoProcessor();
      }

      HRESULT hr = S_OK;
      ID3D11VideoProcessorInputView* pD3D11VideoProcessorInputViewIn = NULL;
      ID3D11VideoProcessorOutputView* pD3D11VideoProcessorOutputView = NULL;
      ID3D11VideoDevice* pD3D11VideoDevice = NULL;
      ID3D11Texture2D* pInTexture2D = NULL;

      ID3D11Texture2D* pDXGIBackBuffer = NULL;
      ID3D11RenderTargetView* pRTView = NULL;
      IMFSample* pRTSample = NULL;
      IMFMediaBuffer* pBuffer = NULL;
      D3D11_VIDEO_PROCESSOR_CAPS vpCaps = { 0 };
      
      try
      {
          IF_FAILED_THROW(swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pDXGIBackBuffer));
          IF_FAILED_THROW(MFCreateSample(&pRTSample));
          IF_FAILED_THROW(MFCreateDXGISurfaceBuffer(__uuidof(ID3D11Texture2D), pDXGIBackBuffer, 0, FALSE, &pBuffer));
          IF_FAILED_THROW(pRTSample->AddBuffer(pBuffer));

          D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC pInDesc;
          ZeroMemory(&pInDesc, sizeof(pInDesc));
          pInDesc.FourCC = 0;
          pInDesc.ViewDimension = D3D11_VPIV_DIMENSION_TEXTURE2D;
          pInDesc.Texture2D.MipSlice = 0;
          pInDesc.Texture2D.ArraySlice = 0;
          pInTexture2D = d3d_texture;

          IF_FAILED_THROW(video_device->CreateVideoProcessorInputView(pInTexture2D, m_pD3D11VideoProcessorEnumerator, &pInDesc, &pD3D11VideoProcessorInputViewIn));

          D3D11_VIDEO_PROCESSOR_OUTPUT_VIEW_DESC pOutDesc;
          ZeroMemory(&pOutDesc, sizeof(pOutDesc));
          pOutDesc.ViewDimension = D3D11_VPOV_DIMENSION_TEXTURE2D;
          pOutDesc.Texture2D.MipSlice = 0;

          IF_FAILED_THROW(video_device->CreateVideoProcessorOutputView(pDXGIBackBuffer, m_pD3D11VideoProcessorEnumerator, &pOutDesc, &pD3D11VideoProcessorOutputView));

          D3D11_VIDEO_PROCESSOR_STREAM StreamData;
          ZeroMemory(&StreamData, sizeof(StreamData));
          StreamData.Enable = TRUE;
          StreamData.OutputIndex = 0;
          StreamData.InputFrameOrField = 0;
          StreamData.PastFrames = 0;
          StreamData.FutureFrames = 0;
          StreamData.ppPastSurfaces = NULL;
          StreamData.ppFutureSurfaces = NULL;
          StreamData.pInputSurface = pD3D11VideoProcessorInputViewIn;
          StreamData.ppPastSurfacesRight = NULL;
          StreamData.ppFutureSurfacesRight = NULL;

          IF_FAILED_THROW(video_ctx->VideoProcessorBlt(m_pD3D11VideoProcessor, pD3D11VideoProcessorOutputView, 0, 1, &StreamData));
          IF_FAILED_THROW(swap_chain->Present(1, 0));
      }
      catch (HRESULT) {}

      //SAFE_RELEASE(pInTexture2D);
      SAFE_RELEASE(pBuffer);
      SAFE_RELEASE(pRTSample);
      SAFE_RELEASE(pDXGIBackBuffer);
      SAFE_RELEASE(pD3D11VideoProcessorOutputView);
      SAFE_RELEASE(pD3D11VideoProcessorInputViewIn);
      //SAFE_RELEASE(pD3D11VideoDevice);

      return true;
  }

  void destroy() {
    SAFE_RELEASE(sampler_clamp_linear);
    SAFE_RELEASE(render_target_view);
    SAFE_RELEASE(swap_chain);
    SAFE_RELEASE(ctx);
    SAFE_RELEASE(device);

    // video processor
    SAFE_RELEASE(video_device);
    SAFE_RELEASE(video_ctx);
  }

  void swapChain() {
    swap_chain->Present(0, 0);
  }

  //--------------------------------------------------------------------------------------
  bool Texture::create(uint32_t new_xres, uint32_t new_yres, DXGI_FORMAT new_format, bool is_dynamic) {
    xres = new_xres;
    yres = new_yres;
    format = new_format;
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = xres;
    desc.Height = yres;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = format;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;  
    if (is_dynamic) {
      desc.Usage = D3D11_USAGE_DYNAMIC;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    }
    ID3D11Texture2D* tex2d = nullptr;
    HRESULT hr = device->CreateTexture2D(&desc, nullptr, (ID3D11Texture2D**)&texture);
    if (FAILED(hr))
      return false;

    // Create a resource view so we can use the data in a shader
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
    ZeroMemory(&srv_desc, sizeof(srv_desc));
    srv_desc.Format = new_format;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = desc.MipLevels;
    hr = device->CreateShaderResourceView(texture, &srv_desc, &shader_resource_view);
    if (FAILED(hr))
      return false;
    
    return true;
  }

  //--------------------------------------------------------------------------------------
  bool Texture::CreateTexture(uint32_t width, uint32_t height, DXGI_FORMAT new_format)
  {
      xres = width;
      yres = height;
      real_yres = yres;
      format = new_format;

      // Texture for update bgra data
      D3D11_TEXTURE2D_DESC desc = {};
      desc.Width = xres;
      desc.Height = yres;
      desc.MipLevels = 1;
      desc.ArraySize = 1;
      desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;//DXGI_FORMAT_B8G8R8A8_UNORM;//DXGI_FORMAT_NV12;
      desc.SampleDesc.Count = 1;
      desc.SampleDesc.Quality = 0;
      desc.Usage = D3D11_USAGE_STAGING;
      desc.BindFlags = 0;
      desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;;// D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
      desc.MiscFlags = 0;

      HRESULT hr = device->CreateTexture2D(&desc, nullptr, (ID3D11Texture2D**)&texture);
      if (FAILED(hr))
          return false;

      // Texture for render at swapchain
      D3D11_TEXTURE2D_DESC desc1 = { 0 };
      texture->GetDesc(&desc1);
      desc1.Usage = D3D11_USAGE_DEFAULT;
      desc1.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
      desc1.BindFlags = D3D11_BIND_RENDER_TARGET;
      hr = device->CreateTexture2D(&desc1, nullptr, &d3d_texture);
      if (FAILED(hr))
          return false;
      
      return true;
  }

  bool Texture::CreateNV12Texture(uint32_t width, uint32_t height)
  {
      HRESULT hr;
      xres = width;
      yres = height;
      real_yres = yres;
      if (yres == 1088)
      {
          real_yres = 1080;
      }
      D3D11_TEXTURE2D_DESC desc2D;
      desc2D.Width = xres;
      desc2D.Height = real_yres;
      desc2D.MipLevels = 1;
      desc2D.ArraySize = 1;
      desc2D.Format = DXGI_FORMAT_NV12;
      desc2D.SampleDesc.Count = 1;
      desc2D.SampleDesc.Quality = 0;
      desc2D.Usage = D3D11_USAGE_DEFAULT;
      desc2D.BindFlags = D3D11_BIND_RENDER_TARGET;
      desc2D.CPUAccessFlags = 0;
      desc2D.MiscFlags = 0;

      hr = device->CreateTexture2D(&desc2D, 0, &d3d_texture);
      if (FAILED(hr))
          return false;

      desc2D.BindFlags = 0;
      desc2D.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
      desc2D.Usage = D3D11_USAGE_STAGING;

      D3D11_TEXTURE2D_DESC const texDesc = CD3D11_TEXTURE2D_DESC(
          DXGI_FORMAT_NV12,           // HoloLens PV camera format, common for video sources
          xres,					// Width of the video frames
          real_yres,					// Height of the video frames
          1,                          // Number of textures in the array
          1,                          // Number of miplevels in each texture
          D3D11_BIND_SHADER_RESOURCE, // We read from this texture in the shader
          D3D11_USAGE_DYNAMIC,        // Because we'll be copying from CPU memory
          D3D11_CPU_ACCESS_WRITE      // We only need to write into the texture
      );

      hr = device->CreateTexture2D(&desc2D, nullptr, &nv12_texture);
      if (FAILED(hr))
          return false;

      return true;
  }
  void Texture::activate(int slot) const {
    ctx->PSSetShaderResources(slot, 1, &shader_resource_view);
  }

  void Texture::destroy() {
    SAFE_RELEASE(texture);
    SAFE_RELEASE(shader_resource_view);
  }

  bool Texture::updateFromIYUV(const uint8_t* data, size_t data_size) {
    assert(data);
    D3D11_MAPPED_SUBRESOURCE ms;
    HRESULT hr = ctx->Map(texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
    if (FAILED(hr))
      return false;

    uint32_t bytes_per_texel = 1;
    assert(format == DXGI_FORMAT_R8_UNORM);
    assert(data_size == xres * yres * 3 / 4);

    const uint8_t* src = data;
    uint8_t*       dst = (uint8_t*)ms.pData;

    // Copy the Y lines
    uint32_t nlines = yres / 2;
    uint32_t bytes_per_row = xres * bytes_per_texel;
    for (uint32_t y = 0; y < nlines; ++y) {
      memcpy(dst, src, bytes_per_row);
      src += bytes_per_row;
      dst += ms.RowPitch;
    }

    // Now the U and V lines, need to add Width/2 pixels of padding between each line
    uint32_t uv_bytes_per_row = bytes_per_row / 2;
    for (uint32_t y = 0; y < nlines; ++y) {
      memcpy(dst, src, uv_bytes_per_row);
      src += uv_bytes_per_row;
      dst += ms.RowPitch;
    }

    ctx->Unmap(texture, 0);
    return true;
  }

  bool Texture::updateYV12(const uint8_t* data, size_t data_size) 
  {
      assert(data);
      assert(data_size == xres * yres * 6 / 4);
      HRESULT hr = S_OK;

      // I420ToNV12
      int stride_y = xres;
      int stride_uv = (xres + 1) / 2;
      int height_uv = (yres + 1) / 2;
      const uint8_t* src_y = data;
      const uint8_t* src_v = data + stride_y * yres;
      const uint8_t* src_u = src_v + (stride_uv * height_uv);

      uint8_t* dst_y = new uint8_t[xres * yres * 6 / 4];
      uint8_t* dst_uv = dst_y + xres * yres;
      libyuv::I420ToNV12(src_y, stride_y,
                         src_u, stride_uv,
                         src_v, stride_uv,
                         dst_y, xres,
                         dst_uv, stride_uv,
                         xres, yres);

      D3D11_TEXTURE2D_DESC desc = { 0 };
      desc.Width = xres;
      desc.Height = yres;
      desc.MipLevels = 1;
      desc.ArraySize = 1;
      desc.Format = DXGI_FORMAT_NV12;//DXGI_FORMAT_420_OPAQUE;// DXGI_FORMAT_NV12
      desc.SampleDesc.Count = 1;
      desc.SampleDesc.Quality = 0;
      desc.Usage = D3D11_USAGE_DEFAULT;
      desc.BindFlags = 0;
      desc.CPUAccessFlags = 0;
      desc.MiscFlags = 0;

      D3D11_SUBRESOURCE_DATA SubResource2D;
      ZeroMemory(&SubResource2D, sizeof(SubResource2D));
      SubResource2D.pSysMem = (void*)dst_y;
      SubResource2D.SysMemPitch = xres;
      //ctx->UpdateSubresource(texture, 0, nullptr, data, xres, 0);
      hr = device->CreateTexture2D(&desc, &SubResource2D, &texture);
      if (FAILED(hr))
          return false;
      ///

      ctx->CopyResource(d3d_texture, texture);

      delete[] dst_y;
      return true;
  }

  bool Texture::updateBGRA(const uint8_t* data, size_t data_size)
  {
      assert(data);
      assert(data_size == xres * yres * 6 / 4);
      HRESULT hr = S_OK;

      // I420ToARGB
      int stride_y = xres;
      int stride_uv = (xres + 1) / 2;
      int height_uv = (yres + 1) / 2;
      const uint8_t* src_y = data;
      const uint8_t* src_v = data + stride_y * yres;
      const uint8_t* src_u = src_v + (stride_uv * height_uv);

      uint8_t* dst_argb = new uint8_t[xres * yres * 4];
      // Caution! Here video frame is not standard I420 format!
      libyuv::I420ToARGB(src_y, stride_y,
                         src_v, stride_uv,
                         src_u, stride_uv,
                         dst_argb, xres * 4,
                         xres, yres);


      // COPY BGRA DATA
      D3D11_MAPPED_SUBRESOURCE ms;
      hr = ctx->Map(texture, 0, D3D11_MAP_READ_WRITE, 0, &ms);
      if (FAILED(hr))
          return false;

      uint8_t* dst = (uint8_t*)ms.pData;
      uint8_t* src = dst_argb;
      for (int i = 0; i < yres; i++)
      {
          memcpy(dst, src, xres * 4);
          dst += xres * 4;
          src += xres * 4;
      }

      ctx->Unmap(texture, 0);

      ctx->CopyResource(d3d_texture, texture);

      delete[] dst_argb;
      return true;
  }

  bool Texture::updateNV12(const uint8_t* data, size_t data_size)
  {
      assert(data);
      assert(data_size == xres * yres * 6 / 4);
      HRESULT hr = S_OK;
      int stride_y = xres;
      uint8_t* src_y = nullptr;
      uint8_t* src_uv = nullptr;

      uint8_t* src = (uint8_t*)data;
      src_y = (uint8_t*)src;
      src_uv = src_y + stride_y * yres;

      // COPY
      D3D11_MAPPED_SUBRESOURCE ms;
      hr = ctx->Map(nv12_texture, 0, D3D11_MAP_WRITE, 0, &ms);
      if (FAILED(hr))
          return false;

      uint8_t* dst = (uint8_t*)ms.pData;
      ms.RowPitch = xres;
      uint8_t* dst_y = dst;
      uint8_t* dst_uv = dst_y + stride_y * real_yres;
      
      for (int i = 0; i < real_yres; i++)
      {
          //memcpy(dst + stride_y * i, dst_y + stride_y * i, stride_y);
          memcpy(dst_y, src_y, stride_y);
          dst_y += stride_y;
          src_y += stride_y;
      }

      for (int i = 0; i < real_yres / 2; i++)
      {
          //memcpy(dst + (real_yres + i) * stride_y, dst_uv + stride_y * i, stride_y);
          memcpy(dst_uv, src_uv, stride_y);
          dst_uv += stride_y;
          src_uv += stride_y;
      }

      ctx->Unmap(nv12_texture, 0);

      ctx->CopyResource(d3d_texture, nv12_texture);

      //char out[200];
      //sprintf(out, "data/nv12_%02d.bmp", frame_count);
      //hr = ProcessNV12ToBmpFile(out, dst, xres, xres, real_yres);
      //if (FAILED(hr))
      //    return false;

      return true;
  }

  HRESULT Texture::ProcessNV12ToBmpFile(LPCSTR wszBmpFile, BYTE* pDataIn, const INT iStride, const UINT uiWidth, const UINT uiHeight)
  {
      HRESULT hr = S_OK;

      UINT uiSampleSize = uiWidth * uiHeight * 3;
      BYTE* pDataRgb = new (std::nothrow)BYTE[uiSampleSize];
      IF_FAILED_RETURN(pDataRgb == NULL ? E_FAIL : S_OK);

      UINT uiStrideRGB = uiWidth * 3;
      UINT uiDelta = iStride - uiWidth;

      BYTE* pDataY = NULL;
      BYTE* pDataY2 = NULL;
      BYTE* pDataUV = NULL;

      BYTE* pRgb1 = NULL;
      BYTE* pRgb2 = NULL;

      BYTE bY1, bY2, bY3, bY4;
      BYTE bU, bV;

      int iY1, iY2, iY3, iY4;
      int iU, iV;

      pRgb1 = pDataRgb;
      pRgb2 = pDataRgb + uiStrideRGB;

      pDataY = pDataIn;
      pDataY2 = pDataIn + iStride;
      pDataUV = pDataIn + (iStride * uiHeight);

      try {

          for (UINT i = 0; i < uiHeight; i += 2)
          {
              for (UINT j = 0; j < uiWidth; j += 2)
              {
                  bY1 = *pDataY++;
                  bY2 = *pDataY++;
                  bY3 = *pDataY2++;
                  bY4 = *pDataY2++;
                  bU = *pDataUV++;
                  bV = *pDataUV++;

                  iY1 = (int)bY1 - 16;
                  iY2 = (int)bY2 - 16;
                  iY3 = (int)bY3 - 16;
                  iY4 = (int)bY4 - 16;

                  iU = (int)bU - 128;
                  iV = (int)bV - 128;

                  *pRgb1++ = GetR(iY1, iU);
                  *pRgb1++ = GetG(iY1, iU, iV);
                  *pRgb1++ = GetB(iY1, iV);

                  *pRgb1++ = GetR(iY2, iU);
                  *pRgb1++ = GetG(iY2, iU, iV);
                  *pRgb1++ = GetB(iY2, iV);

                  *pRgb2++ = GetR(iY3, iU);
                  *pRgb2++ = GetG(iY3, iU, iV);
                  *pRgb2++ = GetB(iY3, iV);

                  *pRgb2++ = GetR(iY4, iU);
                  *pRgb2++ = GetG(iY4, iU, iV);
                  *pRgb2++ = GetB(iY4, iV);
              }

              pDataUV += uiDelta;
              pDataY += (iStride + uiDelta);
              pDataY2 += (iStride + uiDelta);
              pRgb1 += uiStrideRGB;
              pRgb2 += uiStrideRGB;
          }

          IF_FAILED_THROW(CreateBmpFile(wszBmpFile, pDataRgb, uiSampleSize, uiWidth, uiHeight));
      }
      catch (HRESULT) {}

      SAFE_DELETE_ARRAY(pDataRgb);

      return hr;
  }

  HRESULT Texture::CreateBmpFile(LPCSTR wszBmpFile, BYTE* pData, const UINT uiFrameSize, const UINT uiWidth, const UINT uiHeight)
  {
      HRESULT hr = S_OK;

      HANDLE hFile = INVALID_HANDLE_VALUE;
      DWORD dwWritten;
      UINT uiStride;

      BYTE header24[54] = { 0x42, 0x4d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00,
                            0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

      DWORD dwSizeFile = uiWidth * uiHeight * 3;
      dwSizeFile += 54;
      header24[2] = dwSizeFile & 0x000000ff;
      header24[3] = static_cast<BYTE>((dwSizeFile & 0x0000ff00) >> 8);
      header24[4] = static_cast<BYTE>((dwSizeFile & 0x00ff0000) >> 16);
      header24[5] = (dwSizeFile & 0xff000000) >> 24;
      dwSizeFile -= 54;
      header24[18] = uiWidth & 0x000000ff;
      header24[19] = (uiWidth & 0x0000ff00) >> 8;
      header24[20] = static_cast<BYTE>((uiWidth & 0x00ff0000) >> 16);
      header24[21] = (uiWidth & 0xff000000) >> 24;

      header24[22] = uiHeight & 0x000000ff;
      header24[23] = (uiHeight & 0x0000ff00) >> 8;
      header24[24] = static_cast<BYTE>((uiHeight & 0x00ff0000) >> 16);
      header24[25] = (uiHeight & 0xff000000) >> 24;

      header24[34] = dwSizeFile & 0x000000ff;
      header24[35] = (dwSizeFile & 0x0000ff00) >> 8;
      header24[36] = static_cast<BYTE>((dwSizeFile & 0x00ff0000) >> 16);
      header24[37] = static_cast<BYTE>((dwSizeFile & 0xff000000) >> 24);

      UINT i = 0;
      try
      {
          hFile = CreateFile(wszBmpFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

          IF_FAILED_THROW(hFile == INVALID_HANDLE_VALUE ? E_FAIL : S_OK);

          IF_FAILED_THROW(WriteFile(hFile, (LPCVOID)header24, 54, &dwWritten, 0) == FALSE);
          IF_FAILED_THROW(dwWritten == 0 ? E_FAIL : S_OK);

          uiStride = uiWidth * 3;
          BYTE* Tmpbufsrc = pData + (uiFrameSize - uiStride);

          for (i = 0; i < uiHeight; i++) {

              IF_FAILED_THROW(WriteFile(hFile, (LPCVOID)Tmpbufsrc, uiStride, &dwWritten, 0) == FALSE);
              IF_FAILED_THROW(dwWritten == 0 ? E_FAIL : S_OK);

              Tmpbufsrc -= uiStride;
          }
      }
      catch (HRESULT) {}

      if (hFile != INVALID_HANDLE_VALUE)
          CloseHandle(hFile);

      return hr;
  }
};

