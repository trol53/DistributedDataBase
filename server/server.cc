#include <fstream>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string_view>
#include <memory>
#include <utility>
#include <boost/asio/streambuf.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/syslog_sink.h>


#include "lib/ParityCode.hpp"
#include "lib/toml.hpp"
#include "db_interface.hpp"

using boost::asio::ip::tcp;
using namespace std::string_view_literals;
auto config = toml::parse_file("/home/trol53/pets/DistributedDataBase/server/config.toml");
boost::asio::io_context io_context;
auto logger = spdlog::syslog_logger_mt("syslog", "server_ddb");

class session
  : public std::enable_shared_from_this<session>{
public:
  session(tcp::socket socket)
    : socket_(std::move(socket)){}

  void start(std::vector<std::pair<std::string, std::string>>& nodes_params){
    do_read(nodes_params);
  }

  void do_read(std::vector<std::pair<std::string, std::string>>& nodes_params){
    auto self(shared_from_this());
    boost::asio::async_read_until(socket_, buffer_, '\n',
        [this, self, &nodes_params](boost::system::error_code ec, std::size_t) {
          if (!ec) {
            std::string data(boost::asio::buffers_begin(buffer_.data()),
                 boost::asio::buffers_end(buffer_.data()));

            buffer_.consume(buffer_.size());
            data_ = std::move(data);
            data_.pop_back();
            handler(nodes_params);
          }
        });
  }


  void get_file(const std::string& file_name){
    std::ofstream ofile(file_name, std::ios::binary);
    while(true){
      boost::system::error_code ec;
      size_t bytes = socket_.read_some(boost::asio::buffer(const_buffer_.data(), const_buffer_.size()), ec);
      if (!ec){
          ofile.write(const_buffer_.data(), bytes);
          all_read_bytes += bytes;
          if (all_read_bytes >= file_len){
              ofile.flush();
              spdlog::info("read bytes from client: {0:d}", all_read_bytes);
              logger->info("read bytes from client: {0:d}", all_read_bytes);
              break;
          }
      } else {
          spdlog::error("receive file error: {0}", ec.category().name());
          logger->error("receive file error: {0}", ec.category().name());
          break;
      }
    }
    
  }

  std::vector<char> str_to_vec(std::string& str){
    std::vector<char> res(str.size());
    for (size_t i = 0; i < str.size(); i++){
        res[i] = str[i];
    }
    return res;
  }

  void set_file(const std::string& file_name){
    boost::system::error_code ec;
    std::ifstream ifile(file_name, std::ios::binary);
    std::string header{std::to_string(std::filesystem::file_size(file_name)) + '\n'};
    size_t bytes = boost::asio::write(socket_ ,boost::asio::buffer(str_to_vec(header), header.size()), ec);
    while(true){
      if (ifile.eof()){
        spdlog::info("send file to client success");
        logger->info("send file to client success");
        break;
      }
      ifile.read(const_buffer_.data(), const_buffer_.size());
      size_t read_len = ifile.gcount();
      size_t bytes = boost::asio::write(socket_, boost::asio::buffer(const_buffer_.data(), read_len), ec);
    }
    std::remove(file_name.c_str());
  }

  void handler(std::vector<std::pair<std::string, std::string>>& nodes_params)
  {
    std::vector<std::string> mess = split(data_);
    DbInterface db(nodes_params, logger);
    ParityCode pc;
    if (mess[0] == "get"){
      std::string hash = pc.get_hash(mess[1]);
      int ret = db.GetFile(hash);
      if (ret == 0){
        pc.get_file_by_blocks(hash, num_of_nodes); 
      } else {
        pc.get_file_by_parity(hash, num_of_nodes, ret);
      }
      set_file(hash);
    }else{
      std::string hash = pc.get_hash(mess[1]);
      get_file(hash);
      pc.get_blocks(hash, num_of_nodes, file_len);
      pc.get_parity(hash, num_of_nodes);
      try {
        db.SetFile(hash, io_context);
      } catch(std::exception& e){
        spdlog::error("Send file to db error {0}", e.what());
        logger->error("Send file to db error {0}", e.what());
      }
    }
  }

  std::vector<std::string> split(std::string& str){
    std::vector<std::string> result(3);
    std::stringstream ss1 (str);

    ss1 >> result[0];
    ss1 >> result[1];
    if (result[0] == "set"){
      ss1 >> file_len;
    }
    
    return result;
  }



  tcp::socket socket_;
  std::string data_;
  std::array<char, 512> const_buffer_;
  size_t file_len;
  size_t all_read_bytes = 0;
  boost::asio::streambuf buffer_;
  int num_of_nodes = 2;
};

class server
{
public:
  server(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)){
    conf_parse();
    do_accept();
  }

private:

  void conf_parse(){
    int num_of_nodes = config["num_of_nodes"].value_or(0);
    for (int i = 1; i <= num_of_nodes; i++){
      std::pair<std::string, std::string> node_params;
      auto tmp = config["node" + std::to_string(i)];
      node_params.first = tmp["address"].value_or(""sv);
      node_params.second = tmp["port"].value_or(""sv);
      spdlog::info("node{0:d}: {1}:{2}", i, node_params.first.c_str(), node_params.second.c_str());
      logger->info("node{0:d}: {1}:{2}", i, node_params.first.c_str(), node_params.second.c_str());
      nodes_params.push_back(node_params);
    }
  }

  void do_accept()
  {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket){
          if (!ec){
            std::make_shared<session>(std::move(socket))->start(this->nodes_params);
          }
          do_accept();
        });
  }

  std::vector<std::pair<std::string, std::string>> nodes_params;
  tcp::acceptor acceptor_;
};

int main(int argc, char* argv[]){
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l]  %v");
  try
  {

    // boost::asio::io_context io_context;

    server s(io_context, std::atoi("1444"));

    io_context.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}

