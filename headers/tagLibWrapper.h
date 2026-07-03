/**
 * @file taglibwrapper.h
 * @brief C++ wrapper around TagLib for metadata extraction.
 *
 * Provides a simplified interface for reading song metadata such as
 * title, artist, album and embedded artwork using the TagLib library.
 * Exposes a C-compatible API for integration with the main C codebase.
 */
#ifndef TAGLIB_WRAPPER_H
#define TAGLIB_WRAPPER_H

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define METADATA_MAX_LENGTH 256
typedef struct
{
    char   title[METADATA_MAX_LENGTH];
    char   artist[METADATA_MAX_LENGTH];
    char   album_artist[METADATA_MAX_LENGTH];
    char   album[METADATA_MAX_LENGTH];
    char   date[METADATA_MAX_LENGTH];
    char   genre[METADATA_MAX_LENGTH];
    int    trackNumber;
    int    discNumber;
    double replaygainTrack;
    double replaygainAlbum;
} TagSettings;

enum writeMode {
    WRITE_ALBUM,
    WRITE_ARTIST,
    WRITE_YEAR,
    WRITE_TITLE,
    WRITE_TRACK,
    WRITE_GENRE,
    WRITE_USLT,
    WRITE_SYLT,
    WRITE_PICTURE
};

enum lyricType {
    USLT,
    SYLT
};

union writerData {
    int intData;
    char* textData;
};

/*
 * @brief Extracts metadata tags, duration, and cover art from an audio file.
 *
 * This function parses an audio file to extract its metadata (e.g., title,
 * artist, album, year), audio duration. It supports
 * MP3 only. The function also retrieves replay gain information
 * and synchronized or unsynchronized lyrics
 *
 * The extracted metadata is stored in the provided `tag_settings` structure,
 * the duration is stored in the `duration` pointer, and any lyrics are stored
 * in the `lyrics` pointer.
 *
 * The function performs the following tasks:
 * - Extracts title, artist, album, and year.
 * - Retrieves replay gain information (track and album gains).
 * - Loads lyrics (if available) from SYLT or USLT.
 * - Extracts the audio file's duration.
 *
 * @param input_file Path to the audio file from which to extract metadata.
 * @param tag_settings A pointer to a TagSettings structure to store extracted
 *                     metadata (title, artist, album, replay gain, etc.).
 * @param duration A pointer to store the extracted audio duration in seconds.
 * @param cover_file_path The path where the extracted cover art should be saved.
 * @param lyrics A pointer to a Lyrics structure that will be populated with
 *               any lyrics found in the audio file.
 *
 * @return 0 if successful,
 *         -1 for a file that couldn't be opened
 *         -2 if the file is missing a tag,
 *              or audioProperties (duration)
 *         -3 for a NULL FileRef
 *
 * @note This function relies on the TagLib library to parse audio files and
 *       extract metadata. It also supports cover art extraction
 *
 * @warning If the input file is invalid, the function may not correctly
 *          initialize the tag settings, and fallback behavior will be applied.
 */
int extractTags(const char *input_file, TagSettings *tag_settings,
                double *duration, char** lyrics);

/*
 * @brief Extracts disc and track number from tags
 *
 * @param filepath full file path
 * @param track A pointer to the variable where the extracted track number will be stored
 * @param disc A pointer to the variable where the extracted disc number will be stored
 */
void getTrackInfo(const char *filepath, uint32_t* track, uint32_t* disc);

int pullCoverArt(const char* input_file, const char* coverFilePath);

int writeToFile(char* filePath, enum writeMode writingMode, union writerData writeData);

bool c_detectLrcFormat(char* lyrics);
const char* getPictureMimeType(char* fileData, size_t size);

#ifdef __cplusplus
}
#endif

#endif // TAGLIB_WRAPPER_H
