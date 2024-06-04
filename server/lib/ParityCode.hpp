#ifndef RECOVER_HPP_
#define RECOVER_HPP_

#include <vector>
#include <sstream>
#include <string>
#include <algorithm>
#include <string_view>
#include <boost/uuid/detail/md5.hpp>
#include <boost/algorithm/hex.hpp>


class ParityCode {
public:
    ParityCode (const std::string& file_name) : file_name_(file_name){};
    
    ParityCode (const std::string& file_name, const std::string& content, int num_of_blocks) :
        file_name_(file_name), file_content_(content), num_of_blocks_(num_of_blocks){
            set_blocks();
        } 

    std::string get_file_by_blocks(std::vector<std::string>& blocks){
        std::string file;
        for (auto s : blocks){
            file.append(s);
        }
        return file;
    }

    std::string get_file_by_parity(std::string& s1, std::string& parity, int block){
        std::string file;
        if (block == 0){
            file = get_parity(s1, parity) + s1;
        } else {
            file = s1 + get_parity(s1, parity);
        }
        return file;

    }
    
    std::vector<std::string> get_blocks(){
        return blocks_;
    }

    std::string digesttype_to_string(const boost::uuids::detail::md5::digest_type &digest){
        const auto charDigest = reinterpret_cast<const char *>(&digest);
        std::string result;
        boost::algorithm::hex(charDigest, charDigest + sizeof(boost::uuids::detail::md5::digest_type), std::back_inserter(result));
        return result;
    }

    std::string get_hash(){
        boost::uuids::detail::md5 hash;
        boost::uuids::detail::md5::digest_type digest;

        hash.process_bytes(file_name_.data(), file_name_.size());
        hash.get_digest(digest);

        return digesttype_to_string(digest);
    }

    std::string get_parity(){
        return get_parity(blocks_[0], blocks_[1]);
    }

    void set_blocks(){
        blocks_.push_back(std::string(
            file_content_.begin(), file_content_.begin() + file_content_.size() / num_of_blocks_));
        blocks_.push_back(std::string(
            file_content_.begin() + file_content_.size() / num_of_blocks_, file_content_.end()));
    }

    std::string get_parity(std::string& s1, std::string& s2){
        std::string parity;
        if (s1.size() > s2.size())
            std::swap(s1, s2);
        int len = s1.size();
        for (int i = 0; i < len; i++){
            parity.push_back(s1[i] ^ s2[i]);
        }
        parity.append(s2.begin() + len, s2.end());
        return parity;
    }

    std::string file_name_;
    std::string file_content_;
    std::vector<std::string> blocks_;
    int num_of_blocks_;
};

#endif
