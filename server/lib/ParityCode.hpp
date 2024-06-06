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

    std::string get_file_by_blocks(const std::vector<std::string>& blocks){
        std::string file;
        for (const std::string& block : blocks){
            file.append(block);
        }
        return file;
    }

    std::string get_file_by_parity(std::vector<std::string>& blocks, std::string& parity, int broken_block){
        std::string last_parity = get_parity(blocks);
        std::string fix_block = get_parity(last_parity, parity);
        auto iter = blocks.begin() + broken_block;
        blocks.insert(iter, fix_block);
        return get_file_by_blocks(blocks);
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
        return get_parity(blocks_);
    }

    void set_blocks(){
        int step = file_content_.size() / num_of_blocks_;
        for (int i = 1; i <= num_of_blocks_; i++){
            if (i == num_of_blocks_){
                blocks_.push_back(std::string(file_content_.begin() + step * (i - 1), file_content_.end()));
                break;
            }
            blocks_.push_back(std::string(file_content_.begin() + step * (i - 1), file_content_.begin() + step * i));
        }
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

    std::string get_parity(std::vector<std::string>& blocks){
        std::string parity;
        parity = blocks[0];
        for (int i = 1; i < blocks.size(); i++){
            parity = get_parity(parity, blocks[i]); 
        }
        return parity;
    }

    std::string file_name_;
    std::string file_content_;
    std::vector<std::string> blocks_;
    int num_of_blocks_;
};

#endif
