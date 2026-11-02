#include <bencode.hpp>
#include <globals.hpp>
#include <print>
#include <tracker.hpp>

int main(void) {
  vtorrent::Bencode be("movie.torrent");

  auto peer_id = vtorrent::globals::generate_peer_id();
  auto tracker_cb =
      std::make_shared<vtorrent::tracker::Tracker_CB>(be.infohash, peer_id);

  for (auto &url : be.get_tracker_urls()) {
    auto &surl = std::get<std::string>(url);
    if (!surl.starts_with(vtorrent::globals::HTTP_url_prefix))
      continue;

    auto sesh = vtorrent::tracker::make_session(std::move(surl), tracker_cb);
    sesh->run();
  }

  return 0;
}