#include <bencode.hpp>
#include <print>

int main(void) {
    vtorrent::Bencode be("movie.torrent");
    if (!be.dict) std::println("NIGHTMARE");

    be.json_dump();

    return 0;
}