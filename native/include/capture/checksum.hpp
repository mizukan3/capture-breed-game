#pragma once
#include <string>
#include <string_view>
namespace capture {
// FNV-1a over UTF-16 code units, exactly as JS charCodeAt (not UTF-8 bytes).
std::string save_checksum(std::string_view utf8);
}
