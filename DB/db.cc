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

namespace net = boost::asio;
using tcp = net::ip::tcp;

std::map<std::string, std::string> db;

class session : public std::enable_shared_from_this<session> {
 public:
  explicit session(tcp::socket socket) : socket_(std::move(socket)) {}

  void start() { read_request(); }

 private:
  tcp::socket socket_;
  // beast::flat_buffer buffer_;
  boost::asio::streambuf buffer_;
  std::string data_;
  const std::string storage_path = "/opt/storage/";

  std::vector<char> str_to_vec(const std::string& str){
      std::vector<char> res(str.size());
      for (size_t i = 0; i < str.size(); i++){
          res[i] = str[i];
      }
      res.push_back('\0');
      return res;
  }
  void print_map(){
    if(!db.empty()){
      for (auto it : db){
        std::cout << it.first << " " << it.second << std::endl;
      }
    }
  }

  void read_request() {
    std::cout << "reading request\n";
    std::cout.flush();
    auto self = shared_from_this();
    print_map();

    boost::asio::async_read_until(socket_, buffer_, '\0',
        [this, self](boost::system::error_code ec, std::size_t) {
          if (!ec) {
            std::string data(boost::asio::buffers_begin(buffer_.data()),
                 boost::asio::buffers_end(buffer_.data()));

// Очистка буфера/home/trol53/pets/DistributedDataBase/client/build/test
            buffer_.consume(buffer_.size());
            data_ = data;
            data_.pop_back();
            std::cout << "get data: " << data << std::endl;
            std::cout.flush();
            process_request();
          } else {
            std::cout << "read error: " << ec.message() << std::endl;
            std::cout.flush();
          }
        });

    std::cout << "end of reading: " << std::endl;
  }

  void process_request() {
    std::stringstream ss1(this->data_), ss2;
    std::string command, hash, file;
    ss1 >> command >> hash;

    if (command == "get"){
      std::cout << "get request: " << command << " " << hash;
      std::cout << "hash size: " << hash.size() << std::endl;
      std::cout.flush();
      handle_get(hash);
    
    }else if (command == "set") {
      ss2 << ss1.rdbuf();
      file = ss2.str();
      file = std::string(file.begin() + 1, file.end()); 
      std::cout << "set request: " << command << " " << hash << " " << file;
      std::cout << "hash size: " << hash.size() << std::endl;
      std::cout.flush();
      handle_post(hash, file);
    } else{

      std::cout << "bad request\n";
      std::cout.flush();
    }
    
  }

  void handle_get(const std::string& hash) {
    
    std::filesystem::path p(storage_path + hash);
    if (!std::filesystem::exists(p)){
      boost::asio::write(this->socket_, boost::asio::buffer(str_to_vec("dont exist")));
      return;
    }
    std::ifstream file(p);
    std::stringstream buffer;
    buffer << file.rdbuf();
    boost::asio::write(this->socket_, boost::asio::buffer(str_to_vec(buffer.str())));

  }

  void handle_post(const std::string& hash, const std::string& content) {
    
    std::filesystem::path p(storage_path + hash);
    std::ofstream file(p);
    file << content;
    std::cout << "set block: " << content << std::endl;
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
  std::cout << "start db\n";
  std::cout.flush();
  try
    {

      boost::asio::io_context io_context;

      server s(io_context, std::atoi("8080"));

      io_context.run();
    } catch (std::exception& e){
      std::cerr << "Exception: " << e.what() << "\n";
    }
}
