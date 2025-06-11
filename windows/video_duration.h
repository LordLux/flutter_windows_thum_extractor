#ifndef VIDEO_DURATION_H
#define VIDEO_DURATION_H

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

HRESULT GetVideoDuration(IMFMediaSource *pSource, MFTIME *pDuration);

double GetVideoFileDuration(const std::wstring& filePath);

#endif // VIDEO_DURATION_H