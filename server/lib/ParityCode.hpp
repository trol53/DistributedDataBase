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
     
    void get_parity(std::string& block_file_name, std::string& parity_file_name){
    
        std::ifstream parity_ifile{parity_file_name, std::ios::binary | std::ios::ate};
        std::ifstream block_file{block_file_name, std::ios::binary | std::ios::ate};
        std::ofstream parity_ofile{parity_file_name + "_tmp", std::ios::binary}; 
        size_t block_len = block_file.tellg();
        size_t parity_len = parity_ifile.tellg();
        block_file.seekg(0);
        parity_ifile.seekg(0);
        std::array<char, 100> parity_tmp, block_tmp;
        //TODO: xor by blocks
        for(int i = 0; i < std::min(block_len, parity_len); ){
            int len_p = 0, len_b = 0;
            block_file.read(block_tmp.data(), block_tmp.size());
            parity_ifile.read(parity_tmp.data(), parity_tmp.size());
            len_p = parity_ifile.gcount();
            len_b = block_file.gcount(); 
            for (int j = 0; j < std::min(len_p, len_b); j++){
                parity_tmp[j] = parity_tmp[j] ^ block_tmp[j];
            }
            i += parity_tmp.size();
            parity_ofile.write(parity_tmp.data(), len_p);
            
            
        }
        if (!parity_ifile.eof()){
            parity_ofile << parity_ifile.rdbuf();
        }
        parity_ofile.flush();
        std::remove(parity_file_name.c_str());
        std::rename((parity_file_name + "_tmp").c_str(), parity_file_name.c_str());
        
        
    }

    std::string digesttype_to_string(const boost::uuids::detail::md5::digest_type &digest){
        const auto charDigest = reinterpret_cast<const char *>(&digest);
        std::string result;
        boost::algorithm::hex(charDigest, charDigest + sizeof(boost::uuids::detail::md5::digest_type), std::back_inserter(result));
        return result;
    }

    std::string get_hash(std::string& file_name){
        boost::uuids::detail::md5 hash;
        boost::uuids::detail::md5::digest_type digest;

        hash.process_bytes(file_name.data(), file_name.size());
        hash.get_digest(digest);

        return digesttype_to_string(digest);
    }

    void get_blocks(std::string& file_name, float num_of_blocks, float file_size){

        std::ifstream s(file_name);
        int size_of_block = static_cast<int>(std::ceil(file_size / num_of_blocks));
        std::array<char, 100> tmp;
        for (int i = 1; i <= static_cast<int>(num_of_blocks); i++){
            std::string tmp_filename = file_name + std::to_string(i);
            std::ofstream block_file{tmp_filename, std::ios::binary};
            for (int j = 0; j <= size_of_block;){
                if (s.eof())
                    return;
                if (j + tmp.size() > size_of_block){
                    s.read(tmp.data(), size_of_block - j);
                    block_file.write(tmp.data(), s.gcount());
                    break;
                }
                s.read(tmp.data(), tmp.size());
                block_file.write(tmp.data(), s.gcount());
                j += tmp.size();
            }
            block_file.flush();
        }
        std::remove(file_name.c_str());
    }

    void get_parity(std::string& file_name, int num_of_blocks){
        std::string parity_file_name{file_name + "_parity"};

        std::ofstream file_parity{parity_file_name};
        std::ifstream first_block{file_name + "1"};
        file_parity << first_block.rdbuf();
        file_parity.flush();
        for (int i = 2; i <= num_of_blocks; i++){
            std::string block_filename = file_name + std::to_string(i);
            get_parity(block_filename, parity_file_name);
        }
    }

    void get_file_by_blocks(const std::string& file_name, int num_of_blocks){
        std::array<char, 100> tmp;
        std::ofstream file{file_name, std::ios::binary};
        for(int i = 1; i <= num_of_blocks; i++){
            std::string block_file_name{file_name + std::to_string(i)}; 
            std::ifstream block_file{block_file_name, std::ios::binary};
            while (!block_file.eof()){
                block_file.read(tmp.data(), tmp.size());
                file.write(tmp.data(), block_file.gcount());
            }
            std::remove(block_file_name.c_str());
        }
    }

    void get_file_by_parity(std::string& file_name, int num_of_blocks, int broken_block){
        std::rename(std::string(file_name + "_parity").c_str(), std::string(file_name + std::to_string(broken_block)).c_str());
        get_parity(file_name, num_of_blocks);
        std::rename(std::string(file_name + "_parity").c_str(), std::string(file_name + std::to_string(broken_block)).c_str());
        get_file_by_blocks(file_name, num_of_blocks);
    }
    
    int num_of_blocks_;
};

#endif
