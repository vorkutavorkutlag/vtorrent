#include <bencode.hpp>
#include <globals.hpp>
#include <print>
#include <thread>
#include <tracker.hpp>

// tmp delete later
void debug_ip(uint32_t peer_ip, uint16_t peer_port) {
  std::println("Peer {}.{}.{}.{}:{}", peer_ip >> 24, (peer_ip >> 16) & 0xFF,
               (peer_ip >> 8) & 0xFF, peer_ip & 0xFF, peer_port);
}

int main(void) {
  vtorrent::Bencode be("movie.torrent");

  auto peer_id = vtorrent::globals::generate_peer_id();
  auto tracker_cb =
      std::make_shared<vtorrent::tracker::Tracker_CB>(be.infohash, peer_id);

  std::vector<std::jthread> tracker_threads{};

  for (auto &url : be.get_tracker_urls()) {
    auto &surl = std::get<std::string>(url);

    auto sesh = vtorrent::tracker::make_session(std::move(surl), tracker_cb);
    if (!sesh)
      continue;

    tracker_threads.emplace_back([s = std::move(sesh)]() { s->run(); });
  }

  std::set<vtorrent::tracker::basic_peer> seen_peers{};

  for (;;) {
    std::println("Iter");
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::set<vtorrent::tracker::basic_peer> new_peers{};
    {
      std::lock_guard<std::mutex> lock(tracker_cb->mtx_);
      std::set_difference(tracker_cb->peers_.begin(), tracker_cb->peers_.end(),
                          seen_peers.begin(), seen_peers.end(),
                          std::inserter(new_peers, new_peers.begin()));
    }

    seen_peers.insert(new_peers.begin(), new_peers.end());

    for (auto &peer : new_peers) {
      debug_ip(peer.ip_, peer.port_);
    }
  }

  return 0;
}