#include <openssl/sha.h>

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
std::optional<std::string> Bencode::decode_string(std::ifstream& fstream, ustring& raw_info,
                                                  bool in_info) {
    unsigned length{};

    if (!(fstream >> length)) return std::nullopt;

    if (in_info) {
        auto temp = std::to_string(length);
        raw_info.append(temp.begin(), temp.end());
    }

    // // cast away ':'
    if (fstream.get() != consts::delim) return std::nullopt;
    if (in_info) raw_info.push_back(consts::delim);

    std::string decoded;
    decoded.resize(length);

    fstream.read(decoded.data(), length);
    if (in_info) raw_info.append(decoded.begin(), decoded.end());
    return decoded;
}

// e.g. i3e
std::optional<int64_t> Bencode::decode_integer(std::ifstream& fstream, ustring& raw_info,
                                               bool in_info) {
    int64_t i{};
    if (!(fstream >> i)) return std::nullopt;
    if (fstream.get() != consts::container_end) return std::nullopt;

    if (in_info) {
        auto temp = std::to_string(i);
        raw_info.append(temp.begin(), temp.end());
        raw_info.push_back(consts::container_end);
    }

    return i;
}

std::optional<BencodeList> Bencode::decode_list(std::ifstream& fstream, ustring& raw_info,
                                                bool in_info) {
    BencodeList list{};

    for (;;) {
        {
            // termination condition
            char c;
            if ((c = fstream.get()) == consts::container_end) {
                if (in_info) raw_info.push_back(consts::container_end);
                return list;
            } else
                fstream.putback(c);
        }

        auto val = decode_arbitrary(fstream, raw_info, in_info);
        if (!val) return std::nullopt;

        list.push_back(*val);
    }

    return list;
}

// e.g. d5:helloi5ee => {"hello": 5}
// assuming initial 'd' was consumed
std::optional<BencodeDictionary> Bencode::decode_dict(std::ifstream& fstream, ustring& raw_info,
                                                      bool in_info) {
    BencodeDictionary dict{};

    bool was_in_info = in_info;

    for (;;) {
        {
            // termination condition
            char c;
            if ((c = fstream.get()) == consts::container_end) {
                if (in_info) raw_info.push_back(consts::container_end);
                return dict;
            } else
                fstream.putback(c);
        }

        // keys must be strings
        auto d_string = decode_string(fstream, raw_info, in_info);
        if (!d_string) return std::nullopt;

        if (d_string == consts::key_info) in_info = true;

        // values can be any bencode value
        auto d_val = decode_arbitrary(fstream, raw_info, in_info);
        if (!d_val) return std::nullopt;

        if (!was_in_info && in_info) in_info = false;

        dict[*d_string] = *d_val;
    }

    return dict;
}

std::optional<BencodeValue> Bencode::decode_arbitrary(std::ifstream& fstream, ustring& raw_info,
                                                      bool in_info) {
    char c = fstream.get();
    if (in_info) raw_info.push_back(c);
    switch (c) {
        case consts::dict_start:
            return decode_dict(fstream, raw_info, in_info);

        case consts::list_start:
            return decode_list(fstream, raw_info, in_info);

        case consts::integer_start:
            return decode_integer(fstream, raw_info, in_info);

        default:
            if (in_info) raw_info.pop_back();
            fstream.putback(c);
            return decode_string(fstream, raw_info, in_info);
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
            std::println("null");
            ++it;
            continue;
        }

        // special case to make it pretty for dicts
        if (std::holds_alternative<BencodeDictionary>(it->second))
            dump(os, it->second, prefix + "    ");
        else
            dump(os, it->second, "");

        if (++it != dict.end()) std::print(",");
        std::println();
    }
    os << prefix << "}";
}

void Bencode::dump(std::ostream& os, const BencodeValue& v, const std::string& prefix) {
    std::visit([&](auto&& arg) { dump(os, arg, prefix); }, v);
}

// ustring Bencode::encode(int64_t i) {
//     auto temp = std::format("i{}e", i);
//     return ustring{temp.begin(), temp.end()};
// }

// ustring Bencode::encode(const std::string& s) {
//     auto temp = std::format("{}:{}", s.length(), s);
//     return ustring{temp.begin(), temp.end()};
// }

// ustring Bencode::encode(const BencodeList& l) {
//     ustring enc(1, consts::list_start);
//     for (const auto& val : l) enc += encode(val);
//     enc += consts::container_end;
//     return enc;
// }

// ustring Bencode::encode(const BencodeDictionary& d) {
//     ustring enc(1, consts::dict_start);
//     for (const auto& [k, v] : d) {
//         enc += encode(k) += encode(v);
//     }
//     enc += consts::container_end;
//     return enc;
// }

// ustring Bencode::encode(const BencodeValue& val) {
//     return std::visit([&](auto&& arg) { return encode(arg); }, val);
// }

void Bencode::compute_infohash(ustring& raw_info) {
    SHA1(raw_info.data(), raw_info.length(), infohash.data());
}

/*--------------------- public implementation ------------------*/

Bencode::Bencode(const fs::path& path) {
    std::ifstream file_stream(path, std::ifstream::in);
    if (file_stream.get() != consts::dict_start) {
        dict = std::nullopt;
        return;
    }

    ustring raw_info{};
    bool in_info = false;
    dict = decode_dict(file_stream, raw_info, in_info);
    compute_infohash(raw_info);
}

void Bencode::json_dump(std::ostream& os) { Bencode::dump(os, *dict, ""); }

}  // namespace vtorrent