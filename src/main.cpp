#include <bencode.hpp>
#include <print>

int main(void) {
  vtorrent::Bencode be("movie.torrent");
  be.json_dump();

  // for (auto i = 0uz; i < vtorrent::consts::infohash_length; ++i) {
  //   std::print("{:02x}", be.infohash[i]);
  // }

  return 0;
}