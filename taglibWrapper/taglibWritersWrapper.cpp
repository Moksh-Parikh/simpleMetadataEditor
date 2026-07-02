#include <taglib/apefooter.h>
#include <taglib/apeitem.h>
#include <taglib/apetag.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/fileref.h>
#include <taglib/mpegfile.h>
#include <taglib/textidentificationframe.h>
#include <taglib/tpropertymap.h>
#include <taglib/id3v2tag.h>
#include <taglib/id3v2frame.h>
#include <taglib/unsynchronizedlyricsframe.h>
#include <taglib/synchronizedlyricsframe.h>

#include <taglib/tag.h>
#include <taglib/tstring.h>

#include "../headers/tagLibWrapper.h"

void writeUSLTTag(TagLib::FileRef file, char* lyrics) {
    TagLib::ID3v2::Tag* songTag = (dynamic_cast<TagLib::MPEG::File *>(file.file()))->ID3v2Tag();
    if (songTag == nullptr) {
        return;
    }

    TagLib::ID3v2::FrameList fileFrameList = songTag->frameList("USLT");

    TagLib::ID3v2::UnsynchronizedLyricsFrame* usltFrame = 
        dynamic_cast<TagLib::ID3v2::UnsynchronizedLyricsFrame*>(fileFrameList[0]);

    if (usltFrame) {
        usltFrame->setText(lyrics);
        usltFrame->setDescription("Lyrics");
    }
    else {
        TagLib::ID3v2::UnsynchronizedLyricsFrame* newUSLTFrame = 
            new TagLib::ID3v2::UnsynchronizedLyricsFrame();
        
        newUSLTFrame->setText(lyrics);
        newUSLTFrame->setDescription("Lyrics");
        songTag->addFrame(newUSLTFrame);
    }
}

//!TODO complete this function
void writeSYLTTag(TagLib::FileRef file, char* lyrics) {
    if (lyrics == NULL) return;
    TagLib::ID3v2::Tag* songTag = (dynamic_cast<TagLib::MPEG::File *>(file.file()))->ID3v2Tag();
    if (songTag == nullptr) {
        return;
    }

    TagLib::ID3v2::FrameList fileFrameList = songTag->frameList("SYLT");
    
    TagLib::ID3v2::SynchronizedLyricsFrame* syltTag = 
        dynamic_cast<TagLib::ID3v2::SynchronizedLyricsFrame*>(fileFrameList[0]);


    if (syltTag) {
        // for (auto lyricLine : std::string(lyrics).split('\n')) {
            
        // }
    }
    else {
        TagLib::ID3v2::SynchronizedLyricsFrame* newSYLTFrame = 
            new TagLib::ID3v2::SynchronizedLyricsFrame();

        newSYLTFrame->setText(lyrics);
        newSYLTFrame->setDescription("Lyrics");
        songTag->addFrame(newSYLTFrame);
    }

}

void writeLyrics(TagLib::FileRef file, char* lyrics, enum lyricType whichLyrics) {
    switch (whichLyrics) {
        case USLT:
            writeUSLTTag(file, lyrics);
            break;
        case SYLT:
            break;
    }
};

void writeIntData(TagLib::Tag* tag, enum writeMode writingMode, int data) {
    switch (writingMode) {
        case WRITE_YEAR:
            tag->setYear(data);
            break;
        case WRITE_TRACK:
            tag->setTrack(data);
            break;
    }
}

void writeStringData(TagLib::Tag* tag, enum writeMode writingMode, char* data) {
    switch (writingMode) {
        case WRITE_ARTIST:
            tag->setArtist(data);
            break;
        case WRITE_ALBUM:
            tag->setAlbum(data);
            break;
        case WRITE_GENRE:
            tag->setGenre(data);
            break;
        case WRITE_TITLE:
            tag->setTitle(data);
            break;
    }
}

extern "C" {
int writeToFile(char* filePath, enum writeMode writingMode, union writerData writeData) {
    TagLib::FileRef file(filePath);
    switch (writingMode) {
        case WRITE_YEAR:
        case WRITE_TRACK:
            writeIntData(file.tag(), writingMode, writeData.intData);
            break;
        case WRITE_LYRICS:
            writeLyrics(file, writeData.textData, USLT);
            break;
        default:
            writeStringData(file.tag(), writingMode, writeData.textData);
            break;
    }
    file.save();

    return 0;
}
}
