#include <constants.hpp>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace vtorrent {
typedef std::basic_string<unsigned char> ustring;

struct BencodeValue;

using BencodeDictionary = std::unordered_map<std::string, BencodeValue>;
using BencodeList = std::vector<BencodeValue>;

struct BencodeValue : std::variant<int64_t, std::string, BencodeList, BencodeDictionary> {
    using variant::variant;
};

class Bencode {
   private:
    static std::optional<std::string> decode_string(std::ifstream& fstream, ustring& raw_info,
                                                    bool in_info);
    static std::optional<int64_t> decode_integer(std::ifstream& fstream, ustring& raw_info,
                                                 bool in_info);
    static std::optional<BencodeList> decode_list(std::ifstream& fstream, ustring& raw_info,
                                                  bool in_info);
    static std::optional<BencodeDictionary> decode_dict(std::ifstream& fstream, ustring& raw_info,
                                                        bool in_info);
    static std::optional<BencodeValue> decode_arbitrary(std::ifstream& fstream, ustring& raw_info,
                                                        bool in_info);

    static void dump(std::ostream& os, const BencodeValue& v, const std::string& prefix);
    static void dump(std::ostream& os, const BencodeDictionary& dict, const std::string& prefix);
    static void dump(std::ostream& os, const BencodeList& ls, const std::string& prefix);
    static void dump(std::ostream& os, const std::string& s, const std::string& prefix);
    static void dump(std::ostream& os, int64_t i, const std::string& prefix);

    // ustring encode(int64_t i);
    // ustring encode(const std::string& s);
    // ustring encode(const BencodeList& v);
    // ustring encode(const BencodeDictionary& v);
    // ustring encode(const BencodeValue& v);

    void compute_infohash(ustring& raw_info);

   public:
    std::optional<BencodeDictionary> dict;
    std::array<unsigned char, consts::infohash_length> infohash;

    /* Returns an optional decoded dictionary from provided filepath, or nullopt if parsing
       error */
    Bencode(const std::filesystem::path& path);

    void json_dump(std::ostream& os = std::cout);
};
}  // namespace vtorrent