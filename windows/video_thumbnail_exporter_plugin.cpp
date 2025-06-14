#include "video_thumbnail_exporter_plugin.h"
#include "thumbnail_exporter.h"
#include "mkv_metadata_extractor_version5.h"
#include "video_duration.h"

// This must be included before many other Windows headers.
#include <windows.h>

// For getPlatformVersion; remove unless needed for your plugin implementation.
#include <VersionHelpers.h>

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <memory>
#include <sstream>

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <propvarutil.h>
#include <shlwapi.h>
#include <string>

namespace video_thumbnail_exporter {

// static
void VideoThumbnailExporterPlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows *registrar) {
  auto channel =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          registrar->messenger(), "video_thumbnail_exporter",
          &flutter::StandardMethodCodec::GetInstance());

  auto plugin = std::make_unique<VideoThumbnailExporterPlugin>();

  channel->SetMethodCallHandler(
      [plugin_pointer = plugin.get()](const auto &call, auto result) {
        plugin_pointer->HandleMethodCall(call, std::move(result));
      });

  registrar->AddPlugin(std::move(plugin));
}

VideoThumbnailExporterPlugin::VideoThumbnailExporterPlugin() {}

VideoThumbnailExporterPlugin::~VideoThumbnailExporterPlugin() {}

// Helper function to convert wide string to UTF-8
std::string WideToUtf8(const std::wstring& wide) {
  if (wide.empty()) return std::string();
  
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
  std::string strTo(size_needed, 0);
  WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &strTo[0], size_needed, nullptr, nullptr);
  
  // Remove null terminator
  if (!strTo.empty() && strTo.back() == '\0') {
    strTo.pop_back();
  }
  
  return strTo;
}

void VideoThumbnailExporterPlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue> &method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  // Helper lambda to convert EncodableValue→std::wstring
  auto getString = [&](const flutter::EncodableValue& val) -> std::wstring {
    if (auto strPtr = std::get_if<std::string>(&val)) {
      // Convert UTF-8 std::string to std::wstring
      int len = MultiByteToWideChar(CP_UTF8, 0, strPtr->c_str(), -1, nullptr, 0);
      std::wstring wstr(len, L'\0');
      MultiByteToWideChar(CP_UTF8, 0, strPtr->c_str(), -1, &wstr[0], len);
      // trim trailing null
      if (!wstr.empty() && wstr.back() == L'\0') {
        wstr.pop_back();
      }
      return wstr;
    }
    return L"";
  };

  auto method = method_call.method_name();
  if (method == "initializeExtractor") {
    std::wstring testPath;
    const auto* args = std::get_if<flutter::EncodableMap>(method_call.arguments());
    if (args) {
      auto iter = args->find(flutter::EncodableValue("testPath"));
      if (iter != args->end()) {
        std::string path = std::get<std::string>(iter->second);
        // Convert to wstring
        testPath = std::wstring(path.begin(), path.end());
      }
    }
    
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
      result->Error("init_error", "Failed to initialize Media Foundation");
      return;
    }
    
    try {
      MkvMetadataExtractor testExtractor;
      
      // Allocate buffer for temp path
      wchar_t tempPathBuffer[MAX_PATH];
      DWORD tempPathLen = GetTempPathW(MAX_PATH, tempPathBuffer);
      if (tempPathLen == 0 || tempPathLen > MAX_PATH) {
        result->Error("init_error", "Failed to get temp path.");
        return;
      }
      testPath = std::wstring(tempPathBuffer, tempPathLen);

      // Just test if the thumbnail system is accessible
      // No actual extraction happens
      IsExplorerThumbnailAvailable();
      
      result->Success(flutter::EncodableValue(true));
    }
    catch (const std::exception& e) {
      std::string errorMsg = "Initialization error: ";
      errorMsg += e.what();
      result->Error("init_error", errorMsg);
    }
  }
  // Handle video thumbnail extraction
  else if (method == "getThumbnail") {
    // Expect arguments as a Map: {
    //   'videoPath': String,
    //   'outputPath': String,
    //   'size': int
    // }
    const auto* args = std::get_if<flutter::EncodableMap>(method_call.arguments());
    if (!args) {
      result->Error(
        "bad_args",
        "Expected a map with keys 'videoPath', 'outputPath', 'size'.");
      return;
    }
    
     // Extract each field:
    std::wstring videoPathW, outputPathW;
    UINT sizePx = 0;
    
    // Loop over the map and pick out our keys:
    for (const auto& kv : *args) {
      const auto& key = kv.first;
      const auto& value = kv.second;
      if (auto keyStr = std::get_if<std::string>(&key)) {
        if (*keyStr == "videoPath") {
          videoPathW = getString(value);
        } else if (*keyStr == "outputPath") {
          outputPathW = getString(value);
        } else if (*keyStr == "size") {
          if (auto intPtr = std::get_if<int>(&value)) {
            sizePx = static_cast<UINT>(*intPtr);
          }
        }
      }
    }
    
    // Validate
    if (videoPathW.empty() || outputPathW.empty() || sizePx == 0) {
      result->Error(
        "invalid_args",
        "One or more of 'videoPath', 'outputPath', or 'size' is missing/invalid.");
      return;
    }

    // Call our native helper:
    bool ok = GetExplorerThumbnail(videoPathW, outputPathW, sizePx);
    if (ok) {
      result->Success(flutter::EncodableValue(true));
    } else {
      result->Error(
        "native_error",
        "Failed to retrieve or save the thumbnail. Check paths & permissions.");
    }
  } 
  // Handle MKV metadata extraction
  else if (method == "getMkvMetadata") {
    const auto* args = std::get_if<flutter::EncodableMap>(method_call.arguments());
    if (!args) {
      result->Error(
        "bad_args",
        "Expected a map with key 'mkvPath'.");
      return;
    }
    
    // Get the MKV file path
    std::string mkvPath;
    for (const auto& kv : *args) {
      const auto& key = kv.first;
      const auto& value = kv.second;
      if (auto keyStr = std::get_if<std::string>(&key)) {
        if (*keyStr == "mkvPath" && std::get_if<std::string>(&value)) {
          mkvPath = std::get<std::string>(value);
        }
      }
    }
    
    if (mkvPath.empty()) {
      result->Error(
        "invalid_args",
        "Missing or invalid 'mkvPath' parameter.");
      return;
    }
    
    // Create MKV metadata extractor
    MkvMetadataExtractor extractor;
    if (!extractor.open(mkvPath)) {
      result->Error(
        "file_error",
        "Failed to open the MKV file.");
      return;
    }
    
    // Build a response map with the metadata
    flutter::EncodableMap metadata;
    
    // Add general information
    metadata[flutter::EncodableValue("title")] = 
      flutter::EncodableValue(extractor.getTitle());
    metadata[flutter::EncodableValue("duration")] = 
      flutter::EncodableValue(extractor.getDuration());
    metadata[flutter::EncodableValue("muxingApp")] = 
      flutter::EncodableValue(extractor.getMuxingApp());
    metadata[flutter::EncodableValue("writingApp")] = 
      flutter::EncodableValue(extractor.getWritingApp());
    metadata[flutter::EncodableValue("bitrate")] = 
      flutter::EncodableValue(static_cast<int64_t>(extractor.getEstimatedBitrate()));
    
    // Add video streams info
    flutter::EncodableList videoStreams;
    for (const auto& stream : extractor.getVideoStreams()) {
      flutter::EncodableMap videoStream;
      videoStream[flutter::EncodableValue("trackNumber")] = 
        flutter::EncodableValue(static_cast<int>(stream.trackNumber));
      videoStream[flutter::EncodableValue("codecId")] = 
        flutter::EncodableValue(stream.codecID);
      videoStream[flutter::EncodableValue("codecName")] = 
        flutter::EncodableValue(stream.codecName);
      videoStream[flutter::EncodableValue("width")] = 
        flutter::EncodableValue(static_cast<int>(stream.pixelWidth));
      videoStream[flutter::EncodableValue("height")] = 
        flutter::EncodableValue(static_cast<int>(stream.pixelHeight));
      videoStream[flutter::EncodableValue("frameRate")] = 
        flutter::EncodableValue(stream.frameRate);
      
      videoStreams.push_back(flutter::EncodableValue(videoStream));
    }
    metadata[flutter::EncodableValue("videoStreams")] = 
      flutter::EncodableValue(videoStreams);
    
    // Add audio streams info
    flutter::EncodableList audioStreams;
    for (const auto& stream : extractor.getAudioStreams()) {
      flutter::EncodableMap audioStream;
      audioStream[flutter::EncodableValue("trackNumber")] = 
        flutter::EncodableValue(static_cast<int>(stream.trackNumber));
      audioStream[flutter::EncodableValue("codecId")] = 
        flutter::EncodableValue(stream.codecID);
      audioStream[flutter::EncodableValue("codecName")] = 
        flutter::EncodableValue(stream.codecName);
      audioStream[flutter::EncodableValue("channels")] = 
        flutter::EncodableValue(static_cast<int>(stream.channels));
      audioStream[flutter::EncodableValue("sampleRate")] = 
        flutter::EncodableValue(stream.samplingFrequency);
      audioStream[flutter::EncodableValue("bitDepth")] = 
        flutter::EncodableValue(static_cast<int>(stream.bitDepth));
      
      audioStreams.push_back(flutter::EncodableValue(audioStream));
    }
    metadata[flutter::EncodableValue("audioStreams")] = 
      flutter::EncodableValue(audioStreams);
    
    // Add attachments info
    flutter::EncodableList attachmentsList;
    for (size_t i = 0; i < extractor.getAttachments().size(); i++) {
      const auto& attachment = extractor.getAttachments()[i];
      flutter::EncodableMap attachmentMap;
      attachmentMap[flutter::EncodableValue("fileName")] = 
        flutter::EncodableValue(attachment.fileName);
      attachmentMap[flutter::EncodableValue("mimeType")] = 
        flutter::EncodableValue(attachment.mimeType);
      attachmentMap[flutter::EncodableValue("description")] = 
        flutter::EncodableValue(attachment.description);
      attachmentMap[flutter::EncodableValue("size")] = 
        flutter::EncodableValue(static_cast<int64_t>(attachment.dataSize));
      attachmentMap[flutter::EncodableValue("index")] = 
        flutter::EncodableValue(static_cast<int>(i));
      
      attachmentsList.push_back(flutter::EncodableValue(attachmentMap));
    }
    metadata[flutter::EncodableValue("attachments")] = 
      flutter::EncodableValue(attachmentsList);
    
    // Return the metadata
    result->Success(flutter::EncodableValue(metadata));
  }
  // Handle extracting an attachment from an MKV file
  else if (method == "extractMkvAttachment") {
    const auto* args = std::get_if<flutter::EncodableMap>(method_call.arguments());
    if (!args) {
      result->Error(
        "bad_args",
        "Expected a map with keys 'mkvPath', 'attachmentIndex', 'outputPath'.");
      return;
    }
    
    // Get the parameters
    std::string mkvPath;
    int attachmentIndex = -1;
    std::string outputPath;
    
    for (const auto& kv : *args) {
      const auto& key = kv.first;
      const auto& value = kv.second;
      if (auto keyStr = std::get_if<std::string>(&key)) {
        if (*keyStr == "mkvPath" && std::get_if<std::string>(&value)) {
          mkvPath = std::get<std::string>(value);
        } else if (*keyStr == "attachmentIndex" && std::get_if<int>(&value)) {
          attachmentIndex = std::get<int>(value);
        } else if (*keyStr == "outputPath" && std::get_if<std::string>(&value)) {
          outputPath = std::get<std::string>(value);
        }
      }
    }
    
    if (mkvPath.empty() || attachmentIndex < 0 || outputPath.empty()) {
      result->Error(
        "invalid_args",
        "Missing or invalid parameters.");
      return;
    }
    
    // Extract the attachment
    MkvMetadataExtractor extractor;
    if (!extractor.open(mkvPath)) {
      result->Error(
        "file_error",
        "Failed to open the MKV file.");
      return;
    }
    
    if (static_cast<size_t>(attachmentIndex) >= extractor.getAttachments().size()) {
      result->Error(
        "invalid_index",
        "Attachment index out of range.");
      return;
    }
    
    bool success = extractor.extractAttachment(attachmentIndex, outputPath);
    if (success) {
      result->Success(flutter::EncodableValue(true));
    } else {
      result->Error(
        "extract_error",
        "Failed to extract the attachment.");
    }
  }
  // In the HandleMethodCall method
  else if (method == "getVideoDuration") {
    std::wstring videoPathW;
    const auto* args = std::get_if<flutter::EncodableMap>(method_call.arguments());
    if (args) {
      for (const auto& kv : *args) {
        if (kv.first == flutter::EncodableValue("videoPath")) {
          videoPathW = getString(kv.second);  // Using your existing string conversion helper
        }
      }
    }
    
    if (videoPathW.empty()) {
      result->Error("invalid_args", "Missing or invalid 'videoPath' parameter.");
      return;
    }
    
    // Ensure GetVideoFileDuration is declared in video_duration.h and implemented in your project
    // Make sure GetVideoFileDuration is declared in video_duration.h and implemented in your project
    double duration = GetVideoFileDuration(videoPathW);
    result->Success(flutter::EncodableValue(duration));
  }
  // Handle any other method calls as not implemented
  else {
    result->NotImplemented();
  }
}

}  // namespace video_thumbnail_exporter
