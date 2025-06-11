#include "mkv_metadata_extractor_version5.h"
#include <windows.h>
#include <boost/nowide/fstream.hpp>
#include <boost/nowide/cstdio.hpp>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <stringapiset.h>
#include <io.h>

MkvMetadataExtractor::MkvMetadataExtractor() :
    fileSize(0),
    duration(0.0),
    timecodeScale(1000000) // Default timecode scale is 1ms
{
}

MkvMetadataExtractor::~MkvMetadataExtractor() {
    close();
}


bool MkvMetadataExtractor::open(const std::string& filePath) {
    // Close any previously opened file
    close();

    // Open file
    file.open(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // Check if file is an MKV by looking at the first 4 bytes
    char header[4];
    file.read(header, 4);
    file.seekg(0, std::ios::beg); // Reset position

    std::cout << "File header bytes: ";
    for (int i = 0; i < 4; i++) {
        std::cout << std::hex << (int)(unsigned char)header[i] << " ";
    }
    std::cout << std::dec << std::endl;

    // MKV files should start with EBML header (first byte 0x1A)
    if ((unsigned char)header[0] != 0x1A) {
        std::cout << "Not an MKV file (invalid header)" << std::endl;
        return false;
    }

    // Get file size
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Parse EBML header and contents
    return parseEBML();
}

void MkvMetadataExtractor::close() {
    if (file.is_open()) {
        file.close();
    }

    // Clear all stored data
    title.clear();
    duration = 0.0;
    timecodeScale = 1000000;
    muxingApp.clear();
    writingApp.clear();

    videoStreams.clear();
    audioStreams.clear();
    subtitleStreams.clear();
    otherStreams.clear();

    attachments.clear();
}

uint64_t MkvMetadataExtractor::getEstimatedBitrate() const {
    if (duration <= 0.0 || fileSize == 0) {
        return 0;
    }

    // Calculate bitrate in bits per second
    return static_cast<uint64_t>((fileSize * 8.0) / (duration / 1000.0));
}

bool MkvMetadataExtractor::extractAttachment(size_t index, const std::string& outputPath) {
    if (index >= attachments.size() || !file.is_open()) {
        return false;
    }

    const MkvAttachment& attachment = attachments[index];

    // Use Boost.Nowide to open output file with UTF-8 path
    FILE* outFile = boost::nowide::fopen(outputPath.c_str(), "wb");

    if (outFile == nullptr) {
        return false;
    }

    // Seek to attachment data in MKV file
    file.seekg(attachment.dataOffset, std::ios::beg);

    // Read and write in chunks
    const size_t bufferSize = 4096;
    char buffer[bufferSize];
    uint64_t remainingSize = attachment.dataSize;

    while (remainingSize > 0 && file.good()) {
        size_t bytesToRead = (remainingSize > bufferSize) ? bufferSize : static_cast<size_t>(remainingSize);
        file.read(buffer, bytesToRead);

        size_t bytesRead = file.gcount();
        fwrite(buffer, 1, bytesRead, outFile);

        remainingSize -= bytesRead;
    }

    fclose(outFile);
    return (remainingSize == 0);
}


bool MkvMetadataExtractor::parseEBML() {
    // Read EBML ID
    uint32_t id = readID();
    std::cout << "EBML ID: 0x" << std::hex << id << std::dec << std::endl;

    if (id != MkvIds::EBML) {
        std::cout << "Invalid EBML ID, expected: 0x" << std::hex << MkvIds::EBML << std::dec << std::endl;
        return false;
    }

    // Read EBML size
    uint64_t size = readSize();
    std::cout << "EBML size: " << size << std::endl;

    uint64_t endPos = file.tellg() + static_cast<std::streampos>(size);
    std::cout << "End position: " << endPos << std::endl;

    // Skip EBML content (not needed for metadata extraction)
    file.seekg(endPos, std::ios::beg);
    if (file.fail()) {
        std::cout << "Failed to seek to end of EBML header" << std::endl;
        file.clear(); // Clear error flags
        file.seekg(0, std::ios::beg); // Reset position
        return false;
    }

    // IMPORTANT CHANGE: Continue reading until end of file, not endPos
    // Look for the Segment element after the EBML header
    while (!file.eof()) {
        id = readID();
        if (file.eof() || file.fail()) break;  // Check if we reached end of file

        std::cout << "Next element ID: 0x" << std::hex << id << std::dec << std::endl;

        size = readSize();
        std::cout << "Element size: " << size << std::endl;

        if (id == MkvIds::Segment) {
            std::cout << "Found Segment element, parsing..." << std::endl;
            return parseSegment(size);
        }
        else {
            std::cout << "Skipping unknown element" << std::endl;
            skipBytes(size);
        }
    }

    std::cout << "No Segment element found" << std::endl;
    return false;
}

bool MkvMetadataExtractor::parseSegment(uint64_t size) {
    uint64_t endPos = 0;

    // Handle unknown size (size == 0 means unknown)
    if (size == 0) {
        endPos = fileSize;
    }
    else {
        endPos = file.tellg() + static_cast<std::streampos>(size);
    }

    // Parse segment content
    while (static_cast<uint64_t>(file.tellg()) < endPos && !file.eof()) {
        uint32_t id = readID();
        uint64_t elementSize = readSize();

        switch (id) {
        case MkvIds::SegmentInfo:
            parseSegmentInfo(elementSize);
            break;

        case MkvIds::Tracks:
            parseTracks(elementSize);
            break;

        case MkvIds::Attachments:
            parseAttachments(elementSize);
            break;

        default:
            // Skip unneeded elements
            skipBytes(elementSize);
            break;
        }
    }

    return true;
}

bool MkvMetadataExtractor::parseSegmentInfo(uint64_t size) {
    uint64_t endPos = file.tellg() + static_cast<std::streampos>(size);

    while (static_cast<uint64_t>(file.tellg()) < endPos && !file.eof()) {
        uint32_t id = readID();
        uint64_t elementSize = readSize();

        switch (id) {
        case MkvIds::TimecodeScale:
            timecodeScale = readUnsignedInt(elementSize);
            break;

        case MkvIds::Duration:
            if (elementSize == 4) {
                duration = readFloat(elementSize);
            }
            else if (elementSize == 8) {
                duration = readFloat(elementSize);
            }
            // Convert to milliseconds using timecode scale
            duration = duration * timecodeScale / 1000000.0;
            break;

        case MkvIds::Title:
            title = readUTF8(elementSize);
            break;

        case MkvIds::MuxingApp:
            muxingApp = readUTF8(elementSize);
            break;

        case MkvIds::WritingApp:
            writingApp = readUTF8(elementSize);
            break;

        default:
            // Skip unneeded elements
            skipBytes(elementSize);
            break;
        }
    }

    return true;
}

bool MkvMetadataExtractor::parseTracks(uint64_t size) {
    uint64_t endPos = file.tellg() + static_cast<std::streampos>(size);

    while (static_cast<uint64_t>(file.tellg()) < endPos && !file.eof()) {
        uint32_t id = readID();
        uint64_t elementSize = readSize();

        if (id == MkvIds::TrackEntry) {
            MkvStream stream;
            parseTrackEntry(elementSize, stream);
            addStream(stream);
        }
        else {
            // Skip unneeded elements
            skipBytes(elementSize);
        }
    }

    return true;
}

bool MkvMetadataExtractor::parseTrackEntry(uint64_t size, MkvStream& stream) {
    uint64_t endPos = file.tellg() + static_cast<std::streampos>(size);

    while (static_cast<uint64_t>(file.tellg()) < endPos && !file.eof()) {
        uint32_t id = readID();
        uint64_t elementSize = readSize();

        switch (id) {
        case MkvIds::TrackNumber:
            stream.trackNumber = readUnsignedInt(elementSize);
            break;

        case MkvIds::TrackUID:
            stream.trackUID = readUnsignedInt(elementSize);
            break;

        case MkvIds::TrackType:
            stream.trackType = static_cast<uint8_t>(readUnsignedInt(elementSize));
            break;

        case MkvIds::Name:
            stream.name = readUTF8(elementSize);
            break;

        case MkvIds::Language:
            stream.language = readString(elementSize);
            break;

        case MkvIds::CodecID:
            stream.codecID = readString(elementSize);
            break;

        case MkvIds::CodecName:
            stream.codecName = readUTF8(elementSize);
            break;

        case MkvIds::DefaultDuration:
            stream.defaultDuration = readUnsignedInt(elementSize);
            if (stream.defaultDuration > 0) {
                // Calculate frame rate from default duration (in nanoseconds)
                stream.frameRate = 1000000000.0 / stream.defaultDuration;
            }
            break;

        case MkvIds::Video:
            parseVideo(elementSize, stream);
            break;

        case MkvIds::Audio:
            parseAudio(elementSize, stream);
            break;

        case MkvIds::CodecPrivate:
            // Skip codec private data for now
            skipBytes(elementSize);
            break;

        default:
            // Skip unneeded elements
            skipBytes(elementSize);
            break;
        }
    }

    return true;
}

bool MkvMetadataExtractor::parseVideo(uint64_t size, MkvStream& stream) {
    uint64_t endPos = file.tellg() + static_cast<std::streampos>(size);

    while (static_cast<uint64_t>(file.tellg()) < endPos && !file.eof()) {
        uint32_t id = readID();
        uint64_t elementSize = readSize();

        switch (id) {
        case MkvIds::PixelWidth:
            stream.pixelWidth = readUnsignedInt(elementSize);
            break;

        case MkvIds::PixelHeight:
            stream.pixelHeight = readUnsignedInt(elementSize);
            break;

        case MkvIds::DisplayWidth:
            stream.displayWidth = readUnsignedInt(elementSize);
            break;

        case MkvIds::DisplayHeight:
            stream.displayHeight = readUnsignedInt(elementSize);
            break;

        case MkvIds::DisplayUnit:
            stream.displayUnit = static_cast<uint8_t>(readUnsignedInt(elementSize));
            break;

        case MkvIds::FrameRate:
            stream.frameRate = readFloat(elementSize);
            break;

        case MkvIds::ColourSpace:
        {
            uint64_t value = readUnsignedInt(elementSize);
            char hexStr[9];
            snprintf(hexStr, sizeof(hexStr), "%08X", static_cast<unsigned int>(value));
            stream.colorSpace = hexStr;
        }
        break;

        case MkvIds::Colour:
            parseColour(elementSize, stream);
            break;

        default:
            // Skip unneeded elements
            skipBytes(elementSize);
            break;
        }
    }

    return true;
}

bool MkvMetadataExtractor::parseAudio(uint64_t size, MkvStream& stream) {
    uint64_t endPos = file.tellg() + static_cast<std::streampos>(size);

    while (static_cast<uint64_t>(file.tellg()) < endPos && !file.eof()) {
        uint32_t id = readID();
        uint64_t elementSize = readSize();

        switch (id) {
        case MkvIds::SamplingFrequency:
            stream.samplingFrequency = readFloat(elementSize);
            break;

        case MkvIds::OutputSamplingFrequency:
            stream.outputSamplingFrequency = readFloat(elementSize);
            break;

        case MkvIds::Channels:
            stream.channels = static_cast<uint8_t>(readUnsignedInt(elementSize));
            break;

        case MkvIds::BitDepth:
            stream.bitDepth = static_cast<uint8_t>(readUnsignedInt(elementSize));
            break;

        default:
            // Skip unneeded elements
            skipBytes(elementSize);
            break;
        }
    }

    return true;
}

bool MkvMetadataExtractor::parseColour(uint64_t size, MkvStream& stream) {
    uint64_t endPos = file.tellg() + static_cast<std::streampos>(size);

    while (static_cast<uint64_t>(file.tellg()) < endPos && !file.eof()) {
        uint32_t id = readID();
        uint64_t elementSize = readSize();

        switch (id) {
        case MkvIds::MatrixCoefficients:
            stream.matrixCoefficients = static_cast<uint8_t>(readUnsignedInt(elementSize));
            break;

        case MkvIds::BitsPerChannel:
            stream.bitsPerChannel = static_cast<uint8_t>(readUnsignedInt(elementSize));
            break;

        case MkvIds::Range:
            stream.colorRange = static_cast<uint8_t>(readUnsignedInt(elementSize));
            break;

        case MkvIds::TransferCharacteristics:
            stream.transferCharacteristics = static_cast<uint8_t>(readUnsignedInt(elementSize));
            break;

        case MkvIds::Primaries:
            stream.colorPrimaries = static_cast<uint8_t>(readUnsignedInt(elementSize));
            break;

        default:
            // Skip unneeded elements
            skipBytes(elementSize);
            break;
        }
    }

    return true;
}

bool MkvMetadataExtractor::parseAttachments(uint64_t size) {
    uint64_t endPos = file.tellg() + static_cast<std::streampos>(size);

    while (static_cast<uint64_t>(file.tellg()) < endPos && !file.eof()) {
        uint32_t id = readID();
        uint64_t elementSize = readSize();

        if (id == MkvIds::AttachedFile) {
            MkvAttachment attachment;
            parseAttachedFile(elementSize, attachment);
            attachments.push_back(attachment);
        }
        else {
            // Skip unneeded elements
            skipBytes(elementSize);
        }
    }

    return true;
}

bool MkvMetadataExtractor::parseAttachedFile(uint64_t size, MkvAttachment& attachment) {
    uint64_t endPos = file.tellg() + static_cast<std::streampos>(size);

    while (static_cast<uint64_t>(file.tellg()) < endPos && !file.eof()) {
        uint32_t id = readID();
        uint64_t elementSize = readSize();

        switch (id) {
        case MkvIds::FileUID:
            attachment.uid = readUnsignedInt(elementSize);
            break;

        case MkvIds::FileName:
            attachment.fileName = readUTF8(elementSize);
            break;

        case MkvIds::FileDescription:
            attachment.description = readUTF8(elementSize);
            break;

        case MkvIds::FileMimeType:
            attachment.mimeType = readString(elementSize);
            break;

        case MkvIds::FileData:
            attachment.dataOffset = file.tellg();
            attachment.dataSize = elementSize;
            skipBytes(elementSize);
            break;

        default:
            // Skip unneeded elements
            skipBytes(elementSize);
            break;
        }
    }

    return true;
}

void MkvMetadataExtractor::addStream(const MkvStream& stream) {
    switch (stream.trackType) {
    case TRACK_TYPE_VIDEO:
        videoStreams.push_back(stream);
        break;

    case TRACK_TYPE_AUDIO:
        audioStreams.push_back(stream);
        break;

    case TRACK_TYPE_SUBTITLE:
        subtitleStreams.push_back(stream);
        break;

    default:
        otherStreams.push_back(stream);
        break;
    }
}

// EBML helper functions
uint32_t MkvMetadataExtractor::readID() {
    uint8_t firstByte;
    file.read(reinterpret_cast<char*>(&firstByte), 1);

    if (firstByte & 0x80) {
        return firstByte;
    }
    else if (firstByte & 0x40) {
        uint8_t secondByte;
        file.read(reinterpret_cast<char*>(&secondByte), 1);
        return (static_cast<uint32_t>(firstByte) << 8) | secondByte;
    }
    else if (firstByte & 0x20) {
        uint8_t bytes[3];
        file.read(reinterpret_cast<char*>(&bytes[1]), 2);
        bytes[0] = firstByte;
        return (static_cast<uint32_t>(bytes[0]) << 16) |
            (static_cast<uint32_t>(bytes[1]) << 8) |
            bytes[2];
    }
    else if (firstByte & 0x10) {
        uint8_t bytes[4];
        file.read(reinterpret_cast<char*>(&bytes[1]), 3);
        bytes[0] = firstByte;
        return (static_cast<uint32_t>(bytes[0]) << 24) |
            (static_cast<uint32_t>(bytes[1]) << 16) |
            (static_cast<uint32_t>(bytes[2]) << 8) |
            bytes[3];
    }

    return 0;
}

uint64_t MkvMetadataExtractor::readSize() {
    uint8_t firstByte;
    file.read(reinterpret_cast<char*>(&firstByte), 1);

    int len = 0;
    uint64_t value = 0;

    // Determine length
    if (firstByte & 0x80) {
        len = 1;
        value = firstByte & 0x7F;
    }
    else if (firstByte & 0x40) {
        len = 2;
        value = firstByte & 0x3F;
    }
    else if (firstByte & 0x20) {
        len = 3;
        value = firstByte & 0x1F;
    }
    else if (firstByte & 0x10) {
        len = 4;
        value = firstByte & 0x0F;
    }
    else if (firstByte & 0x08) {
        len = 5;
        value = firstByte & 0x07;
    }
    else if (firstByte & 0x04) {
        len = 6;
        value = firstByte & 0x03;
    }
    else if (firstByte & 0x02) {
        len = 7;
        value = firstByte & 0x01;
    }
    else if (firstByte & 0x01) {
        len = 8;
        value = 0;
    }
    else {
        // Invalid size
        return 0;
    }

    // Read remaining bytes
    for (int i = 1; i < len; i++) {
        uint8_t nextByte;
        file.read(reinterpret_cast<char*>(&nextByte), 1);
        value = (value << 8) | nextByte;
    }

    return value;
}

uint64_t MkvMetadataExtractor::readUnsignedInt(uint64_t size) {
    if (size > 8) {
        skipBytes(size);
        return 0;
    }

    uint64_t value = 0;
    for (uint64_t i = 0; i < size; i++) {
        uint8_t byte;
        file.read(reinterpret_cast<char*>(&byte), 1);
        value = (value << 8) | byte;
    }

    return value;
}

int64_t MkvMetadataExtractor::readSignedInt(uint64_t size) {
    uint64_t value = readUnsignedInt(size);

    // Convert to signed
    if (size == 1) {
        return static_cast<int8_t>(value);
    }
    else if (size == 2) {
        return static_cast<int16_t>(value);
    }
    else if (size <= 4) {
        return static_cast<int32_t>(value);
    }
    else {
        return static_cast<int64_t>(value);
    }
}

double MkvMetadataExtractor::readFloat(uint64_t size) {
    if (size == 4) {
        float value;
        uint32_t intValue = static_cast<uint32_t>(readUnsignedInt(size));
        memcpy(&value, &intValue, sizeof(value));
        return value;
    }
    else if (size == 8) {
        double value;
        uint64_t intValue = readUnsignedInt(size);
        memcpy(&value, &intValue, sizeof(value));
        return value;
    }

    skipBytes(size);
    return 0.0;
}

std::string MkvMetadataExtractor::readString(uint64_t size) {
    std::string result;
    result.resize(size);

    file.read(&result[0], size);

    // Remove any null terminators
    size_t nullPos = result.find('\0');
    if (nullPos != std::string::npos) {
        result.resize(nullPos);
    }

    return result;
}

std::string MkvMetadataExtractor::readUTF8(uint64_t size) {
    return readString(size);
}

void MkvMetadataExtractor::skipBytes(uint64_t size) {
    file.seekg(size, std::ios::cur);
}