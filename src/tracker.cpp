#include <boost/url/parse.hpp>
#include <boost/url/url_view.hpp>
#include <iostream>
#include <tracker.hpp>

namespace bio = boost::asio;

namespace vtorrent::tracker {
void Session::debug() {
  std::println("Connected to {} with port {}.", url_, port_);
}

std::unique_ptr<Session> make_session(std::string &&url) {
  try {
    if (url.starts_with(consts::UDP_url_prefix))
      return std::make_unique<SessionUDP>(std::move(url));
    if (url.starts_with(consts::HTTP_url_prefix))
      return std::make_unique<SessionHTTP>(std::move(url));
  } catch (...) {
    return nullptr;
  }

  return nullptr;
}

SessionUDP::SessionUDP(std::string &&url)
    : Session::Session(std::move(url)), socket_(tracker_context_) {
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
      port_ = endpoint.port(); // could be removed later
      return;
    }
  }

  throw(std::runtime_error("Bad tracker url"));
}

SessionHTTP::SessionHTTP(std::string &&url)
    : Session::Session(std::move(url)) {}

} // namespace vtorrent::tracker