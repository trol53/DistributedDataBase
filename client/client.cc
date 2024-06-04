#include <algorithm>
#include <boost/asio.hpp>
#include <experimental/iterator>
#include <fstream>
#include <iostream>
#include <iterator>
#include <streambuf>
#include <string>
#include <vector>

using boost::asio::ip::tcp;

std::vector<char> str_to_vec(std::string& str) {
  std::vector<char> res;
  for (size_t i = 0; i < str.size(); i++) {
    res.push_back(str[i]);
  }
  res.push_back('\0');
  return res;
}
void send_file(tcp::socket& socket, const std::string& filename) {
  std::ifstream file(filename);
  if (file) {
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    std::string message = "set " + filename + " " + content;
    std::vector<char> vec_content = str_to_vec(message);
    boost::asio::write(socket, boost::asio::buffer(vec_content));
  } else {
    std::cerr << "Could not open file!\n";
  }
}
void send_file_v(tcp::socket& socket, const std::string& filename) {
  std::string message = "get " + filename;
  std::vector<char> nums = str_to_vec(message);
  std::copy(nums.begin(), nums.end(),
            std::experimental::make_ostream_joiner(std::cout, ", "));
  boost::asio::write(socket, boost::asio::buffer(nums));
}


void receive_file(tcp::socket& socket, const std::string& filename) {
  send_file_v(socket, filename);

  std::ofstream output_file(filename, std::ios::binary | std::ios::trunc);
  if (!output_file) {
    std::cerr << "Could not open file for writing: " << filename << std::endl;
    return;
  }

  try {
    boost::system::error_code error;
    boost::asio::streambuf buffer;
    while (boost::asio::read(socket, buffer, boost::asio::transfer_at_least(1),
                             error)) {
      output_file << &buffer;
    }

    if (error != boost::asio::error::eof) {
      throw boost::system::system_error(error);
    }

    output_file.close();

    // Open the file and print its contents to the console
    std::ifstream input_file(filename, std::ios::binary);
    if (input_file) {
      std::string content((std::istreambuf_iterator<char>(input_file)),
                          std::istreambuf_iterator<char>());
      std::cout << "Received file contents:\n" << content << std::endl;
    } else {
      std::cerr << "Could not open file for reading: " << filename << std::endl;
    }
  } catch (const std::exception& e) {
    std::cerr << "An error occurred while receiving the file: " << e.what()
              << std::endl;
  }
}

void send_string(tcp::socket& socket, const std::string& str) {
  boost::asio::write(socket, boost::asio::buffer(str));
}

std::string read_string(tcp::socket& socket) {
  boost::asio::streambuf buffer;
  boost::asio::read_until(socket, buffer, '\n');
  std::istream input_stream(&buffer);
  std::string str;
  std::getline(input_stream, str);
  return str;
}

int main() {
  boost::asio::io_service io_service;

  tcp::resolver resolver(io_service);
  tcp::resolver::query query(tcp::v4(), "127.0.0.1", "1444");
  tcp::resolver::iterator iterator = resolver.resolve(query);

  tcp::socket socket(io_service);
  boost::asio::connect(socket, iterator);

  std::string command;
  std::getline(std::cin, command);

  if (command == "send") {
    std::cout << "Enter file path: ";
    std::string filename;
    std::getline(std::cin, filename);
    size_t firstNonSpace = filename.find_first_not_of(" ");
    if (firstNonSpace != std::string::npos) {
      filename = filename.substr(firstNonSpace);
    }

    send_file(socket, filename);
  } else if (command == "get") {
    std::cout << "Enter output file path: ";
    std::string filename;
    std::getline(std::cin, filename);
    receive_file(socket, filename);
  } else {
    std::cerr << "Unknown command!\n";
  }

  return 0;
}
