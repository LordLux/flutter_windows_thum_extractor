import 'dart:async';
import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

class VideoThumbnailExporter {
  static const MethodChannel _channel =
      MethodChannel('video_thumbnail_exporter');

  /// Returns true if the thumbnail was successfully written to [outputPath].
  /// Otherwise throws a PlatformException.
  static Future<bool> getThumbnail({
    required String videoPath,
    required String outputPath,
    required int size,
  }) async {
    final Map<String, dynamic> args = <String, dynamic>{
      'videoPath': videoPath,
      'outputPath': outputPath,
      'size': size,
    };

    final bool success = await _channel.invokeMethod<bool>(
          'getThumbnail',
          args,
        ) ??
        false;
    return success;
  }
}
