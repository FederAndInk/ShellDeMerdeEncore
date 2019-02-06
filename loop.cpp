#include <iostream>
#include <limits>

int main(int argc, char const* argv[])
{
  int i;
  std::cin.exceptions(std::istream::badbit | std::istream::failbit);
  while (1)
  {
    // try
    // {
    //   std::cin >> i;
    //   std::cout << i << "\n";
    // }
    // catch (std::ios_base::failure const& e)
    // {
    //   std::cout << e.what() << "\n";
    //   std::cout << e.code() << "\n";
    //   std::cin.clear();
    //   std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    //   std::cout << "Wrong number\n";
    // }
  }
  return 0;
}
