#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace vtorrent {

struct BencodeValue;

using BencodeDictionary = std::unordered_map<std::string, BencodeValue>;
using BencodeList = std::vector<BencodeValue>;

struct BencodeValue : std::variant<int64_t, std::string, BencodeList, BencodeDictionary> {
    using variant::variant;
};

class Bencode {
   private:
    static const char dict_start = 'd';
    static const char list_start = 'l';
    static const char container_end = 'e';
    static const char integer_start = 'i';
    static const char delim = ':';

    static std::optional<std::string> decode_string(std::ifstream& fstream);
    static std::optional<int64_t> decode_integer(std::ifstream& fstream);
    static std::optional<BencodeList> decode_list(std::ifstream& fstream);
    static std::optional<BencodeDictionary> decode_dict(std::ifstream& fstream);
    static std::optional<BencodeValue> decode_arbitrary(std::ifstream& fstream);

    static void dump(std::ostream& os, const BencodeValue& v, const std::string& prefix);
    static void dump(std::ostream& os, const BencodeDictionary& dict, const std::string& prefix);
    static void dump(std::ostream& os, const BencodeList& ls, const std::string& prefix);
    static void dump(std::ostream& os, const std::string& s, const std::string& prefix);
    static void dump(std::ostream& os, int64_t i, const std::string& prefix);

   public:
    static std::optional<BencodeDictionary> decode_file(const std::filesystem::path& path);

    std::optional<BencodeDictionary> dict;

    /* Returns an optional decoded dictionary from provided filepath, or nullopt if parsing error */
    Bencode(const std::filesystem::path& path) : dict(decode_file(path)) {}

    void json_dump(std::ostream& os = std::cout);
};
}  // namespace vtorrent