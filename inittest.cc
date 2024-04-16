#include "termdetect.hh"

#include <iostream>


int main()
{
  auto ti = terminal::info::get();

  std::cout << "implementation         = " << ti->implementation_name() << std::endl;
  std::cout << "implementation version = " << ti->implementation_version << std::endl;
  std::cout << "emulation              = " << ti->emulation_name() << std::endl;
  std::cout << "features               =";
  for (auto f : ti->feature_set)
    std::cout << ' ' << ti->feature_name(f);
  if (! ti->unknown_features.empty())
    std::cout << ' ' << ti->unknown_features;
  std::cout << std::endl;
  std::cout << "raw                    = " << ti->raw << std::endl;
}
