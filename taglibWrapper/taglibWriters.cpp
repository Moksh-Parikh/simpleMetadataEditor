#include <taglib/apefooter.h>
#include <taglib/apeitem.h>
#include <taglib/apetag.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/fileref.h>
#include <taglib/mpegfile.h>
#include <taglib/synchronizedlyricsframe.h>
#include <taglib/textidentificationframe.h>

#include <taglib/tag.h>

#include "../headers/tagLibWrapper.h"

void writeIntData(TagLib::FileRef file, int writeMode, int data) {
    switch (writeMode) {
        case WRITE_YEAR:
            file.tag()->setYear(data);
            break;
        case WRITE_TRACK:
            file.tag()->setTrack(data);
            break;
    }
    file.save();
}

void writeStringData(TagLib::FileRef file, int writeMode, char* data) {
    switch (writeMode) {
        case WRITE_ARTIST:
            file.tag()->setArtist(data);
            break;
        case WRITE_ALBUM:
            file.tag()->setAlbum(data);
            break;
        case WRITE_GENRE:
            file.tag()->setGenre(data);
            break;
        case WRITE_TITLE:
            file.tag()->setTitle(data);
            break;
    }
    file.save();
}

extern "C" {
int writeToFile(char* filePath, int writeMode, union writerData writeData) {
    TagLib::FileRef file(filePath);
    switch (writeMode) {
        case WRITE_YEAR:
        case WRITE_TRACK:
            writeIntData(file, writeMode, writeData.intData);
            break;
        default:
            writeStringData(file, writeMode, writeData.textData);
            break;
    }

    return 0;
}
}
