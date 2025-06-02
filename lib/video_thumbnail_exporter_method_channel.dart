import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

import 'video_thumbnail_exporter_platform_interface.dart';

/// An implementation of [VideoThumbnailExporterPlatform] that uses method channels.
class MethodChannelVideoThumbnailExporter extends VideoThumbnailExporterPlatform {
  /// The method channel used to interact with the native platform.
  @visibleForTesting
  final methodChannel = const MethodChannel('video_thumbnail_exporter');

  @override
  Future<String?> getPlatformVersion() async {
    final version = await methodChannel.invokeMethod<String>('getPlatformVersion');
    return version;
  }
}
