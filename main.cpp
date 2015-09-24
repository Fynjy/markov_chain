#include <chrono>
#include <fstream>
#include <future>
#include <iostream>
#include <random>
#include <vector>

#include "CurlStream.h"
#include "MarkovChain.h"
#include "Options.h"
#include "WordIterator.h"

std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());

class UsageException
:
  public std::runtime_error
{
public:
  UsageException(const char* what)
  :
    std::runtime_error(what)
  {}
};

class In
{
public:
  In(const std::string& input)
  {
    if (!input.empty())
    {
      ifs_.open(input);

      if (!ifs_.good())
      {
        std::ostringstream oss;
        oss << "Can't open input file '" << input << "': " << std::strerror(errno);
        throw std::runtime_error(oss.str());
      }

      is_ = &ifs_;
    }
  }

  std::istream& is()
  {
    return *is_;
  }

private:
  std::ifstream ifs_;
  std::istream* is_ = &std::cin;
};

class Out
{
public:
  Out(const std::string& output)
  {
    if (!output.empty())
    {
      ofs_.open(output);

      if (!ofs_.good())
      {
        std::ostringstream oss;
        oss << "Can't open output file '" << output << "': " << std::strerror(errno);
        throw std::runtime_error(oss.str());
      }

      os_ = &ofs_;
    }
  }

  std::ostream& os()
  {
    return *os_;
  }

private:
  std::ofstream ofs_;
  std::ostream* os_ = &std::cout;
};

std::vector<std::string> read_urls(const std::string& input)
{
  std::vector<std::string> urls;
  In in(input);

  std::string url;

  while (std::getline(in.is(), url))
  {
    urls.push_back(url);
  }

  return std::move(urls);
}

std::unique_ptr<MarkovChain> download_file(const std::string& url, std::size_t n)
{
  CurlStream curl_stream(url);

  std::unique_ptr<MarkovChain> markov_chain(new MarkovChain());
  WordIterator it(curl_stream);

  MarkovChainState state(n);

  while (it != WordIterator())
  {
    if (state.good())
    {
      markov_chain->merge(state.str(), *it);
    }

    state.push(*it);
    ++it;
  }

  return markov_chain;
}

void learn(const ProgramOptions& program_options)
{
  const std::vector<std::string> urls(read_urls(program_options.input));
  std::vector<std::future<std::unique_ptr<MarkovChain>>> futures;

  for (const auto& url : urls)
  {
    futures.emplace_back(std::async(std::launch::async, download_file, url, program_options.n));
  }

  std::unique_ptr<MarkovChain> markov_chain;

  for (auto& f : futures)
  {
    std::unique_ptr<MarkovChain> chain = f.get();

    if (!markov_chain)
    {
      markov_chain.swap(chain);
    }
    else
    {
      markov_chain->merge(*chain);
    }
  }

  Out out(program_options.output);

  if (markov_chain)
  {
    markov_chain->save(out.os());
  }
}

void predict(const ProgramOptions& program_options)
{
  if (program_options.model.empty())
  {
    throw UsageException("model file not set");
  }

  MarkovChain chain;

  std::ifstream ifs(program_options.model);

  if (!ifs)
  {
    std::ostringstream oss;
    oss << "Can't open model file '" << program_options.model << "': " << std::strerror(errno);
    throw std::runtime_error(oss.str());
  }

  Out out(program_options.output);
  chain.load(ifs);

  In in(program_options.input);
  MarkovChainState state(WordIterator(in.is()), WordIterator());

  for (std::size_t i = 0; i < program_options.count; ++i)
  {
    const std::string word = chain.predict(state.str());

    if (word.empty())
    {
      break;
    }

    out.os() << word << ' ';
    state.push(word);
  }
}

int main(int argc, char** argv)
{
  const std::string USAGE = "Usage:\n"
    "  mail_test OPTIONS\n"
    "  OPTIONS:\n"
    "    -h, --help - print help\n"
    "    -a, --action - 'learn' of 'predict' mode\n"
    "    -i, --input - input file with list of urls, omitted for std::cin\n"
    "    -o, --output - output file, omitted for std::cout\n"
    "    -n, --order - order of markov chain, default 1\n"
    "    -m, --model - file with model for the predict mode\n"
    "    -c, --count - count of words to predict, default 1\n"
    "Examples:\n"
    "  mail_test -a learn -i files.txt -o markov_chain.txt -n 2\n"
    "  mail_test -a predict -m markov_chain.txt";

  try
  {
    const ProgramOptions program_options = parse_program_options(argc, argv);

    if (program_options.print_help)
    {
      std::cout << USAGE << std::endl;
      return 0;
    }

    if (program_options.action == ProgramOptions::Learn)
    {
      learn(program_options);
    }
    else if (program_options.action == ProgramOptions::Predict)
    {
      const std::chrono::time_point<std::chrono::system_clock> start(std::chrono::system_clock::now());

      predict(program_options);

      const std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
      const long elapsed_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
      std::cout << "\nelapsed time: " << elapsed_seconds << "ms\n";
    }
    else
    {
      throw UsageException("Action not set");
    }
  }
  catch (UsageException& ex)
  {
    std::cerr << ex.what() << std::endl;
    std::cout << USAGE << std::endl;
    return 1;
  }
  catch (std::exception& ex)
  {
    std::cerr << "Error: " << ex.what() << std::endl;
    return 1;
  }

  return 0;
}
