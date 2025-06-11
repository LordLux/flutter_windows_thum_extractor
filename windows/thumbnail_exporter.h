// thumbnail_exporter.h
//
// Declares a single function:
//   bool GetExplorerThumbnail(
//     const std::wstring& videoPath,
//     const std::wstring& outputPng,
//     UINT requestedSize
//   );
//
// Returns true on success, false on failure.

#ifndef THUMBNAIL_EXPORTER_H_
#define THUMBNAIL_EXPORTER_H_

#include <string>
#include <wtypes.h>

bool IsExplorerThumbnailAvailable();

/// Attempts to retrieve Windows Explorer’s cached thumbnail for `videoPath`
/// at maximum dimension `requestedSize` (e.g. 256). On success, writes the
/// PNG to `outputPng` and returns true. Otherwise returns false.
bool GetExplorerThumbnail(
    const std::wstring& videoPath,
    const std::wstring& outputPng,
    UINT requestedSize);

#endif  // THUMBNAIL_EXPORTER_H_
