#include <filesystem>
#include <string>
#include <iostream>

int main(){
	std::string file("/home/trol53/pets/DistributedDataBase/client/build/test1");
	std::filesystem::path p(file);
	std::cout << std::filesystem::exists(p);
}
