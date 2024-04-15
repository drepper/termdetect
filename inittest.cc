#include "termdetect.hh"

#include <iostream>


int main()
{
  auto ti = terminal::info::get();

  std::cout << "implementation         = " << ti->implementation_name() << std::endl;
  std::cout << "implementation version = " << ti->implementation_version << std::endl;
}
