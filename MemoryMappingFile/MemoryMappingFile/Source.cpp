#include <iostream>
#include <cstring>
#include "MemoryMappingFile.h"

using namespace std;
void testMemoryMappingFile();


int main(int argc, char *argv[]) {
    std::cout << "start test:" << std::endl;
    testMemoryMappingFile();
    system("pause");
}




void testMemoryMappingFile() {
    std::string path = "mmap.log";

    std::cout << "path: mmap.log" << std::endl;
    MemoryMappingFile m = MemoryMappingFile(path, 1024, true, true);

    std::cout << "is_open():" << m.is_open() << std::endl;
    std::cout << "size():" << m.get_size() << std::endl;
    std::cout << "data():" << m.data() << std::endl;
    std::string s = "hello world!!!!";

    strcpy(m.data(), s.c_str());

    std::cout << "open path again: mmap.log" << std::endl;
    MemoryMappingFile m2 = MemoryMappingFile(path, 1024, true, true);

    std::cout << "is_open():" << m2.is_open() << std::endl;
    std::cout << "size():" << m2.get_size() << std::endl;
    std::cout << "data():" << m2.data() << std::endl;
}
