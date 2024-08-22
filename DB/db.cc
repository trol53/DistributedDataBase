#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <map>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>
#include <fstream>
#include <thread>

namespace net = boost::asio;
using tcp = net::ip::tcp;

std::map<std::string, std::string> db;

class session : public std::enable_shared_from_this<session> {
 public:
  explicit session(tcp::socket socket) : socket_(std::move(socket)) {}

  void start() { read_request(); }

 private:
  tcp::socket socket_;
  boost::asio::streambuf buffer_;
  std::array<char, 512> const_buffer_;
  std::string data_;
  const std::string storage_path = "/opt/storage/";
  size_t all_read_bytes = 0;
  size_t file_size;

  std::vector<char> str_to_vec(const std::string& str){
      std::vector<char> res(str.size());
      for (size_t i = 0; i < str.size(); i++){
          res[i] = str[i];
      }
      return res;
  }
  

  void read_request() {
    std::cout << "reading request\n";
    std::cout.flush();
    auto self = shared_from_this();

    boost::asio::async_read_until(socket_, buffer_, '\n',
        [this, self](boost::system::error_code ec, std::size_t) {
          if (!ec) {
            std::string data(boost::asio::buffers_begin(buffer_.data()),
                 boost::asio::buffers_end(buffer_.data()));
            buffer_.consume(buffer_.size());
            data_ = std::move(data);
            process_request();
          } else {
            std::cout << "read error: " << ec.message() << std::endl;
          }
        });

    std::cout << "end of reading: " << std::endl;
  }

  void process_request() {
    std::stringstream ss1(this->data_);
    std::string command, hash;
    ss1 >> command >> hash;

    if (command == "get"){
      std::cout << "request: " << command << " " << hash << std::endl;
      handle_get(hash);
    
    }else if (command == "set") {
      ss1 >> file_size;
      std::cout << "request: " << command << " " << hash << " " << file_size << std::endl;
      handle_post(hash);
    } else{
      std::cout << "bad request" << std::endl;
    }
    
  }

  void handle_get(const std::string& hash) {
    boost::system::error_code ec;
    std::filesystem::path p(storage_path + hash);
    if (!std::filesystem::exists(p)){
      boost::asio::write(this->socket_, boost::asio::buffer(str_to_vec("dont exist")));
      return;
    }
    std::ifstream file(p, std::ios::binary);
    std::string header{std::to_string(std::filesystem::file_size(p)) + '\n'};
    size_t bytes = boost::asio::write(socket_ ,boost::asio::buffer(str_to_vec(header), header.size()), ec);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    while(true){
      if (file.eof()){
        std::cout << "send file to server" << std::endl;
        break;
      }
      file.read(const_buffer_.data(), const_buffer_.size());
      size_t read_len = file.gcount();
      size_t bytes = boost::asio::write(socket_, boost::asio::buffer(const_buffer_.data(), read_len), ec);
    }
    

  }

  void handle_post(const std::string& hash) {
    
    std::ofstream file(storage_path + hash, std::ios::binary);
    std::cout << "begin of reading block" << std::endl;
    while(true){
      boost::system::error_code ec;
      size_t bytes = socket_.read_some(boost::asio::buffer(const_buffer_.data(), const_buffer_.size()), ec);
      if (!ec){
          std::cout << "read bytes: " << bytes << " all read bytes:" << all_read_bytes << std::endl;
          file.write(const_buffer_.data(), bytes);
          all_read_bytes += bytes;
          if (all_read_bytes >= file_size){
              file.flush();
              boost::asio::write(socket_, boost::asio::buffer(str_to_vec("200\n")), ec);
              std::cout << "read file from server" << std::endl;
              break;
          }
      } else {
          std::cout << "receive file error: " << ec << std::endl;
          break;
      }
    }
  }

};

class server{
public:
  server(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)){
    do_accept();
  }

private:
  void do_accept(){
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket){
          if (!ec){
            std::make_shared<session>(std::move(socket))->start();
          }
          do_accept();
        });
  }

  tcp::acceptor acceptor_;
};

int main() {
  std::cout << "start db" << std::endl;
  try
    {

      boost::asio::io_context io_context;

      server s(io_context, std::atoi("8080"));

      io_context.run();
    } catch (std::exception& e){
      std::cerr << "Exception: " << e.what() << "\n";
    }
}
