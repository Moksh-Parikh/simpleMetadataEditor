#pragma once
#include <vector>

bool looksLikeJpeg(const std::vector<unsigned char> &data);
bool looksLikePng(const std::vector<unsigned char> &data);
bool looksLikeWebp(const std::vector<unsigned char> &data);
const char* getPictureMimeType(char* fileData, size_t size);
