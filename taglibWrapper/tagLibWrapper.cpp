/**
 * @file taglibwrapper.cpp
 * @brief C++ wrapper around TagLib for metadata extraction.
 *
 * Provides a simplified interface for reading song metadata such as
 * title, artist, album and embedded artwork using the TagLib library.
 * Exposes a C-compatible API for integration with the main C codebase.
 */

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdio.h>
#include <string>

#include <taglib/fileref.h>
#include <taglib/mpegfile.h>
#include <taglib/apefooter.h>
#include <taglib/apeitem.h>
#include <taglib/apetag.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/synchronizedlyricsframe.h>
#include <taglib/tag.h>
#include <taglib/textidentificationframe.h>

#include "../headers/tagLibWrapper.h"

#include "taglibReaders.h"

extern "C" {

void turnFilePathIntoTitle(const char *filePath, char *title,
                           size_t titleMaxLength)
{
        std::string filePathStr(
            filePath); // Convert the C-style string to std::string

        size_t lastSlashPos = filePathStr.find_last_of(
            "/\\"); // Find the last '/' or '\\'
        size_t lastDotPos =
            filePathStr.find_last_of('.'); // Find the last '.'

        // Validate that both positions exist and the dot is after the
        // last slash
        if (lastSlashPos != std::string::npos &&
            lastDotPos != std::string::npos &&
            lastDotPos > lastSlashPos) {
                // Extract the substring between the last slash and the
                // last dot
                std::string extractedTitle = filePathStr.substr(
                    lastSlashPos + 1, lastDotPos - lastSlashPos - 1);

                // Trim any unwanted spaces
                trimcpp(extractedTitle);

                // Ensure title is not longer than titleMaxLength,
                // including the null terminator
                if (extractedTitle.length() >= titleMaxLength) {
                        extractedTitle = extractedTitle.substr(
                            0, titleMaxLength - 1);
                }

                // Copy the result into the output char* title, ensuring
                // no overflow
                strncpy(
                    title, extractedTitle.c_str(),
                    titleMaxLength -
                        1); // Copy up to titleMaxLength - 1 characters
                title[titleMaxLength - 1] =
                    '\0'; // Null-terminate the string
        } else {
                // If no valid title is found, ensure title is an empty
                // string
                title[0] = '\0';
        }
}

void getTrackInfo(const char *filepath, uint32_t* track, uint32_t* disc) {
    TagLib::FileRef file(filepath);
    if (file.isNull() || !file.file()) {
            fprintf(stderr,
                    "FileRef is null or file could not be opened: "
                    "'%s'\n",
                    filepath);

            return;
    }

    const TagLib::Tag *tag = file.tag();
    if (!tag) {
            fprintf(stderr, "Tag is null for file '%s'\n",
                    filepath);
            return;
    }

    uint32_t trackNumber = tag->track();

    if (track != NULL) *track = trackNumber;
    if (disc == NULL) return;

    auto mpeg = dynamic_cast<TagLib::MPEG::File *>(file.file());
    if (mpeg == NULL || !mpeg->isValid()) return;

    auto mpegTag = mpeg->ID3v2Tag();
    if (!mpegTag) return;

    TagLib::ID3v2::FrameList discNumber = mpegTag->frameListMap()["TPOS"];

    if (discNumber.isEmpty()) return;

    auto *frame = dynamic_cast<TagLib::ID3v2::TextIdentificationFrame *>(discNumber.front());
    if (!frame) return;

    TagLib::String str = frame->toString();
    std::string raw = str.to8Bit(true);
    char *delimiter;
    uint32_t disc_number = strtoul(raw.c_str(), &delimiter, 10);
    *disc = disc_number;

    return;
}

int pullCoverArt(const char* input_file, const char* coverFilePath) {
        // extract cover art
        std::string filename(input_file);
        std::string extension = toLower(filename.substr(filename.find_last_of('.') + 1));
        bool coverArtExtracted = false;

        if (extension == "mp3") {
                coverArtExtracted =
                    extractCoverArtFromMp3(input_file, coverFilePath);
        }

        if (coverArtExtracted) {
                return 0;
        } else {
                return -1;
        }
}

int extractTags(const char *input_file, TagSettings *tag_settings,
                double *duration, char** lyrics)
{
        memset(tag_settings, 0,
               sizeof(TagSettings)); // Initialize tag settings

        tag_settings->replaygainTrack = 0.0;
        tag_settings->replaygainAlbum = 0.0;

        // Use TagLib's FileRef for generic file parsing.
        TagLib::FileRef f(input_file);
        if (f.isNull() || !f.file()) {
                fprintf(stderr,
                        "FileRef is null or file could not be opened: "
                        "'%s'\n",
                        input_file);

                char title[4096];
                turnFilePathIntoTitle(input_file, title, 4096);
                strncpy(tag_settings->title, title,
                         sizeof(tag_settings->title) - 1);
                tag_settings->title[sizeof(tag_settings->title) - 1] =
                    '\0';

                return -1;
        }

        const TagLib::Tag *tag = f.tag();
        if (!tag) {
                fprintf(stderr, "Tag is null for file '%s'\n",
                        input_file);
                return -2;
        }
        
        strncpy(tag_settings->title, tag->title().toCString(true),
                 sizeof(tag_settings->title) - 1);
        tag_settings->title[sizeof(tag_settings->title) - 1] = '\0';

        // Copy the artist
        strncpy(tag_settings->artist,
                 tag->artist().toCString(true),
                 sizeof(tag_settings->artist) - 1);
        tag_settings->artist[sizeof(tag_settings->artist) - 1] =
            '\0';

        // Copy the album
        strncpy(tag_settings->album,
                 tag->album().toCString(true),
                 sizeof(tag_settings->album) - 1);
        tag_settings->album[sizeof(tag_settings->album) - 1] =
            '\0';
        
        strncpy(tag_settings->genre, tag->genre().toCString(true),
                 sizeof(tag_settings->genre) - 1);
        tag_settings->genre[sizeof(tag_settings->genre) - 1] = '\0';

        // Copy the year as date
        snprintf(tag_settings->date, sizeof(tag_settings->date),
                 "%d", (int)tag->year());

        if (tag_settings->date[0] == '0') {
                tag_settings->date[0] = '\0';
        }

        if (lyrics != nullptr && *lyrics == nullptr) {
            if (auto mpegFile = dynamic_cast<TagLib::MPEG::File *>(f.file())) {
                // 1) True synchronized lyrics (SYLT)
                int status = loadLyricsFromSYLTTag(mpegFile->ID3v2Tag(), lyrics);

                // 2) USLT fallback (may contain LRC timestamps)
                if (*lyrics == nullptr) {
                    printf("loading from USLT\n");
                    loadLyricsFromUSLTTag(mpegFile->ID3v2Tag(), lyrics);
                }
            }
        }

        // Extract audio properties for duration.
        if (f.audioProperties()) {
                *duration = f.audioProperties()->lengthInSeconds();
        } else {
                *duration = 0.0;
                fprintf(stderr,
                        "No audio properties found for file '%s'\n",
                        input_file);
                return -2;
        }

        // Extract replay gain information
        if (auto mp3File = dynamic_cast<TagLib::MPEG::File *>(f.file())) {

                TagLib::ID3v2::Tag *id3v2Tag = mp3File->ID3v2Tag();

                if (id3v2Tag) {

                        // Retrieve all TXXX frames
                        TagLib::ID3v2::FrameList frames =
                            id3v2Tag->frameList("TXXX");
                        for (TagLib::ID3v2::FrameList::Iterator it =
                                 frames.begin();
                            it != frames.end(); ++it) {
                                // Cast to the user-text (TXXX) frame
                                // class
                                TagLib::ID3v2::TextIdentificationFrame
                                    *txxx = dynamic_cast<
                                        TagLib::ID3v2::
                                            TextIdentificationFrame *>(
                                        *it);
                                if (!txxx)
                                        continue;

                                TagLib::StringList fields =
                                    txxx->fieldList();
                                if (fields.size() >= 2) {
                                        TagLib::String desc = fields[0];
                                        TagLib::String val = fields[1];

                                        if (desc.upper() ==
                                            "REPLAYGAIN_TRACK_GAIN") {
                                                tag_settings
                                                    ->replaygainTrack =
                                                    parseDecibelValue(
                                                        val);
                                        } else if (desc.upper() ==
                                                   "REPLAYGAIN_ALBUM_"
                                                   "GAIN") {
                                                tag_settings
                                                    ->replaygainAlbum =
                                                    parseDecibelValue(
                                                        val);
                                        }
                                }
                        }
                }

                TagLib::APE::Tag *apeTag = mp3File->APETag();

                if (apeTag) {
                        TagLib::APE::ItemListMap items =
                            apeTag->itemListMap();
                        for (auto it = items.begin(); it != items.end();
                             ++it) {
                                std::string key =
                                    it->first.upper().toCString();
                                TagLib::String value =
                                    it->second.toString();

                                if (key == "REPLAYGAIN_TRACK_GAIN") {
                                        tag_settings->replaygainTrack =
                                            parseDecibelValue(value);
                                } else if (key == "REPLAYGAIN_ALBUM_GAIN") {
                                        tag_settings->replaygainAlbum =
                                            parseDecibelValue(value);
                                }
                        }
                }
        }

    return 0;
}
}
