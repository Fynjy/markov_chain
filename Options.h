#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <stdexcept>
#include <string>
#include <vector>

class ProgramOptions
{
public:
  enum Action
  {
    Learn,
    Predict,
    NotSet
  };

public:
  bool print_help = false;
  Action action = NotSet;
  std::string input;
  std::string output;
  std::string model;
  std::size_t n = 1;
  std::size_t count = 1;
};

template<typename Target, typename Source>
Target lexical_cast(const Source& src)
{
  return src;
}

template<>
std::size_t lexical_cast(const std::string& src)
{
  return std::stoul(src);
}

template<>
ProgramOptions::Action lexical_cast(const std::string& src)
{
  if (src == "learn")
  {
    return ProgramOptions::Learn;
  }
  else if (src == "predict")
  {
    return ProgramOptions::Predict;
  }

  throw std::runtime_error("Invalid action");
}

template<typename Target, typename Source>
class lexical_cast_
{
public:
  Target operator() (const Source& arg)
  {
    return lexical_cast<Target>(arg);
  }
};

template<typename T, typename F = lexical_cast_<T, std::string>>
void extract_value(
  T& result,
  const std::vector<std::string>& args,
  std::vector<std::string>::const_iterator& i,
  F cast = F())
{
  if (++i == args.end())
  {
    throw std::runtime_error("Argument value isn't set");
  }

  result = cast(*i);
}

ProgramOptions parse_program_options(int argc, char** argv)
{
  const std::vector<std::string> args(argv + 1, argv + argc);
  ProgramOptions program_options;

  try
  {
    for (auto i = args.begin(); i != args.end(); ++i)
    {
      const std::string& name = *i;

      if (name == "-h" || name == "--help")
      {
        program_options.print_help = true;
        break;
      }
      else if (name == "-n" || name == "--order")
      {
        extract_value(program_options.n, args, i);
      }
      else if (name == "-i" || name == "--input")
      {
        extract_value(program_options.input, args, i);
      }
      else if (name == "-o" || name == "--output")
      {
        extract_value(program_options.output, args, i);
      }
      else if (name == "-a" || name == "--action")
      {
        extract_value(program_options.action, args, i);
      }
      else if (name == "-m" || name == "--model")
      {
        extract_value(program_options.model, args, i);
      }
      else if (name == "-c" || name == "--count")
      {
        extract_value(program_options.count, args, i);
      }
      else
      {
        throw std::runtime_error("Unknown option");
      }
    }
  }
  catch (std::exception& ex)
  {
    program_options.print_help = true;
  }

  return program_options;
}

#endif /* OPTIONS_H_ */
