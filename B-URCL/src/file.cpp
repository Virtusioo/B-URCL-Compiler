
#include "file.hpp"

#include <fstream>
#include <sstream>

std::string File::ReadEverything(const std::string& path)
{
    std::ifstream file(path);
    std::stringstream stream; 
    stream << file.rdbuf();
    return stream.str();
}