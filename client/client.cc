#include <string>
#include <iostream>
#include <fstream>
#include <array>
#include <filesystem>
#include <thread>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

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
            std::cout << "file eof" << std::endl;
            std::cout << "send byte: " << all_read_bytes << std::endl;
            return;
        }
        ifile.read(buffer.data(), buffer.size());
        size_t len = ifile.gcount();
        std::cout.flush();
        boost::asio::async_write(glb_socket, boost::asio::buffer(buffer.data(), len), 
            [this](const boost::system::error_code& error, size_t byte){
                if (!error){
                    all_read_bytes += byte;
                    send_file();
                } else {
                    std::cout << "write error: " << error << std::endl; 
                }
        });
    }

    void send_b(std::string& filename){
        file_len = std::filesystem::file_size(filename);
        std::string header{"set " + filename + " " + std::to_string(file_len) + '\n'};
        ifile.open(filename, std::ios::binary);
        std::cout << "send header start" << std::endl;
        std::cout.flush();
        boost::asio::async_write(glb_socket, boost::asio::buffer(str_to_vec(header)), 
        [this](const boost::system::error_code& error, size_t bytes)  {
            std::cout << "handler called" << std::endl;
            if (!error){
                std::cout << "sent header" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                send_file();
            } else{
                std::cout << "write header: " << error << std::endl;

            } 
        });
    }

    void receive_b(){
        while(true){
            boost::system::error_code ec;
            size_t bytes = glb_socket.read_some(boost::asio::buffer(buffer.data(), buffer.size()), ec);
            if (!ec){
                std::cout << "read bytes: " << bytes << std::endl;
                ofile.write(buffer.data(), bytes);
                all_read_bytes += bytes;
                if (all_read_bytes >= file_len){
                    ofile.flush();
                    break;
                }
            } else {
                std::cout << "receive file error: " << ec;
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
        std::cout << "file len: " << file_len << std::endl; 
        receive_b();
    }

};

int main(){

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
        std::cout << "unknown error\n";
    }
    io_context.run();
}
