#include <taglib/apefooter.h>
#include <taglib/apeitem.h>
#include <taglib/apetag.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/fileref.h>
#include <taglib/mpegfile.h>
#include <taglib/tbytevector.h>
#include <taglib/textidentificationframe.h>
#include <taglib/tlist.h>
#include <taglib/tpropertymap.h>
#include <taglib/id3v2tag.h>
#include <taglib/id3v2frame.h>
#include <taglib/unsynchronizedlyricsframe.h>
#include <taglib/synchronizedlyricsframe.h>

#include <taglib/tag.h>
#include <taglib/tstring.h>

#include "../headers/tagLibWrapper.h"
#include "tagLibUtils.h"

void writeUSLTTag(TagLib::FileRef file, char* lyrics) {
    if(file.isNull())
        return;

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

void writeSYLTTag(TagLib::FileRef file, char *lyrics)
{
    if(file.isNull())
        return;

    auto *mpeg = dynamic_cast<TagLib::MPEG::File *>(file.file());
    if(!mpeg)
        return;

    TagLib::ID3v2::Tag *tag = mpeg->ID3v2Tag(true);
    if(!tag)
        return;

    // Remove any existing SYLT frames.
    TagLib::ID3v2::FrameList frames = tag->frameListMap()["SYLT"];
    for(auto *frame : frames)
        tag->removeFrame(frame, true);

    auto *sylt = new TagLib::ID3v2::SynchronizedLyricsFrame;
    sylt->setTextEncoding(TagLib::String::UTF8);
    sylt->setLanguage("eng");
    sylt->setTimestampFormat(
        TagLib::ID3v2::SynchronizedLyricsFrame::AbsoluteMilliseconds);
    sylt->setType(
        TagLib::ID3v2::SynchronizedLyricsFrame::Lyrics);

    std::istringstream stream(lyrics);
    std::string line;

    TagLib::ID3v2::SynchronizedLyricsFrame::SynchedTextList lyricsList;

    while(std::getline(stream, line))
    {
        if(line.size() < 11 || line[0] != '[')
            continue;

        unsigned minutes, seconds, centiseconds;

        if(std::sscanf(line.c_str(),
                       "[%u:%u.%u]",
                       &minutes,
                       &seconds,
                       &centiseconds) != 3)
            continue;

        uint32_t timestamp =
            (minutes * 60 + seconds) * 1000 +
            centiseconds * 10;

        std::size_t closing = line.find(']');
        if(closing == std::string::npos)
            continue;

        if (std::isspace(closing + 1)) closing += 1;
        std::string text = line.substr(closing + 1);
        lyricsList.append(
            TagLib::ID3v2::SynchronizedLyricsFrame::SynchedText(
                timestamp,
                TagLib::String(text, TagLib::String::UTF8)
            )
        );
    }
    sylt->setSynchedText(lyricsList);

    tag->addFrame(sylt);
}

void writeAlbumArt(TagLib::FileRef file, char* art_path) {
    FILE* art_file = fopen(art_path, "rb");
    fseek(art_file, 0L, SEEK_END);
    size_t sz = ftell(art_file);
    fseek(art_file, 0L, SEEK_SET);
    char* imageData = (char*)calloc(sz, sizeof(char));

    fread(imageData, sizeof(char), sz, art_file);
    fclose(art_file);
    TagLib::ByteVector data = TagLib::ByteVector(imageData, sz);
    const char* mimeType = getPictureMimeType(imageData, sz);
    free(imageData);

    file.setComplexProperties("PICTURE", {
      {
        {"data", data},
        {"pictureType", "Front Cover"},
        {"mimeType", mimeType}
      }
    });
}

void writeLyrics(TagLib::FileRef file, char* lyrics, enum lyricType whichLyrics) {
    switch (whichLyrics) {
        case USLT:
            writeUSLTTag(file, lyrics);
            break;
        case SYLT:
            writeSYLTTag(file, lyrics);
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
        default:
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
        default:
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
        case WRITE_USLT:
            writeLyrics(file, writeData.textData, USLT);
            break;
        case WRITE_SYLT:
            writeLyrics(file, writeData.textData, SYLT);
            break;
        case WRITE_PICTURE:
            writeAlbumArt(file, writeData.textData);
            break;
        default:
            writeStringData(file.tag(), writingMode, writeData.textData);
            break;
    }
    file.save();

    return 0;
}
}
