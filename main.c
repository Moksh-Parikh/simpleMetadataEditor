#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "headers/argparse.h"
#include "headers/tagLibWrapper.h"

static const char *const usages[] = {
    "test_program [options] [arguments]",
    NULL,
};

int main(int argc, char **argv) {
    int writeDate = -1;
    bool quiet       = false;
    bool lyricsOrNot = false;
    bool genreOrNot  = false;
    bool albumOrNot  = false;
    bool artistOrNot = false;
    bool titleOrNot  = false;
    bool dateOrNot   = false;

    char* writeArtist = NULL;
    char* writeGenre  = NULL;
    char* lyricsPath  = NULL;
    char* writeAlbum  = NULL;
    char* writeTitle  = NULL;
    char* songPath    = NULL;
    char* artworkPath = NULL;
    
    struct argparse_option options[] = {
        OPT_HELP(), // "When only a filename is specifed, the output follows the following format"
        OPT_GROUP("Editing Options"),
        OPT_INTEGER('d', "write_date", &writeDate, "edit the date", NULL, 0, 0),
        OPT_STRING('f', "file_path", &songPath, "path to file to be edited", NULL, 0, 0),
        OPT_STRING('t', "write_title", &writeTitle, "edit the title", NULL, 0, 0),
        OPT_STRING('r', "write_album", &writeAlbum, "edit the album", NULL, 0, 0),
        OPT_STRING('g', "write_genre", &writeGenre, "edit the genre", NULL, 0, 0),
        OPT_STRING('a', "write_artist", &writeArtist, "edit the artist", NULL, 0, 0),
        OPT_STRING('l', "write_lyrics", &lyricsPath, "path to file containing lyrics to be embedded", NULL, 0, 0),
        OPT_GROUP("Querying Options"),
        OPT_BOOLEAN('q', "quiet", &quiet, "don't read current metadata", NULL, 0, 0),
        OPT_STRING('e', "extract_artwork", &artworkPath, "extract the cover artwork to the specifed path", NULL, 0, 0),
        OPT_BOOLEAN('1', "show_lyrics", &lyricsOrNot, "read the lyrics from SYLT or USLT tags", NULL, 0, 0),
        OPT_BOOLEAN('2', "show_genre", &genreOrNot, "read the genre", NULL, 0, 0),
        OPT_BOOLEAN('3', "show_artist", &artistOrNot, "read the artist", NULL, 0, 0),
        OPT_BOOLEAN('4', "show_title", &titleOrNot, "read the title", NULL, 0, 0),
        OPT_BOOLEAN('5', "show_album", &albumOrNot, "read the album", NULL, 0, 0),
        OPT_BOOLEAN('6', "show_date", &dateOrNot, "read the date", NULL, 0, 0),
        OPT_END()
    };

    struct argparse argparse;
    argparse_init(&argparse, options, usages, 0);
    argparse_describe(&argparse, "\nA simple metadata editing tool", NULL);

    argc = argparse_parse(&argparse, argc, (const char **)argv);

    union writerData writeData;

    if (writeTitle != NULL) {
        writeData.textData = writeTitle;
        writeToFile(songPath, WRITE_TITLE, writeData);
    }

    if (writeAlbum != NULL) {
        writeData.textData = writeAlbum;
        writeToFile(songPath, WRITE_ALBUM, writeData);
    }

    if (writeGenre != NULL) {
        writeData.textData = writeGenre;
        writeToFile(songPath, WRITE_GENRE, writeData);
    }
    
    if (writeArtist != NULL) {
        writeData.textData = writeArtist;
        writeToFile(songPath, WRITE_ARTIST, writeData);
    }

    if (writeDate >= 0) {
        writeData.intData = writeDate;
        writeToFile(songPath, WRITE_YEAR, writeData);
    }

    if (lyricsPath != NULL) {
        FILE* lyricFile = fopen(lyricsPath, "r");
        fseek(lyricFile, 0L, SEEK_END);
        size_t sz = ftell(lyricFile);
        fseek(lyricFile, 0L, SEEK_SET);
        char* lyricsToBeWritten = calloc(sz, sizeof(char));

        fread(lyricsToBeWritten, sizeof(char), sz, lyricFile);
        writeData.textData = lyricsToBeWritten;
        writeToFile(songPath, WRITE_LYRICS, writeData);
        free(lyricsToBeWritten);
    }
    

    TagSettings tagSettings;
    double duration;

    char* lyrics = NULL;

    extractTags(
        songPath,
        &tagSettings,
        &duration,
        &lyrics
    );

    if (artworkPath != NULL) pullCoverArt(songPath, artworkPath);

    if (!quiet || titleOrNot) printf("Title: %s\n", tagSettings.title);
    if (!quiet || artistOrNot) printf("Artist: %s\n", tagSettings.artist);
    if (!quiet || albumOrNot) printf("Album: %s\n", tagSettings.album);
    if (!quiet || genreOrNot) printf("Genre: %s\n", tagSettings.genre);
    if (!quiet || dateOrNot) printf("Date: %s\n", tagSettings.date);
    if (lyricsOrNot) printf("Lyrics:\n%s", lyrics);

    free(lyrics);

    return 0;
}
