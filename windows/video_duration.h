#ifndef VIDEO_DURATION_H
#define VIDEO_DURATION_H

#include <string>

// Returns the duration of a video file in milliseconds.
// Returns 0.0 if the file is not found or an error occurs.
// The filePath can be a long path (with \\?\ prefix).
double GetVideoFileDuration(const std::wstring &filePath);

#endif // VIDEO_DURATION_H
