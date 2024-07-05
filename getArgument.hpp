#include <string>

std::string getArgument(int &argc, char *argv[], std::string arg)
{
  for (int i = 1; i < argc; ++i)
  {
    if (std::string(argv[i]) == arg && i + 1 < argc)
    {
      return argv[i + 1];
    }
  }
  return "";
}