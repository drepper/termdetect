#include "termdetect.hh"

#include <print>


int main()
{
  auto ti = terminal::info::alloc();

  std::println("implementation         = {}", ti->implementation_name());
  std::println("implementation version = {}", ti->implementation_version);
  std::println("emulation              = {}", ti->emulation_name());
  std::print("features               =");
  for (auto f : ti->feature_set)
    std::print(" {}", ti->feature_name(f));
  if (! ti->unknown_features.empty())
    std::print(" {}", ti->unknown_features);
  std::println();
  std::println("raw                    = {}", ti->raw);
  auto [col, row] = ti->get_geometry().value_or(std::make_tuple(80u, 24u));
  std::println("columns                = {}", col);
  std::println("rows                   = {}", row);
  std::println("default foreground     = {:02x}/{:02x}/{:02x}", ti->default_foreground.r, ti->default_foreground.g, ti->default_foreground.b);
  std::println("default background     = {:02x}/{:02x}/{:02x}", ti->default_background.r, ti->default_background.g, ti->default_background.b);
  auto [ccol, crow] = ti->get_cursor_pos().value_or(std::make_tuple(0u, 0u));
  std::println("cursor column          = {}", ccol);
  std::println("cursor row             = {}", crow);
}
