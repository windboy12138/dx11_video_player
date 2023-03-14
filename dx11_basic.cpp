#define _CRT_SECURE_NO_WARNINGS
#include <d3dcompiler.h>
#include "dx11_basic.h"
#include "libyuv\planar_functions.h"
#include "libyuv\convert_from.h"

#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")
#pragma comment(lib, "libyuv_internal.lib")

namespace Render {

  static IDXGISwapChain* swap_chain = nullptr;
  ID3D11Device* device = nullptr;
  ID3D11DeviceContext* ctx = nullptr;
  ID3D11VideoDevice* video_device = nullptr;
  ID3D11VideoContext* video_ctx = nullptr;
  ID3D11VideoProcessorEnumerator* m_pD3D11VideoProcessorEnumerator = NULL;
  ID3D11VideoProcessor* m_pD3D11VideoProcessor = NULL;
  ID3D11Texture2D* dxgi_back_buffer = NULL;
  ID3D11RenderTargetView* render_target_view = nullptr;
  ID3D11SamplerState*     sampler_clamp_linear = nullptr;

  uint32_t                render_width = 0;
  uint32_t                render_height = 0;

  bool create(HWND hWnd) {
    HRESULT hr;

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

    hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, featureLevels, numFeatureLevels,
      D3D11_SDK_VERSION, &sd, &swap_chain, &device, &featureLevel, &ctx);
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
      
      hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, featureLevels, numFeatureLevels,
                                         D3D11_SDK_VERSION, &sd, &swap_chain, &device, &featureLevel, &ctx);
      if (FAILED(hr))
          return false;

      hr = device->QueryInterface(__uuidof(ID3D11VideoDevice), reinterpret_cast<void**>(&video_device));
      if (FAILED(hr))
          return false;

      hr = ctx->QueryInterface(__uuidof(ID3D11VideoContext), (void**)&video_ctx);
      if (FAILED(hr))
          return false;

      return true;
  }

  bool Texture::CreateVideoProcessor()
  {
      D3D11_VIDEO_PROCESSOR_CONTENT_DESC content_desc;
      ZeroMemory(&content_desc, sizeof(content_desc));
      content_desc.InputFrameFormat = D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE;
      content_desc.InputFrameRate.Numerator = 30;
      content_desc.InputFrameRate.Denominator = 1;
      content_desc.InputWidth = xres;
      content_desc.InputHeight = real_yres;
      content_desc.OutputWidth = render_width;
      content_desc.OutputHeight = render_height;
      content_desc.OutputFrameRate.Numerator = 30;
      content_desc.OutputFrameRate.Denominator = 1;
      content_desc.Usage = D3D11_VIDEO_USAGE_OPTIMAL_SPEED;

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

      HRESULT hr = S_OK;
      hr = swap_chain->GetBuffer(0, IID_PPV_ARGS(&dxgi_back_buffer));
      if (FAILED(hr))
          return false;
      D3D11_TEXTURE2D_DESC back_buffer_desc;
      dxgi_back_buffer->GetDesc(&back_buffer_desc);
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

      //rect.right = render_width - 20;
      //rect.bottom = render_height - 20;
      //video_ctx->VideoProcessorSetStreamDestRect(m_pD3D11VideoProcessor, 0, true, &rect);

      video_ctx->VideoProcessorSetStreamFrameFormat(m_pD3D11VideoProcessor, 0, D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE);

      hr = video_ctx->VideoProcessorBlt(m_pD3D11VideoProcessor, output_view, 0, 1, &stream_data);
      if (FAILED(hr))
          return false;

      hr = swap_chain->Present(1, 0);
      if (FAILED(hr))
          return false;

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
  // Helper for compiling shaders with D3DCompile
  //
  // With VS 11, we could load up prebuilt .cso files instead...
  //--------------------------------------------------------------------------------------
  bool compileShaderFromFile(const char* szFileName, const char* szEntryPoint, const char* szShaderModel, ID3DBlob** ppBlobOut)
  {
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#ifdef _DEBUG
    dwShaderFlags |= D3DCOMPILE_DEBUG;
    dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ID3DBlob* pErrorBlob = nullptr;

    wchar_t wFilename[MAX_PATH];
    mbstowcs(wFilename, szFileName, MAX_PATH);

    hr = D3DCompileFromFile(wFilename, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, szEntryPoint, szShaderModel,
      dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
    if (FAILED(hr)) {
      if (pErrorBlob) {
        OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
        pErrorBlob->Release();
      }
      return false;
    }
    if (pErrorBlob) pErrorBlob->Release();

    return true;
  }

  //--------------------------------------------------------------------------------------
  bool Pipeline::create(const char* filename, D3D11_INPUT_ELEMENT_DESC* input_elements, uint32_t ninput_elements) {
    HRESULT hr;

    // Compile the vertex shader
    ID3DBlob* pVSBlob = nullptr;
    if (!compileShaderFromFile(filename, "VS", "vs_4_0", &pVSBlob))
      return false;

    // Create the vertex shader
    hr = device->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &vs);
    if (FAILED(hr)) {
      pVSBlob->Release();
      return false;
    }

    // Create the input layout
    hr = device->CreateInputLayout(input_elements, ninput_elements, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &input_layout);
    pVSBlob->Release();
    if (FAILED(hr))
      return false;

    // Compile the pixel shader
    ID3DBlob* pPSBlob = nullptr;
    if (!compileShaderFromFile(filename, "PS", "ps_4_0", &pPSBlob))
      return false;

    // Create the pixel shader
    hr = device->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &ps);
    pPSBlob->Release();
    if (FAILED(hr))
      return false;

    return true;
  }

  void Pipeline::destroy() {
    SAFE_RELEASE(vs);
    SAFE_RELEASE(ps);
    SAFE_RELEASE(input_layout);
  }

  void Pipeline::activate() const {
    ctx->IASetInputLayout(input_layout);
    ctx->VSSetShader(vs, nullptr, 0);
    ctx->PSSetShader(ps, nullptr, 0);
  }

  //--------------------------------------------------------------------------------------
  bool Mesh::create(const void* vertices, uint32_t new_nvertices, uint32_t new_bytes_per_vertex, eTopology new_topology) {
    topology = new_topology;
    nvertices = new_nvertices;
    bytes_per_vertex = new_bytes_per_vertex;

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = nvertices * bytes_per_vertex;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA InitData = {};
    InitData.pSysMem = vertices;
    HRESULT hr = device->CreateBuffer(&bd, &InitData, &vb);
    if (FAILED(hr))
      return false;

    return true;
  }

  void Mesh::activate() const {
    // Set vertex buffer
    UINT stride = bytes_per_vertex;
    UINT offset = 0;
    ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    ctx->IASetPrimitiveTopology((D3D_PRIMITIVE_TOPOLOGY)topology);
  }

  void Mesh::render() const {
    ctx->Draw(nvertices, 0);
  }

  void Mesh::activateAndRender() const {
    activate();
    render();
  }

  void Mesh::destroy() {
    SAFE_RELEASE(vb);
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
      hr = device->CreateTexture2D(&texDesc, nullptr, &nv12_texture);
      if (FAILED(hr))
          return false;

      D3D11_SHADER_RESOURCE_VIEW_DESC const luminancePlaneDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC(
          nv12_texture,
          D3D11_SRV_DIMENSION_TEXTURE2D,
          DXGI_FORMAT_R8_UNORM
      );
      hr = device->CreateShaderResourceView(
          nv12_texture,
          &luminancePlaneDesc,
          &luminanceView
      );
      if (FAILED(hr))
          return false;

      D3D11_SHADER_RESOURCE_VIEW_DESC const chrominancePlaneDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC(
          nv12_texture,
          D3D11_SRV_DIMENSION_TEXTURE2D,
          DXGI_FORMAT_R8G8_UNORM
      );

      hr = device->CreateShaderResourceView(
          nv12_texture,
          &chrominancePlaneDesc,
          &chrominanceView
      );
      if (FAILED(hr))
          return false;

      D3D11_TEXTURE2D_DESC desc = { 0 };
      nv12_texture->GetDesc(&desc);
      desc.BindFlags = D3D11_BIND_RENDER_TARGET;
      desc.CPUAccessFlags = 0;
      desc.Usage = D3D11_USAGE_DEFAULT;
      hr = device->CreateTexture2D(&desc, 0, &d3d_texture);
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


      // COPY YUV DATA
      //ID3D11ShaderResourceView* pSRV = nullptr;
      //D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
      //ZeroMemory(&srv_desc, sizeof(srv_desc));
      //srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
      //srv_desc.Format = DXGI_FORMAT_420_OPAQUE;
      //srv_desc.Texture2D.MipLevels = 1;
      //srv_desc.Texture2D.MostDetailedMip = 0;
      //

      //hr = device->CreateShaderResourceView(d3d_texture, &srv_desc, &pSRV);
      //if (FAILED(hr))
      //   return false;

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

      // NV12 Copy
      int stride_y = xres;
      uint8_t* src_y = (uint8_t*)data;
      uint8_t* src_uv = src_y + stride_y * yres;

      //FILE* fp = fopen("data/nv12_src.txt", "w+");
      //uint8_t* src = src_y;
      //for (int i = 0; i < yres; i++)
      //{
      //    for (int j = 0; j < xres; j++)
      //    {
      //        fprintf(fp, "%4d", *(src++));
      //    }
      //    fprintf(fp, "\n");
      //}

      //for (int i = 0; i < yres / 2; i++)
      //{
      //    for (int j = 0; j < xres; j++)
      //    {
      //        fprintf(fp, "%4d", *(src++));
      //    }
      //    fprintf(fp, "\n");
      //}

      //fclose(fp);
      
      // if 1080p
      if (yres == 1088)
      {
          real_yres = 1080;
      }

      // COPY
      D3D11_MAPPED_SUBRESOURCE ms;
      hr = ctx->Map(nv12_texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);
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

      //fp = fopen("data/nv12_dst.txt", "w+");
      //src = dst;
      //for (int i = 0; i < real_yres; i++)
      //{
      //    for (int j = 0; j < xres; j++)
      //    {
      //        fprintf(fp, "%4d", *(src++));
      //    }
      //    fprintf(fp, "\n");
      //}

      //for (int i = 0; i < real_yres / 2; i++)
      //{
      //    for (int j = 0; j < xres; j++)
      //    {
      //        fprintf(fp, "%4d", *(src++));
      //    }
      //    fprintf(fp, "\n");
      //}

      //fclose(fp);

      ctx->Unmap(nv12_texture, 0);

      ctx->CopyResource(d3d_texture, nv12_texture);

      return true;
  }
};

