#include <windows.h> 

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <string>

HRESULT GetVideoDuration(IMFMediaSource *pSource, MFTIME *pDuration)
{
    *pDuration = 0;

    IMFPresentationDescriptor *pPD = NULL;

    HRESULT hr = pSource->CreatePresentationDescriptor(&pPD);
    if (SUCCEEDED(hr))
    {
        hr = pPD->GetUINT64(MF_PD_DURATION, (UINT64*)pDuration);
        pPD->Release();
    }
    return hr;
}

// Get the duration of a video file
// Get the duration of a video file
double GetVideoFileDuration(const std::wstring& filePath) {
  // Initialize Media Foundation
  HRESULT hr = MFStartup(MF_VERSION);
  if (FAILED(hr)) 
    return 0.0;
  
  // Create source reader - no conversion needed now
  IMFSourceReader* pReader = NULL;
  hr = MFCreateSourceReaderFromURL(filePath.c_str(), NULL, &pReader);
  if (FAILED(hr)) {
    MFShutdown();
    return 0.0;
  }
  
  // Get duration
  PROPVARIANT var;
  PropVariantInit(&var);
  hr = pReader->GetPresentationAttribute(
    static_cast<DWORD>(MF_SOURCE_READER_MEDIASOURCE), 
    MF_PD_DURATION, 
    &var
  );
  
  double durationMs = 0.0;
  if (SUCCEEDED(hr)) {
    // Convert 100-nanosecond units to milliseconds
    UINT64 duration = var.uhVal.QuadPart;
    durationMs = duration / 10000.0;
  }
  
  // Cleanup
  PropVariantClear(&var);
  if (pReader) pReader->Release();
  MFShutdown();
  
  return durationMs;
}