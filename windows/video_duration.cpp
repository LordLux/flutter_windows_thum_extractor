#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <propvarutil.h>
#include <shlwapi.h> // For PathFileExistsW
#include <string>
#include <iostream>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "Strmiids.lib")
#pragma comment(lib, "shlwapi.lib")
// A simple RAII (Resource Acquisition Is Initialization) helper for Media Foundation.
// This ensures that MFStartup() and MFShutdown() are always called correctly,
// even if an exception or early return occurs.
struct MFInitializer
{
  MFInitializer()
  {
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr))
    {
      std::wcerr << L"Failed to initialize Media Foundation: 0x" << std::hex << hr << std::endl;
    }
  }
  ~MFInitializer()
  {
    MFShutdown();
  }
};

// A helper function to safely release a COM object pointer.
template <class T>
void SafeRelease(T **ppT)
{
  if (*ppT)
  {
    (*ppT)->Release();
    *ppT = NULL;
  }
}
/**
 * @brief Gets the duration of a video file in milliseconds
 *
 * This function uses the Media Foundation API. It is designed to handle long paths
 * (longer than 260 characters) by using MFCreateFile to create a byte stream from
 * the file path, which correctly handles the "\\?\" prefix
 *
 * @param filePath The path to the video file, which can be a long path
 * @return The duration of the video in milliseconds as a double, or 0.0 if an error occurs
 */
double GetVideoFileDuration(const std::wstring &filePath)
{
  IMFByteStream *pByteStream = NULL;
  IMFSourceReader *pReader = NULL;
  PROPVARIANT var;
  PropVariantInit(&var);

  // RAII helper to ensure MFStartup and MFShutdown are handled
  MFInitializer mfInitializer;

  // Checks if the file exists
  if (!PathFileExistsW(filePath.c_str()))
  {
    std::wcerr << L"File not found: " << filePath << std::endl;
    return 0.0;
  }
  else
  {
    // std::wcout << L"File exists: " << filePath << std::endl;
  }

  // Creates a byte stream from the file path.
  HRESULT hr = MFCreateFile(
      MF_ACCESSMODE_READ,            // Access mode
      MF_OPENMODE_FAIL_IF_NOT_EXIST, // Open mode
      MF_FILEFLAGS_NONE,             // File flags
      filePath.c_str(),              // File path
      &pByteStream                   // Output byte stream
  );
  if (FAILED(hr))
  {
    std::wcerr << L"Failed to create byte stream from file: " << filePath << L". Error: 0x" << std::hex << hr << std::endl;
    SafeRelease(&pByteStream);
    return 0.0;
  }

  // Creates the source reader from the byte stream
  hr = MFCreateSourceReaderFromByteStream(pByteStream, NULL, &pReader);
  if (FAILED(hr))
  {
    std::wcerr << L"Failed to create source reader from byte stream. Error: 0x" << std::hex << hr << std::endl;
    SafeRelease(&pByteStream);
    SafeRelease(&pReader);
    return 0.0;
  }

  // Gets duration from the presentation descriptor
  hr = pReader->GetPresentationAttribute(static_cast<DWORD>(MF_SOURCE_READER_MEDIASOURCE), MF_PD_DURATION, &var);

  double durationMs = 0.0;
  if (SUCCEEDED(hr))
  {
    ULONGLONG duration = var.uhVal.QuadPart;
    durationMs = static_cast<double>(duration) / 10000.0; // Converts from 100-nanosecond units to milliseconds
  }
  else
  {
    std::wcerr << L"Failed to get duration attribute. Error: 0x" << std::hex << hr << std::endl;
  }

  // Cleanup resources
  PropVariantClear(&var);
  SafeRelease(&pReader);
  SafeRelease(&pByteStream);

  return durationMs;
}