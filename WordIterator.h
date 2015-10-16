#ifndef WORDITERATOR_H_
#define WORDITERATOR_H_

#include <istream>
#include <iterator>
#include <string>

#include <boost/locale.hpp>

class WordIterator
:
  public std::iterator<std::input_iterator_tag, std::string>
{
public:
  WordIterator()
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
    std::wstring word;

    if (!eof_flag_)
    {
      wchar_t c = get_();

      while (!eof_flag_ && !c)
      {
        c = get_();
      }

      while (!eof_flag_ && c)
      {
        word += c;
        c = get_();
      }
    }

    word_ = boost::locale::conv::utf_to_utf<char>(word.c_str(), word.c_str() + word.length());
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
    return word_;
  }

  const std::string* operator-> () const
  {
    return &word_;
  }

  bool equal(const WordIterator& b) const
  {
    return (eof_flag_ == b.eof_flag_ && word_ == b.word_);
  }

private:
  std::istream* is_ = nullptr;
  const std::locale loc_;
  std::wstring line_;
  std::wstring::const_iterator curr_ = line_.begin();
  std::wstring::const_iterator end_ = line_.begin();
  std::string word_;
  bool eof_flag_ = true;

private:
  wchar_t get_()
  {
    wchar_t c = 0;

    if (curr_ == end_)
    {
      std::string line;
      eof_flag_ = !std::getline(*is_, line);
      line_ = boost::locale::conv::utf_to_utf<wchar_t>(line.c_str(), line.c_str() + line.length());
      curr_ = line_.begin();
      end_ = line_.end();
    }

    if (curr_ != end_)
    {
      c = *curr_;
      ++curr_;
    }

    return (std::isalpha(c, loc_) ? std::tolower(c, loc_) : 0);
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
