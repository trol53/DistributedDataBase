#include <string>
#include <iostream>
#include <fstream>
#include <array>
#include <filesystem>
#include <thread>
#include <boost/asio.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/syslog_sink.h>

using boost::asio::ip::tcp;
auto logger = spdlog::syslog_logger_mt("syslog", "client_ddb");

class client {

    std::ifstream ifile;
    std::ofstream ofile;
    std::array<char, 512> buffer;
    size_t file_len;
    tcp::socket glb_socket;
    boost::asio::streambuf boost_buffer;
    size_t all_read_bytes = 0;


public:

    client(tcp::socket socket) : glb_socket(std::move(socket)){};

    std::vector<char> str_to_vec(std::string& str) {
        std::vector<char> res;
        for (size_t i = 0; i < str.size(); i++) {
            res.push_back(str[i]);
        }
        return res;
    }

    void send_file(){
        if (ifile.eof()){
            spdlog::info("send success, bytes: {0:d}", all_read_bytes);
            logger->info("send success, bytes: {0:d}", all_read_bytes);
            return;
        }
        ifile.read(buffer.data(), buffer.size());
        size_t len = ifile.gcount();
        boost::asio::async_write(glb_socket, boost::asio::buffer(buffer.data(), len), 
            [this](const boost::system::error_code& error, size_t byte){
                if (!error){
                    all_read_bytes += byte;
                    send_file();
                } else {
                    spdlog::error("send file error: {0}", error.category().name());
                    logger->error("send file error: {0}", error.category().name()); 
                }
        });
    }

    void send_b(std::string& filename){
        file_len = std::filesystem::file_size(filename);
        std::string header{"set " + filename + " " + std::to_string(file_len) + '\n'};
        ifile.open(filename, std::ios::binary);
        boost::asio::async_write(glb_socket, boost::asio::buffer(str_to_vec(header)), 
        [this](const boost::system::error_code& error, size_t bytes)  {
            if (!error){
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                send_file();
            } else{
                spdlog::error("error write header: {0}", error.category().name());
                logger->error("error write header: {0}", error.category().name());

            } 
        });
    }

    void receive_b(){
        while(true){
            boost::system::error_code ec;
            size_t bytes = glb_socket.read_some(boost::asio::buffer(buffer.data(), buffer.size()), ec);
            if (!ec){
                ofile.write(buffer.data(), bytes);
                all_read_bytes += bytes;
                if (all_read_bytes >= file_len){
                    ofile.flush();
                    spdlog::info("succesfully receive");
                    logger->info("succesfully receive");
                    break;
                }
            } else {
                spdlog::error("receive file error: {0}", ec.category().name());
                logger->error("receive file error: {0}", ec.category().name());
                break;
            }
        }
    }

    void receive_file(std::string& filename){
        std::string header{"get " + filename + '\n'};
        ofile.open(filename);
        boost::system::error_code ec;
        size_t bytes = boost::asio::write(glb_socket, boost::asio::buffer(str_to_vec(header)), ec);
        bytes = boost::asio::read_until(glb_socket, boost_buffer, '\n');
        std::string len(boost::asio::buffers_begin(boost_buffer.data()),
                 boost::asio::buffers_end(boost_buffer.data()));
        file_len = std::atoll(len.c_str());
        receive_b();
    }

};

int main(){
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l]  %v");
    boost::asio::io_context io_context;
    tcp::resolver resolver(io_context);
    tcp::resolver::query query(tcp::v4(), "127.0.0.1", "1444");
    tcp::resolver::iterator iterator = resolver.resolve(query);

    tcp::socket socket(io_context);
    
    boost::asio::connect(socket, iterator);   
    client cl(std::move(socket));    
    std::string command;
    std::cout << "Enter command(send/get):";  
    std::getline(std::cin, command);  
    

    if (command == "send") {  
        std::cout << "Enter file path: ";   
        std::string filename;   
        std::getline(std::cin, filename);   
        size_t firstNonSpace = filename.find_first_not_of(" "); 
        if (firstNonSpace != std::string::npos) {   
            filename = filename.substr(firstNonSpace);    
        }   
        
        cl.send_b(filename);
    } else if (command == "get") {    
        std::cout << "Enter output file path: ";    
        std::string filename;   
        std::getline(std::cin, filename);
        cl.receive_file(filename);
    } else {
        spdlog::error("unknown command");
        logger->error("unknown command");
    }
    io_context.run();
}
