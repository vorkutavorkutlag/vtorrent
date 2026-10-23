#include <array>
#include <boost/asio.hpp>
#include <globals.hpp>
#include <memory>
#include <mutex>
#include <print>
#include <set>

namespace bio = boost::asio;
typedef std::basic_string<unsigned char> ustring;

namespace vtorrent::tracker {

struct minimal_peer {
  // tmp?
  std::array<uint8_t, 4> ip_{};
  uint16_t port_{};
};

struct Tracker_CB {
  std::mutex mtx_{};

  ustring info_hash_{};
  uint64_t uploaded_{};
  uint64_t downloaded_{};
  uint64_t left_{};
  uint8_t event_{};

  std::set<minimal_peer> peers_{};
};

class Session {
protected:
  explicit Session(std::string &&url, std::shared_ptr<Tracker_CB> cb)
      : url_(std::move(url)), cb_(cb) {}
  std::shared_ptr<Tracker_CB> cb_;
  std::string url_;
  uint16_t port_{};
  uint32_t interval_{};

  static inline bio::io_context tracker_context_{};

public:
  virtual ~Session() = default;
  Session(const Session &) = delete;
  Session &operator=(const Session &) = delete;
};

class SessionHTTP : public Session {
public:
  explicit SessionHTTP(std::string &&url, std::shared_ptr<Tracker_CB> cb);
};

class SessionUDP : public Session {
public:
  explicit SessionUDP(std::string &&url, std::shared_ptr<Tracker_CB> cb);
  void run();

private:
  bio::ip::udp::socket socket_;

  uint64_t connect();
  void announce(uint64_t conn_id);
};

std::unique_ptr<Session> make_session(std::string &&url,
                                      std::shared_ptr<Tracker_CB> cb);

} // namespace vtorrent::tracker
