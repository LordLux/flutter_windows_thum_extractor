#ifndef FLUTTER_PLUGIN_VIDEO_THUMBNAIL_EXPORTER_PLUGIN_H_
#define FLUTTER_PLUGIN_VIDEO_THUMBNAIL_EXPORTER_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>

#include <memory>
#include <string>
#include "thumbnail_exporter.h"

namespace video_thumbnail_exporter {

class VideoThumbnailExporterPlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows* registrar);

  VideoThumbnailExporterPlugin();
  virtual ~VideoThumbnailExporterPlugin();

  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);
};

}  // namespace video_thumbnail_exporter

#endif  // FLUTTER_PLUGIN_VIDEO_THUMBNAIL_EXPORTER_PLUGIN_H_
