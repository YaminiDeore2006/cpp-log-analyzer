#include "Timestamp.h"

namespace {
bool isDigit(char character) noexcept {
    return character >= '0' && character <= '9';
}

int parseNumber(const std::string& timestamp, std::size_t start, std::size_t length) noexcept {
    int value = 0;
    for (std::size_t index = start; index < start + length; ++index) {
        value = value * 10 + (timestamp[index] - '0');
    }
    return value;
}

bool isLeapYear(int year) noexcept {
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}
}

bool isValidTimestamp(const std::string& timestamp) noexcept {
    if (timestamp.size() != 19 ||
        timestamp[4] != '-' || timestamp[7] != '-' || timestamp[10] != ' ' ||
        timestamp[13] != ':' || timestamp[16] != ':') {
        return false;
    }
    for (std::size_t index = 0; index < timestamp.size(); ++index) {
        if (index != 4 && index != 7 && index != 10 && index != 13 && index != 16 &&
            !isDigit(timestamp[index])) {
            return false;
        }
    }

    const int year = parseNumber(timestamp, 0, 4);
    const int month = parseNumber(timestamp, 5, 2);
    const int day = parseNumber(timestamp, 8, 2);
    const int hour = parseNumber(timestamp, 11, 2);
    const int minute = parseNumber(timestamp, 14, 2);
    const int second = parseNumber(timestamp, 17, 2);
    if (year == 0 || month < 1 || month > 12 || hour > 23 || minute > 59 || second > 59) {
        return false;
    }

    constexpr int daysPerMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int maxDay = daysPerMonth[month - 1];
    if (month == 2 && isLeapYear(year)) {
        ++maxDay;
    }
    return day >= 1 && day <= maxDay;
}
