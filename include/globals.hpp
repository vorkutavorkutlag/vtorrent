#include <cstdint>
#include <limits>
#include <random>
#include <stdlib.h>

#pragma once

namespace vtorrent {
class globals {
public:
  static inline constexpr size_t standard_length{20UZ};

  static inline constexpr char dict_start{'d'};
  static inline constexpr char list_start{'l'};
  static inline constexpr char container_end{'e'};
  static inline constexpr char integer_start{'i'};
  static inline constexpr char delim{':'};

  static inline constexpr std::string_view peer_id_prefix{"-VT0001-"};
  static inline constexpr std::string_view key_info{"info"};
  static inline constexpr std::string_view key_announce{"announce"};
  static inline constexpr std::string_view key_announce_ls{"announce-list"};

  static inline constexpr std::string_view UDP_url_prefix{"udp"};
  static inline constexpr std::string_view HTTP_url_prefix{"http"};

  static inline constexpr uint32_t default_ip{0UL};
  static inline constexpr uint32_t default_key{0UL};
  static inline constexpr uint32_t default_numwant{static_cast<uint32_t>(-1L)};

  static inline constexpr uint32_t UDP_conn_validity{1UL}; // in minutes
  static inline constexpr uint64_t UDP_protocol_id{0x41727101980ULL};
  static inline constexpr uint32_t UDP_action_connect{0UL};
  static inline constexpr uint32_t UDP_action_announce{1UL};
  static inline constexpr uint32_t UDP_event_none{0UL};
  static inline constexpr uint32_t UDP_event_completed{1UL};
  static inline constexpr uint32_t UDP_event_started{2UL};
  static inline constexpr uint32_t UDP_event_stopped{3UL};

  static inline uint32_t rand32() { return distrib32(gen); }

  static inline std::string generate_peer_id() {
    std::string peer_id;
    peer_id.reserve(globals::standard_length);
    peer_id.append(peer_id_prefix);

    for (size_t i = 0; i < standard_length - peer_id_prefix.length(); ++i)
      peer_id.push_back(static_cast<char>(globals::rand32() % 256));

    return peer_id;
  }

private:
  static inline std::random_device rd{};
  static inline std::mt19937 gen{rd()};
  static inline std::uniform_int_distribution<uint32_t> distrib32{
      0, std::numeric_limits<uint32_t>::max()};
};
} // namespace vtorrent