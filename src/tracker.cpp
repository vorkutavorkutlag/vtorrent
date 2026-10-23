#include <boost/endian/conversion.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/url_view.hpp>
#include <iostream>
#include <tracker.hpp>

namespace bio = boost::asio;

namespace vtorrent::tracker {

std::unique_ptr<Session> make_session(std::string &&url,
                                      std::shared_ptr<Tracker_CB> cb) {
  try {
    if (url.starts_with(globals::UDP_url_prefix))
      return std::make_unique<SessionUDP>(std::move(url), cb);
    if (url.starts_with(globals::HTTP_url_prefix))
      return std::make_unique<SessionHTTP>(std::move(url), cb);
  } catch (...) {
    return nullptr;
  }

  return nullptr;
}

SessionUDP::SessionUDP(std::string &&url, std::shared_ptr<Tracker_CB> cb)
    : Session::Session(std::move(url), cb), socket_(tracker_context_) {
  auto parse_result = boost::urls::parse_uri(url_);
  if (!parse_result)
    throw(std::runtime_error("Bad tracker url"));

  bio::ip::udp::resolver resolver(tracker_context_);
  socket_.open(bio::ip::udp::v4());
  boost::system::error_code ec;

  auto endpoint = resolver
                      .resolve(bio::ip::udp::v4(), parse_result->host(),
                               parse_result->port())
                      .begin()
                      ->endpoint();

  if (!parse_result->port().empty()) {
    socket_.connect(endpoint, ec);

    // url port is open
    if (!ec) {
      port_ = endpoint.port();
      return;
    }
  }

  // bep specifies interval [6881, 6889]
  for (endpoint.port(6881); endpoint.port() <= 6889;
       endpoint.port(endpoint.port() + 1)) {
    socket_.connect(endpoint, ec);
    if (!ec) {
      port_ = endpoint.port(); // could be removed later?
      return;
    }
  }

  throw(std::runtime_error("Bad tracker url"));
}

uint64_t SessionUDP::connect() {
  // --- SEND ---
  uint32_t transaction_id = globals::rand32();

  std::array<uint8_t, 16> buffer{};
  boost::endian::endian_store<uint64_t, 8, boost::endian::order::big>(
      buffer.data(), globals::UDP_protocol_id);

  boost::endian::endian_store<uint64_t, 4, boost::endian::order::big>(
      buffer.data() + 8, globals::UDP_action_connect);

  boost::endian::endian_store<uint64_t, 4, boost::endian::order::big>(
      buffer.data() + 12, transaction_id);

  socket_.send(boost::asio::buffer(buffer));

  // --- RECEIVE ---
}

void SessionUDP::announce(uint64_t conn_id) {}

void SessionUDP::run() {}

SessionHTTP::SessionHTTP(std::string &&url, std::shared_ptr<Tracker_CB> cb)
    : Session::Session(std::move(url), cb) {}

} // namespace vtorrent::tracker