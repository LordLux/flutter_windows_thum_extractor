import 'package:flutter_test/flutter_test.dart';
import 'package:video_thumbnail_exporter/video_thumbnail_exporter.dart';
import 'package:video_thumbnail_exporter/video_thumbnail_exporter_platform_interface.dart';
import 'package:video_thumbnail_exporter/video_thumbnail_exporter_method_channel.dart';
import 'package:plugin_platform_interface/plugin_platform_interface.dart';

class MockVideoThumbnailExporterPlatform
    with MockPlatformInterfaceMixin
    implements VideoThumbnailExporterPlatform {

  @override
  Future<String?> getPlatformVersion() => Future.value('42');
}

void main() {
  final VideoThumbnailExporterPlatform initialPlatform = VideoThumbnailExporterPlatform.instance;

  test('$MethodChannelVideoThumbnailExporter is the default instance', () {
    expect(initialPlatform, isInstanceOf<MethodChannelVideoThumbnailExporter>());
  });

  test('getPlatformVersion', () async {
    VideoThumbnailExporter videoThumbnailExporterPlugin = VideoThumbnailExporter();
    MockVideoThumbnailExporterPlatform fakePlatform = MockVideoThumbnailExporterPlatform();
    VideoThumbnailExporterPlatform.instance = fakePlatform;

    expect(await videoThumbnailExporterPlugin.getPlatformVersion(), '42');
  });
}
