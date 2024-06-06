#ifndef FILE_HANDLER_H
#define FILE_HANDLER H

#include <vector>
#include <string>
#include <boost/asio.hpp>
#include <iostream>
#include <sstream>

using boost::asio::ip::tcp;
class DbInterface{
public:

    DbInterface() = default;

    std::pair<int, std::vector<std::string>> GetFile(std::string& hash){
        std::vector<std::string> data;
        int error_check = -1;
        try{
            for (int i = 0; i < disk_numbers; i++){
                data.push_back(GetBlock(i, hash));
                std::cout << "recieve block from db: " << data.back() << '\n';
                if (data.back() == "Error"){
                    error_check = i;
                    data.pop_back();
                }
            }
            if (error_check != -1){
                data.push_back(GetBlock(2, hash));
            }
        } catch (std::exception& e){
            std::cout << e.what() << '\n';
        }
        return std::make_pair(error_check, data);
        
    }

    std::string GetBlock(int num, std::string hash){
        try {
            boost::asio::io_service io_service;

            tcp::resolver resolver(io_service);
            tcp::resolver::query query(tcp::v4(), ip[num], ports[num]);
            tcp::resolver::iterator iterator = resolver.resolve(query);

            tcp::socket socket(io_service);
            boost::asio::connect(socket, iterator);
            std::string request = std::string("get ") + hash;
            std::cout << "hash size: " << hash.size() << std::endl;
            std::vector<char> buffer = str_to_vec(request);
            boost::asio::write(socket, boost::asio::buffer(buffer));

            boost::asio::streambuf buffer_r;
            std::stringstream ss;

            boost::asio::read_until(socket, buffer_r, '\0');
            std::string data(boost::asio::buffers_begin(buffer_r.data()),
                 boost::asio::buffers_end(buffer_r.data()));
            data.pop_back();
            return data;

        } catch (std::exception& e) {
            std::cout << "get block error: " << e.what();
            return "Error";
        }
    }

    std::vector<char> str_to_vec(std::string& str){
        std::vector<char> res(str.size());
        for (size_t i = 0; i < str.size(); i++){
            res[i] = str[i];
        }
        res.push_back('\0');
        return res;
    }

    void SetFile(const std::string& hash, std::vector<std::string>& data){
        for (int i = 0; i <= disk_numbers; i++){
            SetBlock(hash, data[i], i);
        }
    }

    void SetBlock(const std::string& hash, std::string& block, int index){
        try {
            std::cout << "block " << index << ": " << block << std::endl;
            boost::asio::io_service io_service; 

            tcp::resolver resolver(io_service);
            tcp::resolver::query query(tcp::v4(), ip[index], ports[index]);
            tcp::resolver::iterator iterator = resolver.resolve(query);

            tcp::socket socket(io_service);
            boost::asio::connect(socket, iterator);
            std::string request = std::string("set ") + hash + " " + block;
            std::cout << "hash size: " << hash.size() << std::endl;
            std::vector<char> buffer = str_to_vec(request);
            boost::asio::write(socket, boost::asio::buffer(buffer));
            std::cout << "end of writing\n";

        } catch (std::exception& e) {
            std::cout << "DB Error " << e.what();
            std::cout.flush();
        }
    }

    std::vector<std::string> ip = {"127.0.0.1", "127.0.0.1", "127.0.0.1"};
    std::vector<std::string> ports = {"8081", "8082", "8083"};
    size_t disk_numbers = 2;
};

#endif