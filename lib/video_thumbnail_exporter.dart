// ignore_for_file: curly_braces_in_flow_control_structures

import 'dart:async';
import 'dart:io';
import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

String get assets => "${(Platform.resolvedExecutable.split(ps)..removeLast()).join(ps)}${ps}data${ps}flutter_assets${ps}assets";
String get ps => Platform.pathSeparator;

class VideoDataExtractor {
  static const MethodChannel _channel = MethodChannel('video_thumbnail_exporter');

  /// Initializes the native components of the video extractor
  /// to avoid first-time performance penalty in subsequent calls.
  ///
  /// Throws an exception if initialization fails.
  static Future<void> initialize() async {
    try {
      // The native side can initialize without a file, so we just call it.
      await _channel.invokeMethod<bool>('initializeExtractor');
    } catch (e) {
      // It's better to rethrow or handle the error meaningfully.
      // Silently ignoring it can hide problems.
      debugPrint('VideoDataExtractor initialization error: $e');
      rethrow;
    }
  }

  /// Returns true if the thumbnail was successfully written to [outputPath].
  ///
  /// Otherwise throws a PlatformException.
  static Future<bool> extractCachedThumbnail({
    /// The path to the video file to extract the thumbnail from.
    required String videoPath,

    /// The path where the thumbnail image will be saved.
    required String outputPath,

    /// The size of the thumbnail in pixels. Usual values are 256, 512, or 1024.
    required int size,
  }) async {
    final Map<String, dynamic> args = <String, dynamic>{
      'videoPath': videoPath,
      'outputPath': outputPath,
      'size': size,
    };

    return await _channel.invokeMethod<bool>('getThumbnail', args) ?? false;
  }

  /// Returns file metadata including creation time, modified time, access time, and file size.
  ///
  /// All times are in milliseconds since epoch (UTC).
  /// File size is in bytes.
  ///
  /// Otherwise throws a PlatformException.
  static Future<Map<String, dynamic>> getFileMetadata({
    /// The path to the file to extract metadata from.
    required String filePath,
  }) async {
    final Map<String, dynamic> args = <String, dynamic>{
      'filePath': filePath,
    };

    return Map<String, dynamic>.from(await _channel.invokeMethod<Map>('getFileMetadata', args) ?? {});
  }

  /// Returns the Video Metadata as a Map.
  ///
  /// Throws an ArgumentError if the video file is not an MKV file.
  ///
  /// Otherwise throws a PlatformException.
  static Future<Map<dynamic, dynamic>> getMkvMetadata({
    /// The path to the video file to extract metadata from.
    required String mkvPath,
  }) async {
    final Map<String, dynamic> args = <String, dynamic>{
      'mkvPath': mkvPath,
    };

    if (mkvPath.split('.').last.toLowerCase() != 'mkv') //
      throw ArgumentError('Only MKV files are supported for metadata extraction for now.');

    return await _channel.invokeMethod<Map<dynamic, dynamic>>('getMkvMetadata', args) ?? {};
  }

  /// Returns true if the attachment was successfully written to [outputPath].
  ///
  /// Otherwise throws a PlatformException.
  static Future<bool> extractVideoAttachment({
    /// The path to the video file to extract the attachment from.
    required String mkvPath,

    /// The path where the attachment will be saved.
    required String outputPath,

    /// The index of the attachment to extract. Defaults to 0 (the first attachment).
    int attachmentIndex = 0,
  }) async {
    final Map<String, dynamic> args = <String, dynamic>{
      'mkvPath': mkvPath,
      'attachmentIndex': attachmentIndex,
      'outputPath': outputPath,
    };

    if (mkvPath.split('.').last.toLowerCase() != 'mkv') //
      throw ArgumentError('Only MKV files are supported for attachment extraction.');

    return await _channel.invokeMethod<bool>('extractMkvAttachment', args) ?? false;
  }

  /// Returns just the duration of the video in milliseconds.
  ///
  /// This is optimized for speed by only extracting the duration metadata.
  /// Returns 0.0 if duration couldn't be determined.
  ///
  /// Otherwise throws a PlatformException.
  static Future<double> getVideoDuration({
    /// The path to the video file to extract the duration from.
    required String videoPath,
  }) async {
    final Map<String, dynamic> args = <String, dynamic>{
      'videoPath': videoPath,
    };

    return await _channel.invokeMethod<double>('getVideoDuration', args) ?? 0.0;
  }
}
