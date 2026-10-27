#include <bencode.hpp>
#include <print>
#include <tracker.hpp>

int main(void) {
  vtorrent::Bencode be("movie.torrent");

  // be.json_dump();
  // for (const auto &url : be.get_tracker_urls()) {
  //   std::println("{}", std::get<std::string>(url));
  // }

  std::string url("udp://tracker.opentrackr.org:1337/announce");

  // temporary peer_id for testing
  auto peer_id = be.infohash;
  std::transform(peer_id.begin(), peer_id.end(), peer_id.begin(),
                 [](unsigned char c) { return c << 1; });

  auto tracker_cb =
      std::make_shared<vtorrent::tracker::Tracker_CB>(be.infohash, peer_id);

  auto sesh = vtorrent::tracker::make_session(std::move(url), tracker_cb);
  sesh->run();

  return 0;
}