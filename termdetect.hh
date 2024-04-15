#ifndef _TERMDETECT_HH
#define _TERMDETECT_HH 1

#include <memory>
#include <set>
#include <string>


namespace terminal {

  enum struct implementations {
    unknown = 0,
    xterm,
    vte,
    foot,
    terminology,
    contour,
    rxvt,
    kitty,
    alacritty,
    st,
    konsole,
  };


  enum struct emulations {
    unknown = 0,
    vt100,
    vt100avo,
    vt101,
    vt102,
    vt125,
    vt131,
    vt132,
    vt220,
    vt240,
    vt330,
    vt340,
    vt320,
    vt382,
    vt420,
    vt510,
    vt520,
    vt525,
  };


  enum features {
    col132,
    printer,
    regis,
    sixel,
    selerase,
    drcs,
    udk,
    nrcs,
    scs,
    techcharset,
    locatorport,
    stateinterrogation,
    windowing,
    sessions,
    horscroll,
    ansicolors,
    greek,
    turkish,
    textlocator,
    latin2,
    pcterm,
    softkeymap,
    asciiemul,
    capturecontour,
    recteditcontour,
  };


  struct info {
    static const std::shared_ptr<info> get();

    static void set_request_delay(int ms);

    implementations implementation = implementations::unknown;
    std::string implementation_version { };
    emulations emulation = emulations::unknown;
    std::set<features> feature_set { };

    std::string implementation_name() const;
    std::string emulation_name() const;
    static std::string feature_name(features feature);
  };

} // namespace terminal

#endif // termdetect.hh
