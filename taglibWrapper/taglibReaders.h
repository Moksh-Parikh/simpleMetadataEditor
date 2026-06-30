#pragma once

#include <taglib/tag.h>
#include <taglib/id3v2frame.h>
#include <taglib/id3v2tag.h>
#define MAX_LYRIC_SIZE 4096

// Base64 character map for decoding
static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz"
                                        "0123456789+/";

#define MAX_REASONABLE_SIZE = 100 * 1024 * 1024; // 100MB limit

std::vector<unsigned char> decodeBase64(const std::string &encoded_string);

std::string toLower(const std::string &str);

void trimcpp(std::string &str);

double parseDecibelValue(const TagLib::String &dbString);

static bool detectLrcFormat(const TagLib::StringList &lines);

static bool parseLyricsFromTagLines(const TagLib::StringList &lines,
                                    char **lyricsOut);

bool loadLyricsFromSYLTTag(TagLib::ID3v2::Tag *id3v2Tag,
                                  char **lyricsOut);

char *loadLyricsFromUSLTTag(TagLib::ID3v2::Tag *id3v2Tag,
                            char **lyricsOut);

unsigned int read_uint32_be(const unsigned char *buffer,
                            size_t buffer_size,
                            size_t offset);

bool looksLikeJpeg(const std::vector<unsigned char> &data);

bool looksLikePng(const std::vector<unsigned char> &data);

bool looksLikeWebp(const std::vector<unsigned char> &data);

bool extractCoverArtFromMp3(const std::string &inputFile,
                            const std::string &coverFilePath);
