#include "headers/tagLibWrapper.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

int main(void) {
    TagSettings tagSettings;
    double duration;
    uint32_t track, disc;

    char* lyrics = NULL;
    char* songPath = "Linkin Park - Blackout.mp3";
    // char* songPath = "Bad Omens - Never Know (Unplugged).mp3";
    getTrackInfo(songPath, &track, &disc);

    union writerData writeData;
    writeData.intData = 697;
    
    writeToFile(songPath, WRITE_YEAR, writeData);
    writeData.textData = "Kevin Tran";
    writeToFile(songPath, WRITE_ALBUM, writeData);

    writeData.textData = "Gay metal";
    writeToFile(songPath, WRITE_GENRE, writeData);

    extractTags(
        songPath,
        &tagSettings,
        &duration,
        &lyrics
    );
    pullCoverArt(songPath, "./picture");

    printf("%s: %s from %s by %s\n", tagSettings.genre, tagSettings.title, tagSettings.album, tagSettings.artist);
    printf("Recorded on %s\n", tagSettings.date);
    printf("Lyrics:\n%s", lyrics);

    free(lyrics);

    return 0;
}
