#ifndef CURLDEVICE_H_
#define CURLDEVICE_H_

#include <cstdio>
#include <cstring>
#include <sstream>
#include <stdexcept>

class Curl
{
public:
  Curl(const std::string& url)
  {
    std::ostringstream oss;
    oss << "curl -s " << url << " -o -";
    handle_ = ::popen(oss.str().c_str(), "r");

    if (!handle_)
    {
      throw std::runtime_error(std::strerror(errno));
    }
  }

  int close()
  {
    int r = 0;
    
    if (handle_)
    {
      r = ::pclose(handle_);
      handle_ = nullptr;
    }
    
    return r;
  }

  ~Curl()
  {
    close();
  }

  FILE* handle()
  {
    return handle_;
  }

private:
  FILE* handle_ = nullptr;
};

#endif /* CURLDEVICE_H_ */
