//
//  Generated file. Do not edit.
//

// clang-format off

#include "generated_plugin_registrant.h"

#include <url_launcher_windows/url_launcher_windows.h>
#include <video_thumbnail_exporter/video_thumbnail_exporter_plugin_c_api.h>

void RegisterPlugins(flutter::PluginRegistry* registry) {
  UrlLauncherWindowsRegisterWithRegistrar(
      registry->GetRegistrarForPlugin("UrlLauncherWindows"));
  VideoThumbnailExporterPluginCApiRegisterWithRegistrar(
      registry->GetRegistrarForPlugin("VideoThumbnailExporterPluginCApi"));
}
