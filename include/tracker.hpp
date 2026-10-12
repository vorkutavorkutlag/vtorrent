#include <array>
#include <constants.hpp>
#include <memory>

namespace vtorrent {
class SessionHTTP {
private:
  std::weak_ptr<std::array<unsigned char, consts::infohash_length>> infohash;
  std::weak_ptr<std::string> client_id;

  // IP parameter deprecated
  uint16_t port;

  size_t uploaded;

  size_t downloaded;

  size_t left;

  uint8_t event;

  // numwant parameter deprecated

public:
};
} // namespace vtorrent