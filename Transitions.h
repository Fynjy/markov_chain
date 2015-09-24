#ifndef TRANSITIONS_H_
#define TRANSITIONS_H_

#include <map>
#include <istream>
#include <ostream>
#include <string>

/**
 * Describes an empirical transition table.
 */
class Transitions
{
public:
  std::map<std::string, std::size_t> counts;
  std::size_t n = 0;

public:
  void merge(const Transitions& transitions)
  {
    auto it1 = counts.begin();
    auto it2 = transitions.counts.begin();

    while (it1 != counts.end() && it2 != transitions.counts.end())
    {
      if (it1->first < it2->first)
      {
        ++it1;
      }
      else if (it1->first > it2->first)
      {
        counts.insert(*it2);
        ++it2;
      }
      else
      {
        it1->second += it2->second;
        ++it1;
        ++it2;
      }
    }

    while (it2 != transitions.counts.end())
    {
      counts.insert(*it2);
      ++it2;
    }

    n += transitions.n;
  }

  void merge(const std::string& value)
  {
    counts.emplace(value, 0).first->second += 1;
    ++n;
  }

  Transitions& operator+= (const Transitions& transitions)
  {
    merge(transitions);
    return *this;
  }

  void save(std::ostream& os) const
  {
    os << n << ' ' << counts.size() << ' ';

    for (const auto& c : counts)
    {
      os << c.first << ' ' << c.second << ' ';
    }
  }

  void load(std::istream& is)
  {
    is >> n;

    std::size_t count = 0;
    is >> count;
    std::string key;

    for (std::size_t j = 0; j < count; ++j)
    {
        is >> key;
        is >> counts[key];
    }
  }
};

#endif /* TRANSITIONS_H_ */
