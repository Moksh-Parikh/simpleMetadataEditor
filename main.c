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
    extractTags(
        songPath,
        &tagSettings,
        &duration,
        &lyrics
    );
    pullCoverArt(songPath, "./picture");

    printf("%s from %s by %s\n", tagSettings.title, tagSettings.album, tagSettings.artist);
    printf("Lyrics:\n%s", lyrics);

    free(lyrics);

    return 0;
}
