import 'package:plugin_platform_interface/plugin_platform_interface.dart';

import 'video_thumbnail_exporter_method_channel.dart';

abstract class VideoThumbnailExporterPlatform extends PlatformInterface {
  /// Constructs a VideoThumbnailExporterPlatform.
  VideoThumbnailExporterPlatform() : super(token: _token);

  static final Object _token = Object();

  static VideoThumbnailExporterPlatform _instance = MethodChannelVideoThumbnailExporter();

  /// The default instance of [VideoThumbnailExporterPlatform] to use.
  ///
  /// Defaults to [MethodChannelVideoThumbnailExporter].
  static VideoThumbnailExporterPlatform get instance => _instance;

  /// Platform-specific implementations should set this with their own
  /// platform-specific class that extends [VideoThumbnailExporterPlatform] when
  /// they register themselves.
  static set instance(VideoThumbnailExporterPlatform instance) {
    PlatformInterface.verifyToken(instance, _token);
    _instance = instance;
  }
}
