#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <taglib/apefooter.h>
#include <taglib/apeitem.h>
#include <taglib/apetag.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/fileref.h>
#include <taglib/id3v2frame.h>
#include <taglib/id3v2tag.h>
#include <taglib/mpegfile.h>
#include <taglib/synchronizedlyricsframe.h>
#include <taglib/textidentificationframe.h>
#include <taglib/xiphcomment.h>

#include <taglib/tag.h>
#include "taglibReaders.h"

std::vector<unsigned char> decodeBase64(const std::string &encoded_string)
{
        const size_t MAX_DECODED_SIZE = 100 * 1024 * 1024; // 100 MB
        const size_t MAX_ENCODED_SIZE =
            (MAX_DECODED_SIZE * 4) / 3 + 4; // Max base64 size + padding
        const std::string base64_chars =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        auto is_base64 = [&](unsigned char c) -> bool { return base64_chars.find(c) != std::string::npos; };

        auto base64_index = [&](unsigned char c) -> unsigned char {
                auto pos = base64_chars.find(c);
                return pos != std::string::npos
                           ? static_cast<unsigned char>(pos)
                           : 0;
        };

        size_t in_len = encoded_string.size();
        size_t i = 0;
        size_t in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];

        // Early check to prevent any overflow issues
        if (in_len > MAX_ENCODED_SIZE) {
                throw std::runtime_error(
                    "Base64 input too large: exceeds reasonable limit");
        }

        // Rough estimate of decoded size
        size_t decoded_size = (in_len * 3) / 4;
        if (decoded_size > MAX_DECODED_SIZE)
                throw std::runtime_error(
                    "Base64 input too large: exceeds 100 MB limit");

        std::vector<unsigned char> decoded_data;
        try {
                decoded_data.reserve(decoded_size);
        } catch (const std::bad_alloc &) {
                throw std::runtime_error(
                    "Cannot allocate memory for base64 decoding");
        }

        while (in_len-- && encoded_string[in_] != '=' &&
               is_base64(encoded_string[in_])) {
                char_array_4[i++] = encoded_string[in_++];
                if (i == 4) {
                        for (i = 0; i < 4; i++)
                                char_array_4[i] = base64_index(char_array_4[i]);

                        char_array_3[0] = (char_array_4[0] << 2) +
                                          ((char_array_4[1] & 0x30) >> 4);
                        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) +
                                          ((char_array_4[2] & 0x3c) >> 2);
                        char_array_3[2] =
                            ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

                        if (decoded_data.size() + 3 > MAX_DECODED_SIZE) {
                                throw std::runtime_error(
                                    "Decoded data exceeds size limit during "
                                    "processing");
                        }

                        for (i = 0; i < 3; i++)
                                decoded_data.push_back(char_array_3[i]);

                        i = 0;
                }
        }

        // Process remaining characters
        if (i > 0) {
                for (size_t j = i; j < 4; j++)
                        char_array_4[j] = 0;

                for (size_t j = 0; j < 4; j++)
                        char_array_4[j] = is_base64(char_array_4[j])
                                              ? base64_index(char_array_4[j])
                                              : 0;

                char_array_3[0] =
                    (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) +
                                  ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] =
                    ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

                if (i > 1) {
                        size_t remaining_bytes = i - 1;
                        if (decoded_data.size() + remaining_bytes >
                            MAX_DECODED_SIZE) {
                                throw std::runtime_error(
                                    "Decoded data exceeds size limit during "
                                    "final processing");
                        }

                        for (size_t j = 0; j < i - 1; j++)
                                decoded_data.push_back(char_array_3[j]);
                }
        }

        return decoded_data;
}

std::string toLower(const std::string &str)
{
        std::string lowerStr = str;
        std::transform(
            lowerStr.begin(),
            lowerStr.end(),
            lowerStr.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return lowerStr;
}

void trimcpp(std::string &str)
{
        // Remove leading spaces
        str.erase(str.begin(),
                  std::find_if(str.begin(), str.end(),
                               [](unsigned char ch) { return !std::isspace(ch); }));

        // Remove trailing spaces
        str.erase(std::find_if(str.rbegin(), str.rend(),
                               [](unsigned char ch) { return !std::isspace(ch); })
                      .base(),
                  str.end());
}

double parseDecibelValue(const TagLib::String &dbString)
{
        double val = 0.0;
        try {
                std::string valStr = dbString.to8Bit(true);
                std::string filtered;
                for (char c : valStr) {
                        if (std::isdigit((unsigned char)c) ||
                            c == '.' || c == '-' || c == '+' ||
                            c == 'e' || c == 'E') {
                                filtered.push_back(c);
                        }
                }
                val = std::stod(filtered);
        } catch (...) {
        }
        return val;
}

static bool detectLrcFormat(const TagLib::StringList &lines)
{
        for (const auto &line : lines) {
                std::string text = line.toCString(true);
                if (text.size() > 3 && text[0] == '[' && std::isdigit((unsigned char)text[1]))
                        return true;
        }
        return false;
}

static bool parseLyricsFromTagLines(const TagLib::StringList &lines, char **lyricsOut)
{
        size_t capacity = lines.size() > 64 ? lines.size() : 64;

        char *lyrics = (char *)calloc(MAX_LYRIC_SIZE, sizeof(char));
        auto lyricsLocation = lyrics;

        for (size_t i = 0; i < lines.size(); ++i) {
                std::string text = lines[i].toCString(true);

                // Trim leading/trailing whitespace
                size_t start = 0;
                while (start < text.size() && std::isspace((unsigned char)text[start]))
                        start++;
                size_t end = text.size();
                while (end > start && std::isspace((unsigned char)text[end - 1]))
                        end--;
                text = text.substr(start, end - start);

                if (text.empty())
                        continue;

                snprintf(lyricsLocation, text.length() + 2, "%s\n", text.c_str());
                lyricsLocation += text.length() + 1;
        }

        *lyricsOut = lyrics;

        return (lyricsLocation - lyrics) > 0;
}

bool loadLyricsFromSYLTTag(TagLib::ID3v2::Tag *id3v2Tag, char **lyricsOut)
{
    if (!id3v2Tag || !lyricsOut) {
        printf("no tag/string\n");
        return false;
    }

    auto frames = id3v2Tag->frameList("SYLT");
    if (frames.isEmpty()) {
        *lyricsOut = NULL;
        printf("no frame\n");
        return false;
    }

    // Count total lines across all SYLT frames
    size_t totalLines = 0;
    for (auto frame : frames) {
            auto sylt = dynamic_cast<TagLib::ID3v2::SynchronizedLyricsFrame *>(frame);
            if (!sylt)
                    continue;
            totalLines += sylt->synchedText().size();
    }

    if (totalLines == 0) {
        printf("no lines\n");
        return false;
    }

    char* lyrics = (char *)calloc(MAX_LYRIC_SIZE, sizeof(char));
    if (lyrics == nullptr)
        return false;

    char* currentLyricPos = lyrics;

    int count = 0;
    for (auto frame : frames) {
        auto sylt = dynamic_cast<TagLib::ID3v2::SynchronizedLyricsFrame *>(frame);
        if (!sylt) {
            printf("SYLT nooo\n");
            continue;
        }

        for (auto lyricLine : sylt->synchedText()) {
            if (lyricLine.text.isEmpty()) continue;

            std::ostringstream timestampedLyrics;
            int mins, seconds, centiseconds;

            mins = lyricLine.time / 60000;
            seconds = lyricLine.time / 1000 - 60 * mins;
            centiseconds = lyricLine.time/10 - 6000 * mins - 100 * seconds;

            timestampedLyrics << std::setfill('0')
                              << "["
                              << std::setw(2) << mins << ":"
                              << std::setw(2) << seconds << "."
                              << std::setw(2) << centiseconds
                              << "] " << lyricLine.text << "\n";

            int size = strlen(timestampedLyrics.str().c_str());
            snprintf(currentLyricPos, size + 1, "%s", timestampedLyrics.str().c_str());
            currentLyricPos += size;
        }
    }

    *lyricsOut = (char*)lyrics;

    if (*lyricsOut == NULL)
        return false;
    else
        return true;
}

char* loadLyricsFromUSLTTag(TagLib::ID3v2::Tag *id3v2Tag,
                                  char **lyricsOut)
{
        if (!id3v2Tag || !lyricsOut)
                return NULL;

        auto frames = id3v2Tag->frameList("USLT");
        if (frames.isEmpty())
                return NULL;

        TagLib::ID3v2::Frame *best = nullptr;
        size_t bestLen = 0;

        // Pick the largest USLT payload
        for (auto frame : frames) {
                TagLib::String text = frame->toString();
                if (text.isEmpty())
                        continue;

                size_t len = text.size();
                if (len > bestLen) {
                        best = frame;
                        bestLen = len;
                }
        }

        if (!best)
                return NULL;

        // Convert once to UTF-8
        std::string utf8 = best->toString().to8Bit(true);
        if (utf8.empty())
                return NULL;

        // Normalize line endings
        std::string normalized;
        normalized.reserve(utf8.size());

        for (size_t i = 0; i < utf8.size(); ++i) {
                if (utf8[i] == '\r') {
                        if (i + 1 < utf8.size() && utf8[i + 1] == '\n')
                                ++i;
                        normalized.push_back('\n');
                } else {
                        normalized.push_back(utf8[i]);
                }
        }

        // Split into TagLib::StringList
        TagLib::StringList lines;
        size_t start = 0;

        while (start < normalized.size()) {
                size_t end = normalized.find('\n', start);
                if (end == std::string::npos)
                        end = normalized.size();

                lines.append(TagLib::String(
                    normalized.substr(start, end - start),
                    TagLib::String::UTF8));

                start = end + 1;
        }

        // Trim trailing blanks
        while (!lines.isEmpty() && lines.back().stripWhiteSpace().isEmpty())
                lines.erase(--lines.end());

        if (lines.isEmpty())
                return NULL;

        bool looksLikeLrc = detectLrcFormat(lines);
        bool ok = parseLyricsFromTagLines(lines, lyricsOut);
        
        return *lyricsOut;
}

// Function to read a 32-bit unsigned integer from buffer in big-endian
// format
unsigned int read_uint32_be(const unsigned char *buffer,
                            size_t buffer_size, size_t offset)
{
        if (buffer == nullptr || offset + 4 > buffer_size) {
                // Handle error - throw exception, return error code,
                // etc.
                throw std::runtime_error(
                    "Buffer overflow in read_uint32_be");
        }

        return (static_cast<unsigned int>(buffer[offset]) << 24) |
               (static_cast<unsigned int>(buffer[offset + 1]) << 16) |
               (static_cast<unsigned int>(buffer[offset + 2]) << 8) |
               static_cast<unsigned int>(buffer[offset + 3]);
}

bool looksLikeJpeg(const std::vector<unsigned char> &data)
{
        return data.size() > 4 && data[0] == 0xFF && data[1] == 0xD8 &&
               data[data.size() - 2] == 0xFF &&
               data[data.size() - 1] == 0xD9;
}
bool looksLikePng(const std::vector<unsigned char> &data)
{
        static const unsigned char pngHeader[8] = {
            0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
        if (data.size() < 12)
                return false;
        if (memcmp(data.data(), pngHeader, 8) != 0)
                return false;
        // Look for IEND within last 16 bytes
        for (size_t i = data.size() >= 16 ? data.size() - 16 : 0;
             i + 3 < data.size(); ++i) {
                if (memcmp(&data[i], "IEND", 4) == 0)
                        return true;
        }
        return false;
}
bool looksLikeWebp(const std::vector<unsigned char> &data)
{
        return data.size() > 12 && memcmp(&data[0], "RIFF", 4) == 0 &&
               memcmp(&data[8], "WEBP", 4) == 0;
}

bool extractCoverArtFromMp3(const std::string &inputFile,
                            const std::string &coverFilePath)
{
        TagLib::MPEG::File file(inputFile.c_str());
        if (!file.isValid()) {
                return false;
        }

        const TagLib::ID3v2::Tag *id3v2tag = file.ID3v2Tag();
        if (id3v2tag) {
                // Collect all attached picture frames
                TagLib::ID3v2::FrameList frames;
                frames.append(id3v2tag->frameListMap()["APIC"]);
                frames.append(id3v2tag->frameListMap()["PIC"]);

                if (!frames.isEmpty()) {
                        for (auto it = frames.begin();
                             it != frames.end(); ++it) {
                                const TagLib::ID3v2::
                                    AttachedPictureFrame *picFrame =
                                        dynamic_cast<
                                            TagLib::ID3v2::
                                                AttachedPictureFrame *>(
                                            *it);
                                if (picFrame) {
                                        // Access picture data and MIME
                                        // type
                                        TagLib::ByteVector pictureData =
                                            picFrame->picture();
                                        TagLib::String mimeType =
                                            picFrame->mimeType();

                                        // Construct the output file
                                        // path
                                        std::string outputFilePath =
                                            coverFilePath;

                                        // Write the image data to a
                                        // file
                                        FILE *outFile = fopen(
                                            outputFilePath.c_str(),
                                            "wb");
                                        if (outFile) {
                                                fwrite(
                                                    pictureData.data(),
                                                    1,
                                                    pictureData.size(),
                                                    outFile);
                                                fclose(outFile);

                                                return true;
                                        } else {
                                                return false; // Failed
                                                              // to open
                                                              // output
                                                              // file
                                        }

                                        // Break if only the first image
                                        // is needed
                                        break;
                                }
                        }
                } else {
                        return false; // No picture frames found
                }
        } else {
                return false; // No ID3v2 tag found
        }

        return true; // Success
}
