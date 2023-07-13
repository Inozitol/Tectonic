
#include "utils.h"

namespace Utils{
    bool readFile(const char* filename, std::string& content){
        std::ifstream f(filename);
        bool ret = false;

        if(f.is_open()){
            std::string line;
            while(std::getline(f, line)){
                content.append(line);
                content.append("\n");
            }
            f.close();
            ret = true;
        }
        return ret;
    }
}