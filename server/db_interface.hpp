#ifndef FILE_HANDLER_H
#define FILE_HANDLER H

#include <vector>
#include <string>
#include <boost/asio.hpp>
#include <iostream>
#include <sstream>
#include <thread>
#include <filesystem>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/syslog_sink.h>

using boost::asio::ip::tcp;



class DbInterface{
public:

    DbInterface() = default;
    DbInterface(std::vector<std::pair<std::string, std::string>>& nodes_params, std::shared_ptr<spdlog::logger> logger) : 
    nodes_params_(nodes_params), logger_(logger){
        num_of_nodes_ = nodes_params_.size();
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l]  %v");
    };

    int GetFile(std::string& hash){
        int error_check = -1;
        for (int i = 0; i < num_of_nodes_ - 1; i++){
            int ret = GetBlock(hash, i);
            if (ret == 500){
                error_check = i;
            }
        }
        if (error_check != -1){
            int ret = GetBlock(hash, num_of_nodes_ - 1);
            return error_check + 1;
        }
        return 0;
        
    }

    int GetBlock(std::string& hash, int num){
        try {
            boost::system::error_code ec;
            boost::asio::io_service io_service;

            tcp::resolver resolver(io_service);
            tcp::resolver::query query(tcp::v4(), nodes_params_[num].first, nodes_params_[num].second);
            tcp::resolver::iterator iterator = resolver.resolve(query);

            tcp::socket db_socket(io_service);
            
            boost::asio::connect(db_socket, iterator, ec);
            if (ec)
                return 500;
            io_service.run();
            std::string header = std::string("get ") + hash + '\n';
            std::vector<char> buffer = str_to_vec(header);
            boost::asio::write(db_socket, boost::asio::buffer(buffer, buffer.size()), ec);
            if (ec)
                return 500;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            std::string file_name;
            if (num == num_of_nodes_ - 1)
                file_name = hash + "_parity";
            else
                file_name = hash + std::to_string(num + 1);


            std::array<char, 512> const_buffer;
            boost::asio::streambuf boost_buffer;
            size_t bytes = boost::asio::read_until(db_socket, boost_buffer, '\n');
            std::string len(boost::asio::buffers_begin(boost_buffer.data()),
                 boost::asio::buffers_end(boost_buffer.data()));
            size_t file_len = std::atoll(len.c_str());
            size_t all_read_bytes = 0;
            std::ofstream ofile(file_name, std::ios::binary);
            while(true){
                size_t bytes = db_socket.read_some(boost::asio::buffer(const_buffer.data(), const_buffer.size()), ec);
                if (!ec){
                    ofile.write(const_buffer.data(), bytes);
                    all_read_bytes += bytes;
                    if (all_read_bytes >= file_len){
                        ofile.flush();
                        spdlog::info("get block {0:d} from db", num + 1);
                        logger_->info("get block {0:d} from db", num + 1);
                        break;
                    }
                } else {
                    spdlog::error("receive block {0:d} with error: {1}", num + 1, ec.message().c_str());
                    logger_->error("receive block {0:d} with error: {1}", num + 1, ec.message().c_str()); 
                    return 500;
                }
            }

            

        } catch (std::exception& e) {
            spdlog::error("receive block {0:d} error:{1}", num + 1, e.what());
            logger_->error("receive block {0:d} error:{1}", num + 1, e.what());
            return 500;
        }
        return 200;
    }

    std::vector<char> str_to_vec(std::string& str){
        std::vector<char> res(str.size());
        for (size_t i = 0; i < str.size(); i++){
            res[i] = str[i];
        }
        return res;
    }

    void SetFile(const std::string& hash, boost::asio::io_context& io_context){
        for (int i = 0; i < num_of_nodes_; i++)
            SetBlock(hash, i, io_context);
    }

    void SetBlock(const std::string& hash, int index, boost::asio::io_context& io_context){
        try {           
            boost::system::error_code ec;
            tcp::resolver resolver(io_context);
            tcp::resolver::query query(tcp::v4(), nodes_params_[index].first, nodes_params_[index].second);
            tcp::resolver::iterator iterator = resolver.resolve(query);
            tcp::socket socket(io_context);
            boost::asio::connect(socket, iterator, ec);
            std::string file_name;
            if (index != num_of_nodes_ - 1)
                file_name = hash + std::to_string(index + 1);
            else
                file_name = hash + "_parity";
            std::ifstream ifile(file_name, std::ios::binary);
            size_t file_len = std::filesystem::file_size(file_name);
            size_t all_send_bytes = 0;
            std::string header = std::string("set ") + hash + " " + std::to_string(file_len) + '\n';
            std::vector<char> buffer = str_to_vec(header);
            boost::asio::write(socket, boost::asio::buffer(buffer, buffer.size()), boost::asio::transfer_all(), ec);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            if (ec){
                spdlog::error("error to send header to db: {0}", ec.message());
                logger_->error("error to send header to db: {0}", ec.message());
            }
            std::array<char, 512> const_buffer;
            
            while (true){
                if (ifile.eof()){
                    break;
                }

                ifile.read(const_buffer.data(), const_buffer.size());
                size_t read_len = ifile.gcount();
                size_t bytes = boost::asio::write(socket, boost::asio::buffer(const_buffer, read_len), boost::asio::transfer_all(), ec);
                if (ec){
                    spdlog::error("write to db error:{0}", ec.message());
                    logger_->error("write to db error:{0}", ec.message());
                }
                
            }
            spdlog::info("send block {0:d} success", index + 1);
            logger_->info("send block {0:d} success", index + 1);
            std::remove(file_name.c_str());

        } catch (std::exception& e) {
            spdlog::error("Node {0:d} error:{1}", index + 1, e.what());
            logger_->error("Node {0:d} error:{1}", index + 1, e.what());
        }
    }

    
    std::vector<std::pair<std::string, std::string>> nodes_params_;
    std::shared_ptr<spdlog::logger> logger_;
    size_t num_of_nodes_ = 2;
};

#endif