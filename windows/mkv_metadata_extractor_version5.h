#ifndef MKV_METADATA_EXTRACTOR_H
#define MKV_METADATA_EXTRACTOR_H

#include <string>
#include <boost/nowide/fstream.hpp>
#include <vector>
#include <map>
#include <fstream>
#include <memory>
#include <cstdint>

// EBML ID constants for Matroska elements
namespace MkvIds {
    // EBML
    const uint32_t EBML = 0x1A45DFA3;
    const uint32_t EBMLVersion = 0x4286;
    const uint32_t EBMLReadVersion = 0x42F7;
    const uint32_t EBMLMaxIDLength = 0x42F2;
    const uint32_t EBMLMaxSizeLength = 0x42F3;
    const uint32_t DocType = 0x4282;
    const uint32_t DocTypeVersion = 0x4287;
    const uint32_t DocTypeReadVersion = 0x4285;

    // Segment
    const uint32_t Segment = 0x18538067;

    // Segment Info
    const uint32_t SegmentInfo = 0x1549A966;
    const uint32_t TimecodeScale = 0x2AD7B1;
    const uint32_t Duration = 0x4489;
    const uint32_t Title = 0x7BA9;
    const uint32_t MuxingApp = 0x4D80;
    const uint32_t WritingApp = 0x5741;

    // Tracks
    const uint32_t Tracks = 0x1654AE6B;
    const uint32_t TrackEntry = 0xAE;
    const uint32_t TrackNumber = 0xD7;
    const uint32_t TrackUID = 0x73C5;
    const uint32_t TrackType = 0x83;
    const uint32_t Name = 0x536E;
    const uint32_t Language = 0x22B59C;
    const uint32_t CodecID = 0x86;
    const uint32_t CodecPrivate = 0x63A2;
    const uint32_t CodecName = 0x258688;
    const uint32_t DefaultDuration = 0x23E383;

    // Video specific
    const uint32_t Video = 0xE0;
    const uint32_t PixelWidth = 0xB0;
    const uint32_t PixelHeight = 0xBA;
    const uint32_t DisplayWidth = 0x54B0;
    const uint32_t DisplayHeight = 0x54BA;
    const uint32_t DisplayUnit = 0x54B2;
    const uint32_t FrameRate = 0x2383E3;
    const uint32_t ColourSpace = 0x2EB524;

    // Video colour
    const uint32_t Colour = 0x55B0;
    const uint32_t MatrixCoefficients = 0x55B1;
    const uint32_t BitsPerChannel = 0x55B2;
    const uint32_t Range = 0x55B9;
    const uint32_t TransferCharacteristics = 0x55BA;
    const uint32_t Primaries = 0x55BB;

    // Audio specific
    const uint32_t Audio = 0xE1;
    const uint32_t SamplingFrequency = 0xB5;
    const uint32_t OutputSamplingFrequency = 0x78B5;
    const uint32_t Channels = 0x9F;
    const uint32_t BitDepth = 0x6264;

    // Attachments
    const uint32_t Attachments = 0x1941A469;
    const uint32_t AttachedFile = 0x61A7;
    const uint32_t FileDescription = 0x467E;
    const uint32_t FileName = 0x466E;
    const uint32_t FileMimeType = 0x4660;
    const uint32_t FileData = 0x465C;
    const uint32_t FileUID = 0x46AE;
}

// Track types
enum MkvTrackType {
    TRACK_TYPE_VIDEO = 0x01,
    TRACK_TYPE_AUDIO = 0x02,
    TRACK_TYPE_COMPLEX = 0x03,
    TRACK_TYPE_SUBTITLE = 0x11,
    TRACK_TYPE_LOGO = 0x10,
    TRACK_TYPE_BUTTONS = 0x12,
    TRACK_TYPE_CONTROL = 0x20
};

// Stream information structure
struct MkvStream {
    uint64_t trackNumber;
    uint64_t trackUID;
    uint8_t trackType;
    std::string codecID;
    std::string codecName;
    std::string language;
    std::string name;
    uint64_t defaultDuration;
    double frameRate;

    // Video specific
    uint64_t pixelWidth;
    uint64_t pixelHeight;
    uint64_t displayWidth;
    uint64_t displayHeight;
    uint8_t displayUnit;
    std::string colorSpace;
    uint8_t matrixCoefficients;
    uint8_t bitsPerChannel;
    uint8_t colorRange;
    uint8_t transferCharacteristics;
    uint8_t colorPrimaries;

    // Audio specific
    double samplingFrequency;
    double outputSamplingFrequency;
    uint8_t channels;
    uint8_t bitDepth;

    MkvStream() :
        trackNumber(0), trackUID(0), trackType(0), defaultDuration(0), frameRate(0),
        pixelWidth(0), pixelHeight(0), displayWidth(0), displayHeight(0), displayUnit(0),
        matrixCoefficients(0), bitsPerChannel(0), colorRange(0),
        transferCharacteristics(0), colorPrimaries(0),
        samplingFrequency(0), outputSamplingFrequency(0), channels(0), bitDepth(0) {
    }
};

// Attachment information
struct MkvAttachment {
    uint64_t uid;
    std::string fileName;
    std::string description;
    std::string mimeType;
    uint64_t dataSize;
    uint64_t dataOffset;

    MkvAttachment() : uid(0), dataSize(0), dataOffset(0) {}
};

// Main class for MKV metadata extraction
class MkvMetadataExtractor {
public:
    MkvMetadataExtractor();
    ~MkvMetadataExtractor();

    // Open MKV file and parse metadata
    bool open(const std::string& filePath);

    // Close file and cleanup
    void close();

    // Get general information
    std::string getTitle() const { return title; }
    double getDuration() const { return duration; }
    uint64_t getTimecodeScale() const { return timecodeScale; }
    std::string getMuxingApp() const { return muxingApp; }
    std::string getWritingApp() const { return writingApp; }

    // Get stream information
    const std::vector<MkvStream>& getVideoStreams() const { return videoStreams; }
    const std::vector<MkvStream>& getAudioStreams() const { return audioStreams; }
    const std::vector<MkvStream>& getSubtitleStreams() const { return subtitleStreams; }

    // Get attachment information
    const std::vector<MkvAttachment>& getAttachments() const { return attachments; }

    // Extract attachment data to file
    bool extractAttachment(size_t index, const std::string& outputPath);

    // Calculate estimated bitrate
    uint64_t getEstimatedBitrate() const;

private:
    boost::nowide::ifstream file;
    uint64_t fileSize;

    // General info
    std::string title;
    double duration;
    uint64_t timecodeScale;
    std::string muxingApp;
    std::string writingApp;

    // Stream collections
    std::vector<MkvStream> videoStreams;
    std::vector<MkvStream> audioStreams;
    std::vector<MkvStream> subtitleStreams;
    std::vector<MkvStream> otherStreams;

    // Attachments
    std::vector<MkvAttachment> attachments;

    // EBML parsing
    bool parseEBML();
    bool parseSegment(uint64_t size);
    bool parseSegmentInfo(uint64_t size);
    bool parseTracks(uint64_t size);
    bool parseTrackEntry(uint64_t size, MkvStream& stream);
    bool parseVideo(uint64_t size, MkvStream& stream);
    bool parseAudio(uint64_t size, MkvStream& stream);
    bool parseColour(uint64_t size, MkvStream& stream);
    bool parseAttachments(uint64_t size);
    bool parseAttachedFile(uint64_t size, MkvAttachment& attachment);

    // EBML helper functions
    uint32_t readID();
    uint64_t readSize();
    uint64_t readUnsignedInt(uint64_t size);
    int64_t readSignedInt(uint64_t size);
    double readFloat(uint64_t size);
    std::string readString(uint64_t size);
    std::string readUTF8(uint64_t size);
    void skipBytes(uint64_t size);

    // Add stream to appropriate collection based on type
    void addStream(const MkvStream& stream);
};

#endif // MKV_METADATA_EXTRACTOR_H