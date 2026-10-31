#include <array>
#include <boost/asio.hpp>
#include <chrono>
#include <globals.hpp>
#include <memory>
#include <mutex>
#include <print>
#include <set>

namespace bio = boost::asio;
typedef std::basic_string<unsigned char> ustring;

namespace vtorrent::tracker {

struct basic_peer {
  uint32_t ip_;
  uint16_t port_;

  auto operator<=>(const basic_peer &other) const = default;
};

struct Tracker_CB {
  Tracker_CB(const auto &info_hash, const auto &peer_id)
      : info_hash_(info_hash), peer_id_(peer_id) {
    info_hash_s = std::string(reinterpret_cast<char *>(info_hash_.data()),
                              info_hash_.size());
    peer_id_s =
        std::string(reinterpret_cast<char *>(peer_id_.data()), peer_id_.size());
  }

  std::mutex mtx_{};

  std::array<unsigned char, 20> info_hash_{};
  std::array<unsigned char, 20> peer_id_{};
  std::string info_hash_s{};
  std::string peer_id_s{};
  uint64_t uploaded_{};
  uint64_t downloaded_{};
  uint64_t left_{};
  uint8_t event_{};

  std::set<basic_peer> peers_{};
};

class Session {
protected:
  explicit Session(std::string &&url, std::shared_ptr<Tracker_CB> cb)
      : url_(std::move(url)), cb_(cb) {}
  std::shared_ptr<Tracker_CB> cb_;
  std::string url_;
  uint32_t interval_{};
  uint16_t port_{};

  static inline bio::io_context tracker_context_{};

public:
  virtual ~Session() = default;
  Session(const Session &) = delete;
  Session &operator=(const Session &) = delete;

  virtual void run() = 0;
};

class SessionHTTP : public Session {
public:
  explicit SessionHTTP(std::string &&url, std::shared_ptr<Tracker_CB> cb);

  void run() override;
};

class SessionUDP : public Session {
public:
  explicit SessionUDP(std::string &&url, std::shared_ptr<Tracker_CB> cb);

  void run() override;

private:
  bio::ip::udp::socket socket_;

  std::chrono::steady_clock::time_point last_connect_{};

  std::optional<uint64_t> connect();
  bool announce(uint64_t conn_id);
};

std::unique_ptr<Session> make_session(std::string &&url,
                                      std::shared_ptr<Tracker_CB> cb);

} // namespace vtorrent::tracker
