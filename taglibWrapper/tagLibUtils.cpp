#include <cstring>
#include <vector>

#include "tagLibUtils.h"

bool looksLikeJpeg(const std::vector<unsigned char> &data)
{
        return data.size() > 4 && data[0] == 0xFF && data[1] == 0xD8 &&
               data[data.size() - 2] == 0xFF &&
               data[data.size() - 1] == 0xD9;
}
bool looksLikePng(const std::vector<unsigned char> &data)
{
        static const unsigned char pngHeader[8] = {
            0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
        if (data.size() < 12)
                return false;
        if (memcmp(data.data(), pngHeader, 8) != 0)
                return false;
        // Look for IEND within last 16 bytes
        for (size_t i = data.size() >= 16 ? data.size() - 16 : 0;
             i + 3 < data.size(); ++i) {
                if (memcmp(&data[i], "IEND", 4) == 0)
                        return true;
        }
        return false;
}

bool looksLikeWebp(const std::vector<unsigned char> &data)
{
        return data.size() > 12 && memcmp(&data[0], "RIFF", 4) == 0 &&
               memcmp(&data[8], "WEBP", 4) == 0;
}

const char* getPictureMimeType(char* fileData, size_t size) {
    std::vector<unsigned char> imageData (fileData, fileData + size);

    if (looksLikeJpeg(imageData)) {
        return "image/jpeg";
    }
    
    if (looksLikePng(imageData)) {
        return "image/png";
    }
    
    if (looksLikeWebp(imageData)) {
        return "image/webp";
    }

    return NULL;
}
