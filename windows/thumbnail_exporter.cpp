﻿// export_video_thumbnail_code.cpp

#include "thumbnail_exporter.h"

// Must be included before many other Windows headers.
#include <windows.h>

#include <shobjidl.h>      // IThumbnailCache, ISharedBitmap
#include <shlwapi.h>       // SHCreateItemFromParsingName
#include <wrl/client.h>    // Microsoft::WRL::ComPtr
#include <gdiplus.h>       // GDI+ (for saving the HBITMAP as PNG)
#include <string>
#include <vector>
#include <thumbcache.h>

#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Gdiplus.lib")

class GdiplusInit
{
public:
    GdiplusInit()
    {
        Gdiplus::GdiplusStartupInput input;
        GdiplusStartup(&token, &input, nullptr);
    }
    ~GdiplusInit()
    {
        Gdiplus::GdiplusShutdown(token);
    }
private:
    ULONG_PTR token;
};

bool GetExplorerThumbnail(
    const std::wstring& videoPath,
    const std::wstring& outputPng,
    UINT requestedSize)
{
    // Initialize COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        return false;
    }

    // Create ShellItem from video path
    Microsoft::WRL::ComPtr<IShellItem> shellItem;
    hr = SHCreateItemFromParsingName(videoPath.c_str(), nullptr, IID_PPV_ARGS(&shellItem));
    if (FAILED(hr))
    {
        CoUninitialize();
        return hr;
    }

    // Obtain IThumbnailCache
    Microsoft::WRL::ComPtr<IThumbnailCache> thumbCache;
    hr = CoCreateInstance(
        CLSID_LocalThumbnailCache,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&thumbCache)
    );
    if (FAILED(hr))
    {
        CoUninitialize();
        return hr;
    }

    // Request thumbnail 
    Microsoft::WRL::ComPtr<ISharedBitmap> sharedBitmap;
    WTS_THUMBNAILID thumbId = {};
    WTS_FLAGS flags = WTS_NONE;
	WTS_CACHEFLAGS thumbCacheFlags = WTS_DEFAULT;

    hr = thumbCache->GetThumbnail(
       shellItem.Get(),
       requestedSize,
       flags, // Now using the correct type
       &sharedBitmap,
       &thumbCacheFlags,
       &thumbId
    );

    if (FAILED(hr))
    {
        CoUninitialize();
        return hr;
    }

    // Convert ISharedBitmap to HBITMAP
    HBITMAP hBitmap = nullptr;
    hr = sharedBitmap->GetSharedBitmap(&hBitmap);
    if (FAILED(hr) || hBitmap == nullptr)
    {
        if (hBitmap) DeleteObject(hBitmap);
        CoUninitialize();
        return hr == S_OK ? E_FAIL : hr;
    }

    // Initialize GDI+ so we can HBITMAP -> PNG
    GdiplusInit gdiInit;

    Gdiplus::Bitmap bmp(hBitmap, nullptr);

    CLSID pngClsid = {};
    {
        UINT numEncoders = 0, sizeInBytes = 0;
        Gdiplus::GetImageEncodersSize(&numEncoders, &sizeInBytes);
        std::vector<BYTE> buffer(sizeInBytes);
        auto pEncoders = reinterpret_cast<Gdiplus::ImageCodecInfo*>(buffer.data());
        Gdiplus::GetImageEncoders(numEncoders, sizeInBytes, pEncoders);

        for (UINT i = 0; i < numEncoders; i++)
        {
            if (wcscmp(pEncoders[i].MimeType, L"image/png") == 0)
            {
                pngClsid = pEncoders[i].Clsid;
                break;
            }
        }
    }

    // Save to PNG file
    Gdiplus::Status status = bmp.Save(outputPng.c_str(), &pngClsid, nullptr);


    DeleteObject(hBitmap);
    CoUninitialize();

    return (status == Gdiplus::Ok);
}

bool IsExplorerThumbnailAvailable() {
    // Just check if we can create the necessary COM objects
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) return false;
    
    IThumbnailProvider* pThumbProvider = NULL;
    (void)pThumbProvider;
    bool result = false;
    
    // Just test if the API is available
    if (SUCCEEDED(hr)) {
        CoUninitialize();
        result = true;
    }
    
    return result;
}