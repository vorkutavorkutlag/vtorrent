#include <boost/endian/conversion.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/url_view.hpp>
#include <iostream>
#include <tracker.hpp>

namespace bio = boost::asio;

namespace vtorrent::tracker {

/*--------------------- SHARED ------------------*/

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

/*--------------------- UDP ------------------*/

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

std::optional<uint64_t> SessionUDP::connect() {
  // --- SEND ---
  uint32_t transaction_id = globals::rand32();
  std::array<uint8_t, 16> buffer{};

  boost::endian::endian_store<uint64_t, 8, boost::endian::order::big>(
      buffer.data(), globals::UDP_protocol_id);

  boost::endian::endian_store<uint64_t, 4, boost::endian::order::big>(
      buffer.data() + 8, globals::UDP_action_connect);

  boost::endian::endian_store<uint64_t, 4, boost::endian::order::big>(
      buffer.data() + 12, transaction_id);

  boost::system::error_code ec;

  socket_.send(boost::asio::buffer(buffer), 0, ec);
  if (ec)
    return std::nullopt;

  // --- RECEIVE ---

  socket_.receive(boost::asio::buffer(buffer), 0, ec);
  if (ec)
    return std::nullopt;

  uint32_t resp_protocol_id =
      boost::endian::endian_load<uint32_t, 4, boost::endian::order::big>(
          buffer.data());

  uint32_t resp_transaction_id =
      boost::endian::endian_load<uint32_t, 4, boost::endian::order::big>(
          buffer.data() + 4);

  uint64_t resp_connection_id =
      boost::endian::endian_load<uint64_t, 8, boost::endian::order::big>(
          buffer.data() + 8);

  if (resp_protocol_id != globals::UDP_action_connect)
    return std::nullopt;
  if (resp_transaction_id != transaction_id)
    return std::nullopt;

  last_connect_ = std::chrono::steady_clock::now();
  return resp_connection_id;
}

/*
  Offset  Size    Name    Value
  0       64-bit integer  connection_id
  8       32-bit integer  action          1 // announce
  12      32-bit integer  transaction_id
  16      20-byte string  info_hash
  36      20-byte string  peer_id
  56      64-bit integer  downloaded
  64      64-bit integer  left
  72      64-bit integer  uploaded
  80      32-bit integer  event
  84      32-bit integer  IP address      0 // default
  88      32-bit integer  key
  92      32-bit integer  num_want        -1 // default
  96      16-bit integer  port
  98
*/

void SessionUDP::announce(uint64_t conn_id) {
  using namespace boost;
  uint32_t transaction_id = globals::rand32();
  std::array<uint8_t, 98> sbuf{};

  // --- SEND ---

  size_t off{};
  auto put64 = [&](uint64_t v) {
    endian::endian_store<uint64_t, 8, endian::order::big>(sbuf.data() + off, v);
    off += 8;
  };
  auto put32 = [&](uint32_t v) {
    endian::endian_store<uint32_t, 4, endian::order::big>(sbuf.data() + off, v);
    off += 4;
  };
  auto put16 = [&](uint16_t v) {
    endian::endian_store<uint16_t, 2, endian::order::big>(sbuf.data() + off, v);
    off += 2;
  };
  auto putBytes = [&](const uint8_t *p, size_t n) {
    std::memcpy(sbuf.data() + off, p, n);
    off += n;
  };

  put64(conn_id);
  put32(globals::UDP_action_announce);
  put32(transaction_id);
  putBytes(cb_->info_hash_.data(), globals::standard_length);
  putBytes(cb_->peer_id_.data(), globals::standard_length);
  put64(cb_->downloaded_);
  put64(cb_->left_);
  put64(cb_->uploaded_);
  put32(cb_->event_);
  put32(globals::default_ip);
  put32(globals::default_key);
  put32(globals::default_num_want);
  put16(6881); // should not be magic number

  boost::system::error_code ec;
  socket_.send(boost::asio::buffer(sbuf), 0, ec);
  if (ec)
    return;

  // --- RECEIVE ---
  std::array<uint8_t, 1220> resp_buf{};
  size_t resp_size = socket_.receive(boost::asio::buffer(resp_buf), 0, ec);
  if (ec || resp_size < 20)
    return;

  uint32_t resp_action =
      boost::endian::endian_load<uint32_t, 4, boost::endian::order::big>(
          resp_buf.data());

  if (resp_action != globals::UDP_action_announce)
    return;

  uint32_t resp_transaction_id =
      boost::endian::endian_load<uint32_t, 4, boost::endian::order::big>(
          resp_buf.data() + 4);

  if (resp_transaction_id != transaction_id)
    return;

  interval_ =
      boost::endian::endian_load<uint32_t, 4, boost::endian::order::big>(
          resp_buf.data() + 8);

  uint32_t leechers =
      boost::endian::endian_load<uint32_t, 4, boost::endian::order::big>(
          resp_buf.data() + 12);

  uint32_t seeders =
      boost::endian::endian_load<uint32_t, 4, boost::endian::order::big>(
          resp_buf.data() + 16);

  size_t peer_count = (resp_size - 20) / 6;
  for (size_t i = 0; i < peer_count; ++i) {
    size_t peer_offset = 20 + i * 6;
    uint32_t peer_ip =
        boost::endian::endian_load<uint32_t, 4, boost::endian::order::big>(
            resp_buf.data() + peer_offset);
    uint16_t peer_port =
        boost::endian::endian_load<uint16_t, 2, boost::endian::order::big>(
            resp_buf.data() + peer_offset + 4);
    // std::println("Peer {}.{}.{}.{}:{}", peer_ip >> 24, (peer_ip >> 16) &
    // 0xFF,
    //              (peer_ip >> 8) & 0xFF, peer_ip & 0xFF, peer_port);
    cb_->peers_.insert({peer_ip, peer_port});
  }
}

/*
  Every minute, should reconnect to the tracker to keep the connection alive.
*/
void SessionUDP::run() {
  size_t n{};
  for (;;) {
    auto conn_id = connect();
    if (!conn_id) {
      std::this_thread::sleep_for(std::chrono::seconds(15 * (2 << n)));
      if (++n > 8)
        break;
      continue;
    }

    for (;;) {
      if (last_connect_ + std::chrono::minutes(globals::UDP_conn_validity) <
          std::chrono::steady_clock::now())
        break; // time to reconnect

      announce(*conn_id);
      std::this_thread::sleep_for(std::chrono::minutes(interval_));
    }
  }
}

/*--------------------- HTTP ------------------*/

SessionHTTP::SessionHTTP(std::string &&url, std::shared_ptr<Tracker_CB> cb)
    : Session::Session(std::move(url), cb) {}

void SessionHTTP::run() { ; }

} // namespace vtorrent::tracker