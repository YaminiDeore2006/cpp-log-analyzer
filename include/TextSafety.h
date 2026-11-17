#ifndef TEXT_SAFETY_H
#define TEXT_SAFETY_H

#include <string>

inline std::string escapeControlCharacters(const std::string& value) {
    constexpr char hexDigits[] = "0123456789ABCDEF";
    std::string escaped;
    escaped.reserve(value.size());
    for (unsigned char character : value) {
        if (character < 0x20 || character == 0x7F) {
            escaped += "\\x";
            escaped += hexDigits[character >> 4];
            escaped += hexDigits[character & 0x0F];
        } else {
            escaped += static_cast<char>(character);
        }
    }
    return escaped;
}

#endif
