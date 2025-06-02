import 'dart:io';

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

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  Future<void> _processFilePath(String filePath) async {
    setState(() {
      _status = 'Processing...';
      _thumbnailPath = null;
    });

    filePath = filePath.replaceAll('"', "");

    File file = File(filePath);
    if (!await file.exists()) {
      setState(() {
        _status = 'Error: File does not exist';
      });
      return;
    }

    try {
      final fileName = path.basename(filePath);
      final tempDir = await getTemporaryDirectory();
      final fileNameWithoutExt = path.basenameWithoutExtension(fileName);
      final tempPath = path.join(tempDir.path, '$fileNameWithoutExt.png');

      final bool success = await VideoThumbnailExporter.getThumbnail(
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

      await _openFile(tempPath);
    } catch (e) {
      setState(() {
        _status = 'Error: $e';
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
        title: const Text('Thumbnail Extractor'),
      ),
      body: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            TextField(
              controller: _controller,
              decoration: const InputDecoration(
                labelText: 'Enter file path',
                hintText: 'e.g., C:\\path\\to\\file.ext',
                border: OutlineInputBorder(),
              ),
              onSubmitted: _processFilePath,
            ),
            const SizedBox(height: 16),
            Text(_status),
            const SizedBox(height: 16),
            if (_thumbnailPath != null) ...[
              const Text('Thumbnail Preview:'),
              const SizedBox(height: 8),
              Image.file(
                File(_thumbnailPath!),
                width: 512,
                fit: BoxFit.contain,
                errorBuilder: (context, error, stackTrace) {
                  // Log the error for debugging
                  print('Error loading thumbnail: $error\n$stackTrace');

                  // More informative error message for the user
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
            ],
          ],
        ),
      ),
    );
  }
}
