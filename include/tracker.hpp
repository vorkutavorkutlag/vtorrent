#include <array>
#include <boost/asio.hpp>
#include <constants.hpp>
#include <memory>
#include <print>

namespace bio = boost::asio;

namespace vtorrent::tracker {

struct Tracker_CB {
  uint64_t uploaded_{};
  uint64_t downloaded_{};
  uint64_t left_{};
  uint8_t event_{};

  // and a set of peers
};

class Session {
protected:
  explicit Session(std::string &&url) : url_(std::move(url)) {}
  std::string url_;
  uint16_t port_{};
  uint32_t interval_{};

  static inline bio::io_context tracker_context_{};

public:
  virtual ~Session() = default;
  Session(const Session &) = delete;
  Session &operator=(const Session &) = delete;

  void debug();
};

class SessionHTTP : public Session {
public:
  explicit SessionHTTP(std::string &&url);
};

class SessionUDP : public Session {
public:
  explicit SessionUDP(std::string &&url);

private:
  bio::ip::udp::socket socket_;
};

std::unique_ptr<Session> make_session(std::string &&url);

} // namespace vtorrent::tracker
