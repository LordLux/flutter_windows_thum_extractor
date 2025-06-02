#include "video_thumbnail_exporter_plugin.h"
#include "thumbnail_exporter.h"

// This must be included before many other Windows headers.
#include <windows.h>

// For getPlatformVersion; remove unless needed for your plugin implementation.
#include <VersionHelpers.h>

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <memory>
#include <sstream>

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

void VideoThumbnailExporterPlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue> &method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  auto method = method_call.method_name();
  if (method == "getThumbnail") {
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
  } else {
    result->NotImplemented();
  }
}

}  // namespace video_thumbnail_exporter
