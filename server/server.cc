#include <fstream>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <memory>
#include <utility>
#include <boost/asio/streambuf.hpp>

#include "lib/ParityCode.hpp"
#include "db_interface.hpp"

using boost::asio::ip::tcp;

class session
  : public std::enable_shared_from_this<session>{
public:
  session(tcp::socket socket)
    : socket_(std::move(socket)){}

  void start(){
    do_read();
  }

  void do_read(){
    auto self(shared_from_this());
    boost::asio::async_read_until(socket_, buffer_, '\0',
        [this, self](boost::system::error_code ec, std::size_t) {
          if (!ec) {
            std::string data(boost::asio::buffers_begin(buffer_.data()),
                 boost::asio::buffers_end(buffer_.data()));

// Очистка буфера
            buffer_.consume(buffer_.size());
            data_ = data;
            data_.pop_back();
            handler();
            
            
            
          }
        });
  }

  void handler()
  {
    // auto self(shared_from_this());
    std::vector<std::string> mess = split(data_);
    DbInterface db(num_of_nodes);
    if (mess[0] == "get"){
      ParityCode pc(mess[1]);
      std::string hash = pc.get_hash();
      std::pair<int, std::vector<std::string>> file_blocks = db.GetFile(hash);
      std::string file;
      if (file_blocks.first == -1){
        file = pc.get_file_by_blocks(file_blocks.second);
        std::cout << "file blocks size: " << file_blocks.second[0].size() << " " << file_blocks.second[1].size() << std::endl; 
      } else {
        std::string parity = file_blocks.second.back();
        file_blocks.second.pop_back();
        try{
          file = pc.get_file_by_parity(file_blocks.second, parity, file_blocks.first);
        } catch(std::exception& e){
          std::cout << "get file error: " << e.what() << std::endl;
        }
      }
      socket_.write_some(boost::asio::buffer(db.str_to_vec(file)));
      // std::cout << "get" << mess[1];
    }else{
      std::cout << "set " << mess[1] << " " << mess[2] << std::endl;
      ParityCode pc(mess[1], mess[2], 2);
      std::string hash = pc.get_hash();
      std::vector<std::string> blocks = pc.get_blocks();
      std::string parity = pc.get_parity();
      blocks.push_back(parity);
      std::cout.flush();
      try {
        db.SetFile(hash, blocks);
      } catch(std::exception& e){
        std::cout << "Server Error " << e.what();
      }
    }
    std::cout.flush();
  }

  std::vector<std::string> split(std::string& str){
    std::vector<std::string> result(3);
    std::stringstream ss1 (str), ss2;

    ss1 >> result[0];
    ss1 >> result[1];
    if (result[0] == "set"){
      ss2 << ss1.rdbuf();
      result[2] = ss2.str();
      result[2] = std::string(result[2].begin() + 1, result[2].end());
    }
    
    return result;
  }



  tcp::socket socket_;
  std::string data_;
  boost::asio::streambuf buffer_;
  int num_of_nodes = 2;
};

class server
{
public:
  server(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)){
    do_accept();
  }

private:
  void do_accept()
  {
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

int main(int argc, char* argv[])
{
  try
  {

    boost::asio::io_context io_context;

    server s(io_context, std::atoi("1444"));

    io_context.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

