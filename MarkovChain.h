#ifndef MARKOVCHAIN_H_
#define MARKOVCHAIN_H_

#include <istream>
#include <list>
#include <ostream>
#include <random>
#include <string>
#include <unordered_map>

#include "Transitions.h"

extern std::default_random_engine generator;

/**
 * Uses to construct the state of the Markov chain from stream of words.
 */
class MarkovChainState
{
public:
  template<typename Iter>
  MarkovChainState(Iter bi, Iter ei)
  :
    state_(bi, ei),
    order_(state_.size())
  {}

  MarkovChainState(std::size_t order)
  :
    order_(order)
  {}

  bool good() const
  {
    return (state_.size() == order_);
  }

  std::string str() const
  {
    if (state_.size() < order_)
    {
      return std::string();
    }

    std::ostringstream oss;
    auto i = state_.begin();
    oss << *i;
    ++i;

    for (; i != state_.end(); ++i)
    {
      oss << SEPARATOR << *i;
    }

    return oss.str();
  }

  void push(const std::string& word)
  {
    if (state_.size() == order_)
    {
      state_.erase(state_.begin());
    }

    state_.push_back(word);
  }

private:
  static const char SEPARATOR = '_';

  std::list<std::string> state_;
  const std::size_t order_;
};

/**
 * Implements operations:
 * Learn process:
 *  - merge single value to chain;
 *  - merge another chain.
 *
 * Predict process:
 *  - get a random emulated value for the state.
 */
class MarkovChain
{
public:
  void merge(const std::string& key, const std::string& value)
  {
    transitions_.emplace(key, Transitions()).first->second.merge(value);
  }

  void merge(const MarkovChain& chain)
  {
    TransitionsMap merged_transitions;

    auto it1 = transitions_.begin();
    auto it2 = chain.transitions_.begin();

    while (it1 != transitions_.end() && it2 != chain.transitions_.end())
    {
      if (it1->first < it2->first)
      {
        merged_transitions.insert(*it1);
        ++it1;
      }
      else if (it1->first > it2->first)
      {
        merged_transitions.insert(*it2);
        ++it2;
      }
      else
      {
        it1->second.merge(it2->second);
        merged_transitions.insert(*it1);
        ++it1;
        ++it2;
      }
    }

    while (it1 != transitions_.end())
    {
      merged_transitions.insert(*it1);
      ++it1;
    }

    while (it2 != chain.transitions_.end())
    {
      merged_transitions.insert(*it2);
      ++it2;
    }

    transitions_.swap(merged_transitions);
  }

  void save(std::ostream& os) const
  {
    os << transitions_.size() << '\n';

    for (const auto& tr : transitions_)
    {
      os << tr.first << ' ';
      tr.second.save(os);
      os << '\n';
    }
  }

  void load(std::istream& is)
  {
    std::size_t sz = 0;
    is >> sz;

    for (std::size_t i = 0; i < sz; ++i)
    {
      std::string key;
      is >> key;
      transitions_[key].load(is);
    }
  }

  const std::string& predict(const std::string& key) const
  {
    const auto it = transitions_.find(key);

    if (it != transitions_.end() && it->second.n)
    {
      std::uniform_int_distribution<std::size_t> distribution(0, it->second.n - 1);
      const std::size_t r = distribution(generator);
      std::size_t curr = 0;

      for (const auto& c : it->second.counts)
      {
        curr += c.second;

        if (curr >= r)
        {
          return c.first;
        }
      }
    }

    return EMPTY_;
  }

private:
  typedef std::unordered_map<std::string, Transitions> TransitionsMap;

private:
  const std::string EMPTY_;
  TransitionsMap transitions_;
};

#endif /* MARKOVCHAIN_H_ */
