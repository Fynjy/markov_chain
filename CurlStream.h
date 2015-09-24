#ifndef CURLSTREAM_H_
#define CURLSTREAM_H_

#include <cstdio>
#include <cstring>
#include <sstream>
#include <stdexcept>

/**
 * Implements std::istream interface for the reading a text file from curl utility.
 */
class CurlStream
:
  public std::istream
{
public:
  CurlStream(const std::string& url)
  :
    std::ios(&streambuf_),
    std::istream(&streambuf_),
    curl_pipe_(nullptr)
  {
    std::ostringstream oss;
    oss << "curl -s " << url << " -o -";
    curl_pipe_ = ::popen(oss.str().c_str(), "r");

    if (!curl_pipe_)
    {
      throw std::runtime_error(std::strerror(errno));
    }

    streambuf_.curl_pipe = curl_pipe_;
  }

  CurlStream(const CurlStream&) = delete;
  CurlStream& operator= (const CurlStream&) = delete;

  virtual ~CurlStream()
  {
    pclose(curl_pipe_);
  }

private:
  class Streambuf
  :
    public std::streambuf
  {
  public:
    Streambuf()
    :
      curl_pipe(nullptr)
    {}

    virtual ~Streambuf()
    {}

    virtual std::streamsize showmanyc()
    {
      return (std::feof(curl_pipe) ? 0 : 1);
    }

    virtual int underflow()
    {
      if (this->gptr() == this->egptr())
      {
        const std::size_t n = std::fread(buffer_, 1, sizeof(buffer_), curl_pipe);
        setg(buffer_, buffer_, buffer_ + n);
      }

      return gptr() == egptr()
         ? std::char_traits<char>::eof()
         : std::char_traits<char>::to_int_type(*gptr());
    }

  public:
    char buffer_[4096];
    FILE* curl_pipe;
  };

private:
  FILE* curl_pipe_;
  Streambuf streambuf_;
};

#endif /* CURLSTREAM_H_ */
