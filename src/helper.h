#ifndef __HELPER_H__
#define __HELPER_H__

#include <string>
#include <fstream>

// read the file content and generate a string from it.
std::string convertFileToString(const std::string& filename) {
    std::ifstream ifile(filename);
    if (!ifile){
        return std::string("");
    }

    return std::string(std::istreambuf_iterator<char>(ifile), (std::istreambuf_iterator<char>()));

}

#endif