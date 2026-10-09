#include <stdlib.h>

namespace vtorrent::consts {
constexpr const size_t infohash_length = 20uz;

constexpr const char dict_start = 'd';
constexpr const char list_start = 'l';
constexpr const char container_end = 'e';
constexpr const char integer_start = 'i';
constexpr const char delim = ':';

constexpr const char* key_info = "info";

}  // namespace vtorrent::consts