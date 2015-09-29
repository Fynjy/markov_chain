#include <chrono>
#include <fstream>
#include <future>
#include <iostream>
#include <locale>
#include <random>
#include <vector>

#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/program_options.hpp>

#include "CurlDevice.h"
#include "MarkovChain.h"
#include "WordIterator.h"

std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());

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

  for (std::string url; std::getline(in.is(), url);)
  {
    urls.push_back(url);
  }

  return urls;
}

std::unique_ptr<MarkovChain> learn_by_one_file(const std::string& url, std::size_t n)
{
  Curl curl(url);
  typedef boost::iostreams::stream<boost::iostreams::file_descriptor_source> CurlStream;
  CurlStream curl_stream(fileno(curl.handle()), boost::iostreams::close_handle);

  WordIterator it(curl_stream);

  std::unique_ptr<MarkovChain> markov_chain(new MarkovChain());
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

  const auto curl_exit_code = curl.close();

  if (curl_exit_code)
  {
    std::ostringstream oss;
    oss << "curl returned error: " << curl_exit_code << " for '" << url << "'";
    throw std::runtime_error(oss.str());
  }

  return markov_chain;
}

void learn(
  const std::string& input,
  const std::string& output,
  std::size_t n)
{
  const std::vector<std::string> urls(read_urls(input));
  std::vector<std::future<std::unique_ptr<MarkovChain>>> futures;

  for (const auto& url : urls)
  {
    futures.emplace_back(std::async(std::launch::async, learn_by_one_file, url, n));
  }

  std::unique_ptr<MarkovChain> markov_chain;

  for (auto& f : futures)
  {
    try
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
    catch (std::exception& ex)
    {
      std::cerr << "Got error: " << ex.what() << std::endl;
    }
  }

  Out out(output);

  if (markov_chain)
  {
    markov_chain->save(out.os());
  }
}

void predict(
  const std::string& input,
  const std::string& output,
  const std::string& model,
  std::size_t count)
{
  MarkovChain chain;
  std::ifstream ifs(model);

  if (!ifs)
  {
    std::ostringstream oss;
    oss << "Can't open model file '" << model << "': " << std::strerror(errno);
    throw std::runtime_error(oss.str());
  }

  Out out(output);
  chain.load(ifs);

  In in(input);
  MarkovChainState state(WordIterator(in.is()), WordIterator());

  for (std::size_t i = 0; i < count; ++i)
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

void print_help(const boost::program_options::options_description& options_desc)
{
  options_desc.print(std::cout);
  std::cout <<
    "Examples:\n"
    "  markov_chain -a learn -i files.txt -o markov_chain.txt -n 2\n"
    "  markov_chain -a predict -m markov_chain.txt\n";
}

enum class Action
{
  Learn,
  Predict
};

std::istream& operator>>(std::istream& in, Action& action)
{
  std::string token;
  in >> token;

  if (token == "learn")
  {
    action = Action::Learn;
  }
  else if (token == "predict")
  {
    action = Action::Predict;
  }
  else
  {
    throw boost::program_options::validation_error(
      boost::program_options::validation_error::invalid_option_value,
      "action",
      token);
  }

  return in;
}

int main(int argc, char** argv)
{
  std::locale::global(std::locale(""));

  Action action;
  std::string input;
  std::string output;
  std::string model;
  std::size_t order;
  std::size_t count;

  boost::program_options::options_description options_desc("Markov chain utility.\n"
    "Usage:\n"
    "  markov_chain OPTIONS\n"
    "OPTIONS");
  options_desc.add_options()
    ("help,h", "print help")
    ("action,a", boost::program_options::value<Action>(&action)->required(), "'learn' or 'predict' mode")
    ("input,i", boost::program_options::value<std::string>(&input), "input file with list of urls, omitted for std::cin")
    ("output,o", boost::program_options::value<std::string>(&output), "output file, omitted for std::cout")
    ("order,n", boost::program_options::value<std::size_t>(&order)->default_value(1), "order of markov chain, default 1")
    ("model,m", boost::program_options::value<std::string>(&model), "file with model for the predict mode")
    ("count,c", boost::program_options::value<std::size_t>(&count)->default_value(1), "count of words to predict, default 1");

  boost::program_options::variables_map program_options;

  try
  {
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, options_desc), program_options);

    if (program_options.count("help"))
    {
      print_help(options_desc);
      return 0;
    }

    boost::program_options::notify(program_options);
  }
  catch (std::exception& ex)
  {
    std::cerr << "Argument error: " << ex.what() << std::endl;
    print_help(options_desc);
    return 1;
  }

  try
  {
    if (action == Action::Learn)
    {
      learn(input, output, order);
    }
    else if (action == Action::Predict)
    {
      if (model.empty())
      {
        throw boost::program_options::validation_error(
          boost::program_options::validation_error::invalid_option_value,
          "model");
      }

      const std::chrono::time_point<std::chrono::system_clock> start(std::chrono::system_clock::now());

      predict(input, output, model, count);

      const std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
      const long elapsed_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
      std::cout << "\nelapsed time: " << elapsed_seconds << "ms\n";
    }
  }
  catch (boost::program_options::validation_error& ex)
  {
    std::cerr << "Argument error: " << ex.what() << std::endl;
    print_help(options_desc);
    return 1;
  }
  catch (std::exception& ex)
  {
    std::cerr << "Error: " << ex.what() << std::endl;
    return 1;
  }

  return 0;
}
