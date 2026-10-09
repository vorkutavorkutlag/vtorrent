#include <bencode.hpp>
#include <print>

int main(void) {
    vtorrent::Bencode be("movie.torrent");
    // be.json_dump();

    for (auto i = 0uz; i < 20; i++) {
        printf("%02x ", be.infohash[i]);
    }

    return 0;
}