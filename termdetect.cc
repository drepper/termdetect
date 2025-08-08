#include "termdetect.hh"

#include <cassert>
#include <cctype>
#include <csignal>
#include <cstring>
#include <format>
#include <map>
#include <optional>
#include <string_view>
#include <system_error>
#include <utility>

#include <fcntl.h>
#include <paths.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>


namespace terminal {

  namespace {

    // Special string to indicate that the command never was issued.
    constexpr auto not_issued = "<NOT ISSUED>";
    constexpr auto no_reply = "<NO REPLY>";


    struct info_impl final : info {
      info_impl(bool close_fd);

      std::string da1_reply = not_issued;
      std::string da2_reply = not_issued;
      std::string_view da2_reply_tail{};
      std::string da3_reply = not_issued;
      std::string q_reply = not_issued;
      std::string tn_reply = not_issued;
      std::string osc702_reply = not_issued;

      bool da2_alarmed = false;

      // Version number derived from DA2 reply.
      unsigned vn = 0;

      // Intermediate result, the emulation announced by DA2.
      emulations da2_emulation = emulations::unknown;


      void make_da1_request(int fd);
      bool make_da2_request(int fd);
      void make_da3_request(int fd);
      void make_tn_request(int fd);
      void make_q_request(int fd);
      void make_osc702_request(int fd);

      void parse_da1();
      void parse_da2();

      bool is_st() const;
      bool is_alacritty() const;
      bool is_vte() const;
      bool is_not_vte() const;
      bool is_terminology() const;
      bool is_contour() const;
      bool is_xterm() const;
      bool is_rxvt() const;
      bool is_mrxvt() const;
      bool is_kitty() const;
      bool is_konsole() const;
      bool is_eterm() const;
      bool is_qt5() const;
      bool is_ghostty() const;
    };


// Escape sequences.
#define CSI "\e["
#define OSC "\e]"
#define DCS "\eP"
#define ST "\e\\"

#define Q_REQUEST CSI ">q"
#define Q_REPLY_PREFIX DCS ">|"
#define Q_REPLY_SUFFIX ST

#define TN_REQUEST DCS "+q544e" ST
#define TN_REPLY_PREFIX DCS "1+r544e="
#define TN_REPLY_SUFFIX ST

#define OSC702_REQUEST OSC "702;?" ST
#define OSC702_REPLY_PREFIX OSC "702;"
#define OSC702_REPLY_SUFFIX "\e"

#define DA1_REQUEST CSI "c"
#define DA1_REPLY_PREFIX CSI "?"
#define DA1_REPLY_SUFFIX "c"

#define DA2_REQUEST CSI ">c"
#define DA2_REPLY_PREFIX CSI ">"
#define DA2_REPLY_SUFFIX "c"

#define DA3_REQUEST CSI "=c"
#define DA3_REPLY_PREFIX DCS "!|"
#define DA3_REPLY_SUFFIX ST


    const std::array known_emulations{
      std::make_tuple("0;", emulations::vt100),
      std::make_tuple("1;0", emulations::vt101),
      std::make_tuple("1;2", emulations::vt100avo),
      std::make_tuple("2;", emulations::vt240),
      std::make_tuple("4;6", emulations::vt132),
      std::make_tuple("6;", emulations::vt102),
      std::make_tuple("7;", emulations::vt131),
      std::make_tuple("18;", emulations::vt330),
      std::make_tuple("12;", emulations::vt125),
      std::make_tuple("19;", emulations::vt340),
      std::make_tuple("24;", emulations::vt320),
      std::make_tuple("32;", emulations::vt382),
      std::make_tuple("41;", emulations::vt420),
      std::make_tuple("61;", emulations::vt510),
      std::make_tuple("62;", emulations::vt220),
      std::make_tuple("63;", emulations::vt320),
      std::make_tuple("64;", emulations::vt520),
      std::make_tuple("65;", emulations::vt525),
      // These entries are present for rxvt which stores 'U' or 'R' in the first number of the DA2 reply
      std::make_tuple("85;", emulations::unknown),
      std::make_tuple("82;", emulations::unknown),
    };


    const std::map<unsigned, features> known_features{
      {1, features::col132},
      {2, features::printer},
      {3, features::regis},
      {4, features::sixel},
      {6, features::selerase},
      {7, features::drcs},
      {8, features::udk},
      {9, features::nrcs},
      {12, features::scs},
      {15, features::techcharset},
      {16, features::locatorport},
      {17, features::stateinterrogation},
      {18, features::windowing},
      {19, features::sessions},
      {21, features::horscroll},
      {22, features::ansicolors},
      {23, features::greek},
      {24, features::turkish},
      {28, features::recteditcontour},
      {29, features::textlocator},
      {42, features::latin2},
      {44, features::pcterm},
      {45, features::softkeymap},
      {46, features::asciiemul},
      {314, features::capturecontour},
    };


    // Timeout for individual requests in case the emulator does not answer.
    std::optional<int> request_delay;

    int get_default_request_delay()
    {
      // So far we only handle remote sessions special.  Recognize them by the DISPLAY envvar.
      auto display = std::getenv("DISPLAY");

      if (display != nullptr && display[0] != '\0' && display[0] != ':')
        // This is likely a remote sessions.  Give it more time.
        return 500;

      // Local emulation.  Should be really fast.
      return 100;
    }


    // Issue the request to the termi
    bool make_request(std::string& res, int fd, const char* request, const char* reply_prefix, const char* reply_suffix)
    {
      bool wok = false;
      bool rok = false;

      termios t_old;
      ::tcgetattr(fd, &t_old);
      termios t_new = t_old;
      ::cfmakeraw(&t_new);
      if (::tcsetattr(fd, TCSAFLUSH, &t_new) < 0) [[unlikely]]
        // This might indicate the process is running in the background and has no access to the terminal.
        return false;

      if (::write(fd, request, strlen(request)) == ssize_t(strlen(request))) [[likely]] {
        wok = true;

        pollfd pfds[1]{{fd, POLLIN, 0}};
        auto n = ::poll(pfds, 1, *request_delay);
        rok = n != 0;
        if (rok) {
          char buf[4096];
          auto nread = ::read(fd, buf, sizeof(buf));

          rok = nread > 0;
          if (rok) [[likely]] {
            res.clear();
            for (decltype(nread) i = 0; i < nread; ++i)
              res.push_back(buf[i]);
          }
        } else
          res = no_reply;
      }

      ::tcsetattr(fd, TCSAFLUSH, &t_old);

      if (wok && rok) {
        // Strip out the expected prefix and suffix.
        if (res.size() > strlen(reply_prefix) + strlen(reply_suffix) && res.starts_with(reply_prefix) && res.ends_with(reply_suffix)) [[likely]]
          res = res.substr(strlen(reply_prefix), res.size() - (strlen(reply_suffix)) - (strlen(reply_prefix)));
      }

      return ! rok;
    }


    void info_impl::make_da1_request(int fd)
    {
      (void) make_request(da1_reply, fd, DA1_REQUEST, DA1_REPLY_PREFIX, DA1_REPLY_SUFFIX);

      parse_da1();
    }


    void info_impl::parse_da1()
    {
      std::string_view sv = da1_reply;

      // Remove the terminal prefix from DA1 reply.  Some emulators (e.g., Terminology)
      // are inconsistent in the announcement of the terminal type in the DA2 and DA1
      // replies.  Give preference to the former.
      for (const auto& e : known_emulations)
        if (sv.starts_with(std::get<const char*>(e))) {
          if (emulation == emulations::unknown || emulation == emulations::vt100)
            emulation = std::get<emulations>(e);
          sv.remove_prefix(strlen(std::get<const char*>(e)));
          break;
        } else if (sv.size() == strlen(std::get<const char*>(e)) - 1 && strncmp(sv.data(), std::get<const char*>(e), sv.size()) == 0) {
          // Some terminals just announce the emulation and therefore do not have the trailing semicolon
          // present in the known_emulation table.
          if (emulation == emulations::unknown)
            emulation = std::get<emulations>(e);
          sv.remove_prefix(sv.size());
          break;
        }

      while (! sv.empty()) {
        unsigned code;
        auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), code);
        if (ec != std::errc{} || ! (ptr == sv.data() + sv.size() || ptr[0] == ';'))
          break;
        if (ptr[0] != '\0')
          ++ptr;
        if (auto feature = known_features.find(code); feature != known_features.end())
          feature_set.insert(feature->second);
        else
          unknown_features += std::string(sv.data(), ptr - sv.data());
        sv.remove_prefix(ptr - sv.data());
      }

      if (unknown_features.ends_with(";"))
        unknown_features.erase(unknown_features.size() - 1);
    }


    bool info_impl::make_da2_request(int fd)
    {
      bool rfailed = make_request(da2_reply, fd, DA2_REQUEST, DA2_REPLY_PREFIX, DA2_REPLY_SUFFIX);

      parse_da2();

      return rfailed;
    }


    void info_impl::parse_da2()
    {
      std::string_view sv = da2_reply;

      bool matched = false;
      for (const auto& e : known_emulations)
        if (sv.starts_with(std::get<const char*>(e))) {
          da2_emulation = emulation = std::get<emulations>(e);
          sv.remove_prefix(strlen(std::get<const char*>(e)));
          matched = true;
          break;
        }

      if (! matched && sv.starts_with("1;"))
        // This is the non-descript answer of VT220 etc which refer to DA1 for the real answer.
        // Only the rest of the information is important.
        sv.remove_prefix(2);

      // The DA2 reply consists of the version information.  Usually separated by semicolons.
      auto skip = sv.find(';');
      auto svend = sv.data() + (skip == std::string_view::npos ? sv.size() : skip);
      auto [endp, ec] = std::from_chars(sv.data(), svend, vn);
      if (ec == std::errc{}) {
        if (endp < svend && *endp == '.') {
          do {
            unsigned vn2;
            auto [endp2, ec2] = std::from_chars(endp + 1, svend, vn2);
            endp = endp2;
            ec = ec2;
          } while (ec == std::errc{} && endp < svend && *endp == '.');
          if (ec == std::errc{} && (endp == svend || *endp == ';'))
            implementation_version = std::string(sv.data(), endp);

          sv.remove_prefix(endp - sv.data());
          if (sv == ";0")
            return;
        } else
          sv.remove_prefix(endp - sv.data());

        da2_reply_tail = sv;
        if (sv[0] == ';') {
          unsigned vn2;
          auto [endp2, ec2] = std::from_chars(sv.data() + 1, sv.data() + sv.size(), vn2);

          // Terminal emulators do not agree how to encode the version number.  Some encode all the data in the number
          // after the first semicolon.  Others use the second semicolon as a decimal point.  Yet others use floating-point
          // notation.  Try to guess.
          if (ec2 == std::errc{} && vn < 10000 && vn2 != 0 && vn2 < 100) {
            vn = vn * 100 + vn2;
            sv.remove_prefix(endp2 - sv.data());
            da2_reply_tail = sv;
          }
          // Many emulators add ";0" at the end.  Ignore it.
          if (da2_reply_tail == ";0")
            da2_reply_tail = "";
        }
      }
    }


    void info_impl::make_da3_request(int fd)
    {
      (void) make_request(da3_reply, fd, DA3_REQUEST, DA3_REPLY_PREFIX, DA3_REPLY_SUFFIX);
    }

    void info_impl::make_tn_request(int fd)
    {
      (void) make_request(tn_reply, fd, TN_REQUEST, TN_REPLY_PREFIX, TN_REPLY_SUFFIX);

      // Recognize the error code.
      if (tn_reply.starts_with(DCS "0"))
        tn_reply = "???";
    }

    void info_impl::make_q_request(int fd)
    {
      (void) make_request(q_reply, fd, Q_REQUEST, Q_REPLY_PREFIX, Q_REPLY_SUFFIX);
    }

    void info_impl::make_osc702_request(int fd)
    {
      (void) make_request(osc702_reply, fd, OSC702_REQUEST, OSC702_REPLY_PREFIX, OSC702_REPLY_SUFFIX);
    }


    bool info_impl::is_st() const
    {
      if (implementation != implementations::unknown)
        return implementation == implementations::st;

      return da1_reply == "6" && da2_alarmed;
    }


    bool info_impl::is_alacritty() const
    {
      if (implementation != implementations::unknown)
        return implementation == implementations::alacritty;

      if (da2_reply.size() < 5)
        return false;

      unsigned val;
      auto [endp, ec] = std::from_chars(da2_reply.data() + 2, da2_reply.data() + da2_reply.size(), val, 10);
      return ec == std::errc() && (da2_reply.data() + da2_reply.size() - endp) == 2 && da1_reply == "6" && da2_reply.starts_with("0;") && da2_reply.ends_with(";1");
    }


    bool info_impl::is_vte() const
    {
      if (implementation != implementations::unknown)
        return implementation == implementations::vte;

      return da3_reply == "7E565445";
    }


    // This function is not the inverse of is_vte.  It returns true only if given the limited information collected to that
    // point it VTE can definitely be excluded.
    bool info_impl::is_not_vte() const
    {
      if (implementation != implementations::unknown)
        return implementation != implementations::vte;

      // VTE always (so far) sets the terminal ID to 65.
      return ! da1_reply.starts_with("65;") || ! da2_reply.starts_with("65;") || feature_set.contains(features::capturecontour);
    }


    bool info_impl::is_rxvt() const
    {
      if (implementation != implementations::unknown)
        return implementation == implementations::rxvt;

      return da2_reply.starts_with("85;") || da2_reply.starts_with("82;");
    }


    bool info_impl::is_mrxvt() const
    {
      if (implementation != implementations::unknown)
        return implementation == implementations::mrxvt;

      return ! implementation_version.empty() && (da2_reply.starts_with("85;") || da2_reply.starts_with("82;"));
    }


    bool info_impl::is_kitty() const
    {
      if (implementation != implementations::unknown)
        return implementation == implementations::kitty;

      return tn_reply == "787465726d2d6b69747479";
    }


    bool info_impl::is_xterm() const
    {
      if (implementation != implementations::unknown)
        return implementation == implementations::xterm;

      return q_reply.starts_with("XTerm");
    }


    bool info_impl::is_contour() const
    {
      if (implementation != implementations::unknown)
        return implementation == implementations::contour;

      return q_reply.starts_with("contour");
    }


    bool info_impl::is_terminology() const
    {
      if (implementation != implementations::unknown)
        return implementation == implementations::terminology;

      return q_reply.starts_with("terminology");
    }


    bool info_impl::is_konsole() const
    {
      if (implementation != implementations::unknown)
        return implementation == implementations::konsole;

      return q_reply.starts_with("Konsole");
    }


    bool info_impl::is_eterm() const
    {
      return implementation == implementations::eterm;
    }


    bool info_impl::is_qt5() const
    {
      if (implementation != implementations::unknown)
        return implementation == implementations::qt5;

      return da2_emulation == emulations::vt100 && emulation == emulations::vt100avo;
    }


    bool info_impl::is_ghostty() const
    {
      if (implementation != implementations::unknown)
        return implementation == implementations::ghostty;

      return q_reply.starts_with("ghostty");
    }

  } // anonymous namespace

  info_impl::info_impl(bool close_fd) : info()
  {
    struct ::sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SIG_IGN;
    struct ::sigaction sa_out_old;
    ::sigaction(SIGTTOU, &sa, &sa_out_old);
    struct ::sigaction sa_in_old;
    ::sigaction(SIGTTIN, &sa, &sa_in_old);

    if (! request_delay.has_value())
      request_delay = get_default_request_delay();

    tty_fd = ::open(_PATH_TTY, O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (tty_fd != -1) [[likely]] {
      // The DA1 and DA2 requests seem to be universally implemented.  Note that the order of the calls is required.
      // Information about the terminal emulation from DA2 is more reliable.
      da2_alarmed = make_da2_request(tty_fd);
      make_da1_request(tty_fd);

      // The order to make requests without stalling/timing out in the reads is complicated.
      // - alacritty does not handle CSI > q, DCS + q T N, DA3, nor OSC702
      // - VTE does not understand CSI > q but that is the ultimate informer for xterm.
      // - alternatively DA3 can be used as a weak signal for xterm but DA3 does not work for kitty nor rxvt
      // - kitty needs the CSI + q T N request but this also does not work for VTE
      // - Eterm and Emacs Term do not handle *anything*
      // We break the cycle by not issuing DA3 early and avoid if the CSI > q and DCS + q T N requests if
      // the terminal could possibly be VTE based.  Once we can exclude rxvt and kitty we can issue DA3
      // to be sure.
      // +----------------+-----------+---------------+-----------+-----------+-----------+------------+
      // | Name           |    DA1    |      DA2      |    DA3    |     Q     |    TN     |   OSC702   |
      // +----------------+-----------+---------------+-----------+-----------+-----------+------------+
      // |                |           |               |           |           |           |            |
      // | Alacritty      | 6         | 0;VERS;1      | no resp   | no resp   | no resp   |            |
      // | Contour        | a lot     | 65;VERS;0     | C0000000  | contour * | ""        |            |
      // | EmacsTerm      | no resp   | no resp       | no resp   | no resp   | echo      |            |
      // | ETerm          | no resp   | no resp       | no resp   | no resp   | no resp   |            |
      // | Foot           | 62;4;22   | 1;VERS;0      | 464f4f54  | foot(*    | 666F6F74  |            |
      // | Kitty          | 62;       | 1;4000;29     | no resp   | kitty(*   | 78746572* |            |
      // | Konsole        | 62;1;4    | 1;VERS;0      | 7E4B4445  | Konsole*  | no esp    |            |
      // | rxvt           | 1;2       | 85;VERS;0     | no resp   | no resp   | no resp   | rxvt*      |
      // | mrxvt          | 1;2       | 82;V1.V2.V3;0 | no resp   | no resp   | no resp   |            |
      // | QT5            | 1;2       | 0;VERS;0      | no resp   | no resp   | echo      |            |
      // | ST             | 6         | no resp       | no resp   | no resp   | no resp   |            |
      // | Terminology    | a lot     | 61;VERS;0     | 7E7E5459  | terminolo*| no resp   |            |
      // | VTE            | 65;1;9    | 65;VERS;1     | 7E565445  | no resp   | no resp   |            |
      // | XTerm          | a lot     | 41;VERS;0     | 00000000  | XTerm(*   | no resp   |            |
      // |                |           |               |           |           |           |            |
      // +----------------+-----------+---------------+-----------+-----------+-----------+------------+
      //
      // Other terminals use the same engines:
      // VTE: gnome-console, mate-terminal, lxterminal, xfce4-terminal, roxterm, tilix
      // QT5: deepin, qterminal

      // We are desperate when checking for eterm and emacs term.  They do not handle any request and others than
      // Any request other than DA1 and DA2 must be avoided (eterm does not trip over DA3 but still).
      if (da1_reply == no_reply && da2_reply == no_reply) {
        if (auto term = ::getenv("TERM"); term != nullptr && strncmp(term, "eterm", 5) == 0) {
          implementation = implementations::emacsterm;
          // Assume the most basic.
          emulation = emulations::vt100;
        } else if (term != nullptr && strcmp(term, "Eterm") == 0) {
          implementation = implementations::eterm;
          // Assume the most basic.
          emulation = emulations::vt100;
        }
      }

      // Detecting ST is, with the currently used requests, not possible without a delay.  It only
      // responds to DA1 and its answer to that request (= "6") is not unique (same as Alacritty).
      // Unless there is something else that can be done the best we can do is to limit the number
      // of delays to one by determining the emulator type based on the DA2 request timeout.
      if (! is_st() && ! is_alacritty() && ! is_eterm() && ! is_qt5()) {
        if (is_not_vte() && ! is_rxvt()) {
          make_q_request(tty_fd);

          // Do not issue the TN request for rxvt and xterm.  We use the DA2 or Q reply for this.  It might not be conclusive but
          // no counterexamples are known so far.
          if (! is_rxvt() && ! is_xterm() && ! is_contour() && ! is_terminology() && ! is_konsole())
            make_tn_request(tty_fd);
        }

        if (! is_kitty() && ! is_rxvt()) {
          make_da3_request(tty_fd);

          // Reconsider whether to issue the Q and TN requests.
          if (is_not_vte() && ! is_vte() && ! is_xterm() && ! is_konsole()) {
            make_q_request(tty_fd);
            if (! is_terminology() && ! is_ghostty())
              make_tn_request(tty_fd);
          }
        }

        // Do not issue the DA3 and OSC702 requests for the kitty terminal emulator, it does not handle them so far.
        // We also do not do this for mrxvt, it does not handle the DA3 request nor does it provide any answer
        // to OSC702, just an empty string.
        if (! is_kitty() && ! is_mrxvt()) {
          // Do not issue the DA3 request for rxvt.
          if (! is_rxvt() && ! is_ghostty())
            make_da3_request(tty_fd);

          if (da3_reply == not_issued) {
            make_osc702_request(tty_fd);

            // The code below assumes that we can identify rxvt via the OSC702 reply.
            assert(! is_rxvt() || osc702_reply.starts_with("rxvt"));
          }
        }
      }

      if (close_fd)
        ::close(tty_fd);

      raw = std::format("TN={}, DA1={}, DA2={}, DA3={}, OSC702={}, Q={}", tn_reply, da1_reply, da2_reply, da3_reply, osc702_reply, q_reply);

      // We are ready to determine the implementation.
      if (is_st())
        implementation = implementations::st;
      else if (da3_reply == "7E565445")
        implementation = implementations::vte;
      else if (da3_reply == "464f4f54")
        implementation = implementations::foot;
      else if (is_terminology())
        implementation = implementations::terminology;
      else if (is_contour())
        implementation = implementations::contour;
      else if (is_xterm())
        implementation = implementations::xterm;
      else if (is_mrxvt())
        implementation = implementations::mrxvt;
      else if (osc702_reply.starts_with("rxvt"))
        implementation = implementations::rxvt;
      else if (is_kitty())
        implementation = implementations::kitty;
      else if (is_alacritty())
        implementation = implementations::alacritty;
      else if (is_konsole())
        implementation = implementations::konsole;
      else if (is_qt5())
        implementation = implementations::qt5;
      else if (is_ghostty())
        implementation = implementations::ghostty;

      // Determine the implementation version.
      if (implementation_version.empty()) {
        if (is_terminology()) {
          // Terminology does not fill DA2 replies with appropriate version information.  Use the CSI > q reply.
          assert(! q_reply.empty());
          implementation_version = q_reply.substr(12);
        } else if (is_konsole()) {
          // Konsole does not fill DA2 replies with appropriate version information.  Use the CSI > q reply.
          assert(! q_reply.empty());
          implementation_version = q_reply.substr(8);
        } else if (is_kitty() && q_reply.starts_with("kitty(") && q_reply.ends_with(")") && q_reply.size() > 7)
          implementation_version = q_reply.substr(6, q_reply.size() - 7);
        else {
          if (is_rxvt())
            // rxvt encodes the version number as Mm (major/minor) in two digits.
            vn = (vn / 10) * 10000 + (vn % 10) * 100;
          else if (is_kitty() && vn > 400000)
            // For some reason kitty adds 4000 to the first number.
            vn = (vn - 400000) * 100;
          else if (is_xterm())
            // XTerm version numbers are > 100 and there is not even a minor version number.
            vn *= 10000;
          else if (is_vte())
            // Ignore the last number after all.
            vn /= 100;

          // Not all implementations provide a patch number.
          if (vn % 10000 == 0)
            implementation_version = std::format("{}", vn / 10000);
          else if (vn % 100 == 0)
            implementation_version = std::format("{}.{}", vn / 10000, (vn / 100) % 100);
          else
            implementation_version = std::format("{}.{}.{}", vn / 10000, (vn / 100) % 100, vn % 100);
        }
      }

      if (is_alacritty() && emulation == emulations::vt100) {
        std::string da1_extended = da1_reply + ";";
        for (const auto& e : known_emulations)
          if (da1_extended.starts_with(std::get<const char*>(e))) {
            emulation = std::get<emulations>(e);
            break;
          }
      }

      // Add features which are not discovered automatically.
      if (is_kitty())
        // OSC777 supported.
        feature_set.insert(features::desktopnotification);
      if (is_contour())
        // Vertical line markers.
        feature_set.insert(features::vertlinemarkers);

      // Unless demonstrated otherwise, assume that the terminal has DECSTBM support.
      feature_set.insert(features::decstbm);
    }

    ::sigaction(SIGTTOU, &sa_out_old, nullptr);
    ::sigaction(SIGTTIN, &sa_in_old, nullptr);
  }


  const std::shared_ptr<info> info::alloc(bool close_fd)
  {
    return std::make_shared<info_impl>(close_fd);
  }


  void info::set_request_delay(int ms)
  {
    request_delay = ms;
  }


  std::string info::implementation_name() const
  {
    auto real_this = reinterpret_cast<const info_impl*>(this);

    std::string res;

    switch (implementation) {
    case implementations::unknown:
      res = "unknown";
      break;
    case implementations::vte:
      res = "VTE-based";
      break;
    case implementations::foot:
      res = "Foot";
      break;
    case implementations::terminology:
      res = "Terminology";
      break;
    case implementations::contour:
      res = "Contour";
      break;
    case implementations::xterm:
      res = "XTerm";
      break;
    case implementations::rxvt:
      res = "rxvt";
      break;
    case implementations::mrxvt:
      res = "mrxvt";
      break;
    case implementations::kitty:
      res = "Kitty";
      break;
    case implementations::alacritty:
      res = "Alacritty";
      break;
    case implementations::st:
      res = "st";
      break;
    case implementations::konsole:
      res = "Konsole";
      break;
    case implementations::eterm:
      res = "ETerm";
      break;
    case implementations::emacsterm:
      res = "Emacs Term";
      break;
    case implementations::qt5:
      res = "Qt5";
      break;
    case implementations::ghostty:
      res = "ghostty";
      break;
    default:
      for (auto b : real_this->da3_reply)
        if (isprint(b))
          res += b;
        else
          std::format_to(std::back_inserter(res), "\\x{:02x}", b);
      break;
    }

    return res;
  }


  std::string info::emulation_name() const
  {
    auto real_this = reinterpret_cast<const info_impl*>(this);

    std::string res;

    switch (emulation) {
    case emulations::vt100:
      res = "VT100";
      break;
    case emulations::vt100avo:
      res = "VT100 w/ Advanced Video Option";
      break;
    case emulations::vt101:
      res = "VT101";
      break;
    case emulations::vt102:
      res = "VT102";
      break;
    case emulations::vt125:
      res = "VT125";
      break;
    case emulations::vt131:
      res = "VT131";
      break;
    case emulations::vt132:
      res = "VT132";
      break;
    case emulations::vt220:
      res = "VT220";
      break;
    case emulations::vt240:
      res = "VT240";
      break;
    case emulations::vt330:
      res = "VT330";
      break;
    case emulations::vt340:
      res = "VT340";
      break;
    case emulations::vt320:
      res = "VT320";
      break;
    case emulations::vt382:
      res = "VT382";
      break;
    case emulations::vt420:
      res = "VT420";
      break;
    case emulations::vt510:
      res = "VT510";
      break;
    case emulations::vt520:
      res = "VT520";
      break;
    case emulations::vt525:
      res = "VT525";
      break;
    default:
      res = "<unknown terminal>";
    }

    for (auto b : real_this->da2_reply_tail)
      if (isprint(b))
        res += b;
      else
        std::format_to(std::back_inserter(res), " \\x{:02x}", b);

    return res;
  }


  std::string info::feature_name(features feature)
  {
    switch (feature) {
    case features::col132:
      return "132cols";
    case features::printer:
      return "printer";
    case features::regis:
      return "regis";
    case features::sixel:
      return "sixel";
    case features::selerase:
      return "selerase";
    case features::drcs:
      return "drcs";
    case features::udk:
      return "udk";
    case features::nrcs:
      return "nrcs";
    case features::scs:
      return "scs";
    case features::techcharset:
      return "techcharset";
    case features::locatorport:
      return "locatorport";
    case features::stateinterrogation:
      return "stateinterrogation";
    case features::windowing:
      return "windowing";
    case features::sessions:
      return "sessions";
    case features::horscroll:
      return "horscoll";
    case features::ansicolors:
      return "ansicolors";
    case features::greek:
      return "greek";
    case features::turkish:
      return "turkish";
    case features::textlocator:
      return "textlocator";
    case features::latin2:
      return "latin2";
    case features::pcterm:
      return "pcterm";
    case features::softkeymap:
      return "softkeymap";
    case features::asciiemul:
      return "asciiemul";
    case features::capturecontour:
      return "capturecontour";
    case features::recteditcontour:
      return "recteditcontour";
    case features::desktopnotification:
      return "desktopnotification";
    case features::decstbm:
      return "decstbm";
    case features::vertlinemarkers:
      return "vertlinemarkers";
    default:
      return std::format("unknown{}", std::to_underlying(feature));
    }
  }

  std::optional<std::tuple<unsigned, unsigned>> info::get_geometry(int fd)
  {
    bool opened = fd == -1;
    if (fd == -1) {
      fd = ::open(_PATH_TTY, O_RDWR | O_CLOEXEC | O_NOCTTY | O_CLOEXEC);
      if (fd == -1)
        return std::nullopt;
    }
    winsize ws;
    auto r = ::ioctl(fd, TIOCGWINSZ, &ws);
    if (opened)
      ::close(fd);
    if (r == 0)
      return std::make_tuple(unsigned(ws.ws_col), unsigned(ws.ws_row));
    return std::nullopt;
  }

} // namespace terminal
