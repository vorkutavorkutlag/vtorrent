#include <bencode.hpp>
#include <print>
#include <tracker.hpp>

int main(void) {
  // vtorrent::Bencode be("movie.torrent");
  // be.json_dump();

  // for (const auto &url : be.get_tracker_urls()) {
  //   std::println("{}", std::get<std::string>(url));
  // }

  std::string url("udp://tracker.opentrackr.org:1337/announce");
  auto sesh = vtorrent::tracker::make_session(std::move(url));

  if (!sesh)
    std::println("What?");

  sesh->debug();

  return 0;
}