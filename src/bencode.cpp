#include <bencode.hpp>
#include <fstream>
#include <iostream>
#include <optional>
#include <print>
#include <utility>
#include <variant>

namespace vtorrent {
namespace fs = std::filesystem;

/*--------------------- private implementation ------------------*/

// e.g. 4:spam
std::optional<std::string> Bencode::decode_string(std::ifstream& fstream) {
    unsigned length{};

    if (!(fstream >> length)) return std::nullopt;

    // // cast away ':'
    if (fstream.get() != delim) return std::nullopt;

    std::string decoded;
    decoded.resize(length);

    fstream.read(decoded.data(), length);
    return decoded;
}

// e.g. i3e
std::optional<int64_t> Bencode::decode_integer(std::ifstream& fstream) {
    int64_t i{};
    if (!(fstream >> i)) return std::nullopt;
    if (fstream.get() != container_end) return std::nullopt;

    return i;
}

std::optional<BencodeList> Bencode::decode_list(std::ifstream& fstream) {
    BencodeList list{};

    for (;;) {
        {
            // termination condition
            char c;
            if ((c = fstream.get()) == container_end)
                return list;
            else
                fstream.putback(c);
        }

        auto val = decode_arbitrary(fstream);
        if (!val) return std::nullopt;

        list.push_back(*val);
    }

    return list;
}

// e.g. d5:helloi5ee => {"hello": 5}
// assuming initial 'd' was consumed
std::optional<BencodeDictionary> Bencode::decode_dict(std::ifstream& fstream) {
    BencodeDictionary dict{};

    for (;;) {
        {
            // termination condition
            char c;
            if ((c = fstream.get()) == container_end)
                return dict;
            else
                fstream.putback(c);
        }

        // keys must be strings
        auto d_string = decode_string(fstream);
        if (!d_string) return std::nullopt;

        // values can be any bencode value
        auto d_val = decode_arbitrary(fstream);
        if (!d_val) return std::nullopt;

        dict[*d_string] = *d_val;
    }

    return dict;
}

std::optional<BencodeValue> Bencode::decode_arbitrary(std::ifstream& fstream) {
    char c = fstream.get();
    switch (c) {
        case dict_start:
            return decode_dict(fstream);

        case list_start:
            return decode_list(fstream);

        case integer_start:
            return decode_integer(fstream);

        default:
            fstream.putback(c);
            return decode_string(fstream);
    }
}

void Bencode::dump(std::ostream& os, int64_t i, const std::string& prefix) {
    std::print(os, "{}{}", prefix, i);
}

void Bencode::dump(std::ostream& os, const std::string& s, const std::string& prefix) {
    std::print(os, "{}\"{}\"", prefix, s);
}

void Bencode::dump(std::ostream& os, const BencodeList& ls, const std::string& prefix) {
    if (ls.size() <= 1)
        std::print(os, "[");
    else
        std::print(os, "[\n");

    for (auto it = ls.begin(); it != ls.end();) {
        dump(os, *it, prefix);

        if (++it != ls.end()) std::print(os, " , ");
    }

    if (ls.size() <= 1)
        std::print(os, "]");
    else
        std::print(os, "\n]");
}

void Bencode::dump(std::ostream& os, const BencodeDictionary& dict, const std::string& prefix) {
    os << "{\n";
    for (BencodeDictionary::const_iterator it = dict.begin(); it != dict.end();) {
        dump(os, it->first, prefix + "    ");  // guaranteed string

        std::print(":");

        if (it->first == "pieces") {
            std::println("\"I'm not printing that\"");
            ++it;
            continue;
        }

        // special case to make it pretty for dicts
        if (std::holds_alternative<BencodeDictionary>(it->second))
            dump(os, it->second, prefix + "    ");
        dump(os, it->second, "");

        if (++it != dict.end()) std::print(",");
        std::println();
    }
    os << "}";
}

void Bencode::dump(std::ostream& os, const BencodeValue& v, const std::string& prefix) {
    std::visit([&](auto&& arg) { dump(os, arg, prefix); }, v);
}

/*--------------------- public implementation ------------------*/

std::optional<BencodeDictionary> Bencode::decode_file(const fs::path& path) {
    std::ifstream file_stream(path, std::ifstream::in);
    if (file_stream.get() != dict_start) return std::nullopt;
    return decode_dict(file_stream);
}

void Bencode::json_dump(std::ostream& os) { Bencode::dump(os, *dict, ""); }
}  // namespace vtorrent