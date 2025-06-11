import 'dart:io';

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'dart:async';

import 'package:path/path.dart' as path;
import 'package:flutter/services.dart';
import 'package:path_provider/path_provider.dart';
import 'package:url_launcher/url_launcher.dart';
import 'package:video_thumbnail_exporter/video_thumbnail_exporter.dart';

void main() {
  runApp(MaterialApp(home: const MyApp()));
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  final TextEditingController _controller = TextEditingController();
  String _status = '';
  String? _thumbnailPath;
  Map<dynamic, dynamic>? _metadata;
  bool _isProcessing = false;

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  Future<void> _processFilePath(String filePath) async {
    setState(() {
      _status = 'Processing...';
      _thumbnailPath = null;
      _metadata = null;
      _isProcessing = true;
    });

    filePath = filePath.replaceAll('"', "").replaceAll("'", "").replaceAll("\\", Platform.pathSeparator).replaceAll("/", Platform.pathSeparator);

    File file = File(filePath);
    if (!await file.exists()) {
      setState(() {
        _status = 'Error: File does not exist';
        _isProcessing = false;
      });
      return;
    }

    try {
      // Generate thumbnail
      await _generateThumbnail(filePath);

      // Extract metadata if it's an MKV file
      final String extension = path.extension(filePath).toLowerCase();
      if (extension == '.mkv') await _extractMetadata(filePath);

      setState(() {
        _isProcessing = false;
      });
    } catch (e) {
      setState(() {
        _status = 'Error: $e';
        _isProcessing = false;
      });
    }
  }

  Future<void> _generateThumbnail(String filePath) async {
    try {
      final fileName = path.basename(filePath);
      final tempDir = await getTemporaryDirectory();
      final fileNameWithoutExt = path.basenameWithoutExtension(fileName);
      final tempPath = path.join(tempDir.path, '$fileNameWithoutExt.png');

      final bool success = await VideoDataExtractor.extractCachedThumbnail(
        videoPath: filePath,
        outputPath: tempPath,
        size: 1024,
      );

      if (!success) {
        setState(() {
          _status = 'Failed to generate thumbnail';
        });
        return;
      }

      final thumbnailFile = File(tempPath);
      if (!await thumbnailFile.exists()) {
        setState(() {
          _status = 'Error: Thumbnail file was not created';
        });
        return;
      }

      setState(() {
        _status = 'Thumbnail generated at: "$tempPath"';
        _thumbnailPath = tempPath;
      });
    } catch (e) {
      setState(() {
        _status = '$_status\nThumbnail error: $e';
      });
    }
  }

  Future<void> _extractMetadata(String filePath) async {
    try {
      final metadata = await VideoDataExtractor.getVideoMetadata(
        mkvPath: filePath,
      );

      setState(() {
        _metadata = metadata;
        _status = '$_status\nMetadata extracted successfully';
      });
    } catch (e, st) {
      setState(() {
        _status = '$_status\nMetadata error: $e\n$st';
      });
    }
  }

  Future<void> _extractAttachment(String videoPath, int index) async {
    try {
      setState(() {
        _status = '$_status\nExtracting attachment #$index...';
      });

      // Create a unique filename for the attachment
      final tempDir = await getTemporaryDirectory();
      final attachmentInfo = _metadata?['attachments'][index];
      final fileName = attachmentInfo?['fileName'] ?? 'attachment_$index';
      final outputPath = path.join(tempDir.path, fileName);

      videoPath = videoPath.replaceAll('"', "").replaceAll("'", "").replaceAll("\\", Platform.pathSeparator).replaceAll("/", Platform.pathSeparator);
      final success = await VideoDataExtractor.extractVideoAttachment(
        mkvPath: videoPath,
        outputPath: outputPath,
        attachmentIndex: index,
      );

      if (success) {
        setState(() {
          _status = '$_status\nAttachment extracted to: $outputPath';
        });
        await _openFile(outputPath);
      } else {
        setState(() {
          _status = '$_status\nFailed to extract attachment';
        });
      }
    } catch (e) {
      setState(() {
        _status = '$_status\nAttachment extraction error: $e';
      });
    }
  }

  Future<void> _openFile(String filePath) async {
    try {
      final uri = Uri.file(filePath);
      if (!await launchUrl(uri)) {
        setState(() {
          _status = '$_status\nCould not open the file';
        });
      }
    } catch (e) {
      setState(() {
        _status = '$_status\nError opening file: $e';
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Video Data Extractor'),
      ),
      body: Padding(
        padding: const EdgeInsets.all(16.0),
        child: SingleChildScrollView(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              TextField(
                controller: _controller,
                decoration: const InputDecoration(
                  labelText: 'Enter video file path',
                  hintText: 'e.g., C:\\path\\to\\video.mkv',
                  border: OutlineInputBorder(),
                ),
                onSubmitted: _processFilePath,
              ),
              const SizedBox(height: 16),
              Row(
                children: [
                  ElevatedButton(
                    onPressed: _isProcessing ? null : () => _processFilePath(_controller.text),
                    child: const Text('Process Video'),
                  ),
                  if (_isProcessing)
                    const Padding(
                      padding: EdgeInsets.only(left: 16.0),
                      child: CircularProgressIndicator(),
                    ),
                ],
              ),
              const SizedBox(height: 16),
              Container(
                padding: const EdgeInsets.all(8),
                decoration: BoxDecoration(
                  border: Border.all(color: Colors.grey),
                  borderRadius: BorderRadius.circular(4),
                ),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    const Text('Status:', style: TextStyle(fontWeight: FontWeight.bold)),
                    const SizedBox(height: 4),
                    Text(_status),
                  ],
                ),
              ),
              const SizedBox(height: 16),

              // Thumbnail Section
              if (_thumbnailPath != null) ...[
                const Text('Thumbnail Preview:', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
                const SizedBox(height: 8),
                Image.file(
                  File(_thumbnailPath!),
                  width: 512,
                  fit: BoxFit.contain,
                  errorBuilder: (context, error, stackTrace) {
                    return Column(
                      children: [
                        const Icon(Icons.broken_image, size: 64, color: Colors.grey),
                        const SizedBox(height: 8),
                        Text('Unable to display thumbnail: ${error.toString().split('\n').first}'),
                        TextButton(
                          onPressed: () => _openFile(_thumbnailPath!),
                          child: const Text('Try opening externally'),
                        ),
                      ],
                    );
                  },
                ),
                const SizedBox(height: 16),
              ],

              // Metadata Section
              if (_metadata != null) ...[
                const Text('Video Metadata:', style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
                const SizedBox(height: 8),
                Card(
                  child: Padding(
                    padding: const EdgeInsets.all(12.0),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        _buildMetadataField('Title', _metadata!['title']),
                        _buildMetadataField('Duration', '${(_metadata!['duration'] as double? ?? 0.0).toStringAsFixed(2)} ms'),
                        _buildMetadataField('Bitrate', '${(_metadata!['bitrate'] as int? ?? 0) ~/ 1000} kbps'),
                        _buildMetadataField('Muxing App', _metadata!['muxingApp']),
                        _buildMetadataField('Writing App', _metadata!['writingApp']),

                        const SizedBox(height: 16),
                        const Text('Video Streams:', style: TextStyle(fontWeight: FontWeight.bold)),
                        const SizedBox(height: 8),

                        // Video Streams
                        ..._buildVideoStreams(),

                        const SizedBox(height: 16),
                        const Text('Audio Streams:', style: TextStyle(fontWeight: FontWeight.bold)),
                        const SizedBox(height: 8),

                        // Audio Streams
                        ..._buildAudioStreams(),

                        // Attachments Section
                        if ((_metadata!['attachments'] as List?)?.isNotEmpty ?? false) ...[
                          const SizedBox(height: 16),
                          const Text('Attachments:', style: TextStyle(fontWeight: FontWeight.bold)),
                          const SizedBox(height: 8),

                          // Attachments
                          ..._buildAttachments(),
                        ],
                      ],
                    ),
                  ),
                ),
              ],
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildMetadataField(String label, dynamic value) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 8.0),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          SizedBox(
            width: 100,
            child: Text('$label:', style: const TextStyle(fontWeight: FontWeight.bold)),
          ),
          Expanded(
            child: Text(value?.toString() ?? 'N/A'),
          ),
        ],
      ),
    );
  }

  List<Widget> _buildVideoStreams() {
    final List<dynamic> videoStreams = _metadata?['videoStreams'] ?? [];
    if (videoStreams.isEmpty) return [const Text('No video streams found')];

    return videoStreams.map<Widget>((stream) {
      return Card(
        margin: const EdgeInsets.only(bottom: 8),
        child: Padding(
          padding: const EdgeInsets.all(8.0),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              _buildMetadataField('Track', '${stream['trackNumber']}'),
              _buildMetadataField('Codec', '${stream['codecName']} (${stream['codecId']})'),
              _buildMetadataField('Resolution', '${stream['width']}x${stream['height']}'),
              _buildMetadataField('Frame Rate', '${stream['frameRate']}'),
            ],
          ),
        ),
      );
    }).toList();
  }

  List<Widget> _buildAudioStreams() {
    final List<dynamic> audioStreams = _metadata?['audioStreams'] ?? [];
    if (audioStreams.isEmpty) return [const Text('No audio streams found')];

    return audioStreams.map<Widget>((stream) {
      return Card(
        margin: const EdgeInsets.only(bottom: 8),
        child: Padding(
          padding: const EdgeInsets.all(8.0),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              _buildMetadataField('Track', '${stream['trackNumber']}'),
              _buildMetadataField('Codec', '${stream['codecName']} (${stream['codecId']})'),
              _buildMetadataField('Channels', '${stream['channels']}'),
              _buildMetadataField('Sample Rate', '${stream['sampleRate']} Hz'),
              _buildMetadataField('Bit Depth', '${stream['bitDepth']} bit'),
            ],
          ),
        ),
      );
    }).toList();
  }

  List<Widget> _buildAttachments() {
    final List<dynamic> attachments = _metadata?['attachments'] ?? [];
    if (attachments.isEmpty) return [const Text('No attachments found')];

    return attachments.map<Widget>((attachment) {
      final int index = attachment['index'] as int;
      return Card(
        margin: const EdgeInsets.only(bottom: 8),
        child: Padding(
          padding: const EdgeInsets.all(8.0),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              _buildMetadataField('File Name', attachment['fileName']),
              _buildMetadataField('MIME Type', attachment['mimeType']),
              _buildMetadataField('Description', attachment['description']),
              _buildMetadataField('Size', '${(attachment['size'] as int) ~/ 1024} KB'),
              const SizedBox(height: 8),
              ElevatedButton(
                onPressed: () => _extractAttachment(_controller.text, index),
                child: const Text('Extract Attachment'),
              ),
            ],
          ),
        ),
      );
    }).toList();
  }
}
