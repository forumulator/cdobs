#include <iostream>
#include <string>

#ifndef CONFIG_H
#define CONFIG_H


extern const std::string kDefaultDbFile;
extern const std::string kDefaultDbPath;
extern const int kMaxTimeLength;
extern const int kMaxObjectSize;
extern const int kSegmentSize;

// Debug output 
extern const std::string kDevNull;

extern std::ostream &dout;                         

#endif