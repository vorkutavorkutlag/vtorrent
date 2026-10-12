#include <cstdint>
#include <limits>
#include <stdlib.h>

namespace vtorrent::consts {
constexpr const size_t infohash_length{20UZ};

constexpr const char dict_start{'d'};
constexpr const char list_start{'l'};
constexpr const char container_end{'e'};
constexpr const char integer_start{'i'};
constexpr const char delim{':'};

constexpr const char *key_info{"info"};
constexpr const char *key_announce{"announce"};
constexpr const char *key_announce_ls{"announce-list"};

constexpr const uint32_t default_ip{0UL};
constexpr const uint32_t default_numwant{
    std::numeric_limits<uint32_t>::max()}; // equiv to -1 in 2's complement

constexpr const uint64_t UDP_protocol_id{0x41727101980ULL};
constexpr const uint32_t UDP_action_connect{0UL};
constexpr const uint32_t UDP_action_announce{1UL};
constexpr const uint32_t UDP_event_none{0UL};
constexpr const uint32_t UDP_event_completed{1UL};
constexpr const uint32_t UDP_event_started{2UL};
constexpr const uint32_t UDP_event_stopped{3UL};

} // namespace vtorrent::consts