#include "include/video_thumbnail_exporter/video_thumbnail_exporter_plugin_c_api.h"

#include <flutter/plugin_registrar_windows.h>

#include "video_thumbnail_exporter_plugin.h"

void VideoThumbnailExporterPluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  video_thumbnail_exporter::VideoThumbnailExporterPlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
