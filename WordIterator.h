#ifndef WORDITERATOR_H_
#define WORDITERATOR_H_

#include <cstdint>
#include <istream>
#include <iterator>
#include <string>

class WordIterator
:
  public std::iterator<std::input_iterator_tag, std::string>
{
public:
  WordIterator()
  :
    is_(nullptr),
    eof_flag_(true)
  {}

  WordIterator(std::istream& is)
  :
    is_(&is),
    eof_flag_(false)
  {
    operator++ ();
  }

  WordIterator& operator= (const WordIterator&) = default;

  WordIterator& operator++ ()
  {
    str_.clear();

    if (!eof_flag_)
    {
      char c = get();

      while (!eof_flag_ && (c == ' '))
      {
        c = get();
      }

      while (!eof_flag_ && (c != ' '))
      {
        str_ += c;
        c = get();
      }
    }

    return *this;
  }

  WordIterator operator++ (int)
  {
    const WordIterator it = *this;
    ++(*this);
    return it;
  }

  const std::string& operator* () const
  {
    return str_;
  }

  const std::string* operator-> () const
  {
    return &str_;
  }

  bool equal(const WordIterator& b) const
  {
    return (eof_flag_ == b.eof_flag_ && str_ == b.str_);
  }

private:
  std::istream* is_;
  std::string str_;
  bool eof_flag_;

private:
  char get()
  {
    const char c = is_->get();
    eof_flag_ = is_->eof();
    return (std::isalpha(c) ? std::tolower(c) : ' ');
  }
};

inline bool
operator== (const WordIterator& a, const WordIterator& b)
{
  return a.equal(b);
}

inline bool
operator!= (const WordIterator& a, const WordIterator& b)
{
  return !a.equal(b);
}

#endif /* WORDITERATOR_H_ */
