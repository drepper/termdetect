#ifndef _TERMDETECT_HH
#define _TERMDETECT_HH 1

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>

#include <unistd.h>


namespace terminal {

  enum struct implementations {
    unknown = 0,
    xterm,
    vte,
    foot,
    terminology,
    contour,
    rxvt,
    mrxvt,
    kitty,
    alacritty,
    st,
    konsole,
    eterm,
    emacsterm,
    qt5,
    ghostty,
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
    sixel, // Sixel graphics
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
    desktopnotification, // OSC777
    decstbm,             // DECSTBM, CSI n1;n1r
    vertlinemarkers,
  };


  struct info {
    virtual ~info() { close(); }

    static const std::shared_ptr<info> alloc(bool close_fd = true);

    static void set_request_delay(int ms);

    implementations implementation = implementations::unknown;
    std::string implementation_version{};
    emulations emulation = emulations::unknown;
    std::set<features> feature_set{};
    std::string unknown_features{};
    std::string raw{};

    std::string implementation_name() const;
    std::string emulation_name() const;
    static std::string feature_name(features feature);

    static std::optional<std::tuple<unsigned, unsigned>> get_geometry(int fd = -1);

    int get_fd() const { return tty_fd; }
    void close()
    {
      if (tty_fd != -1) {
        ::close(tty_fd);
        tty_fd = -1;
      }
    }

  protected:
    // File descriptor for the terminal.
    int tty_fd = -1;
  };

} // namespace terminal

#endif // termdetect.hh
