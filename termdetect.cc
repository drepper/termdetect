#include "termdetect.hh"

#include <cctype>
#include <cstring>
#include <format>
#include <optional>
#include <utility>

#include <fcntl.h>
#include <paths.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>


namespace terminal {

  namespace {

    struct info_impl final : info {
      info_impl();

      std::string da1_reply { };
      std::string da2_reply { };
      std::string da3_reply { };
      std::string q_reply { };
      std::string tn_reply { };
      std::string osc702_reply { };

      bool da2_alarmed = false;


      void make_da1_request(int fd);
      void parse_da1();

      bool make_da2_request(int fd);
      void parse_da2();

      bool is_st() const;
      bool is_alacritty() const;
      bool is_vte() const;
      bool is_not_vte() const;
      bool is_terminology() const;
      bool is_contour() const;
      bool is_xterm() const;
      bool is_rxvt() const;
      bool is_kitty() const;
      bool is_konsole() const;
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
      ::tcsetattr(fd, TCSAFLUSH, &t_new);

      if (::write(fd, request, strlen(request)) == ssize_t(strlen(request))) [[likely]] {
        wok = true;

        pollfd pfds[1] {
          { fd, POLLIN, 0 }
        };
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
        }
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
    }


    bool info_impl::make_da2_request(int fd)
    {
      bool rfailed = make_request(da2_reply, fd, DA2_REQUEST, DA2_REPLY_PREFIX, DA2_REPLY_SUFFIX);

      parse_da2();

      return rfailed;
    }


    void info_impl::parse_da2()
    {
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
      auto [endp,ec] = std::from_chars(da2_reply.data() + 2, da2_reply.data() + da2_reply.size(), val, 10);
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

  } // anonymous namespace


  info_impl::info_impl()
  : info()
  {
    if (! request_delay.has_value())
      request_delay = get_default_request_delay();

    int fd = ::open(_PATH_TTY, O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
    if (fd != -1) [[likely]] {
      // The DA1 and DA2 requests seem to be universally implemented.  Note that the order of the calls is required.
      // Information about the terminal emulation from DA2 is more reliable.
      da2_alarmed = make_da2_request(fd);
      make_da1_request(fd);

      // The order to make requests without stalling/timing out in the reads is complicated.
      // - alacritty does not handle CSI > q, CSI + q T N, DA3, nor OSC 702
      // - VTE does not understand CSI > q but that is the ultimate informer for xterm.
      // - alternatively DA3 can be used as a weak signal for xterm but DA3 does not work for kitty nor rxvt
      // - kitty needs the CSI + q T N request but this also does not work for VTE
      // We break the cycle by not issuing DA3 early and avoid if the CSI > q and CSI + q T N requests if
      // the terminal could possibly be VTE based.  Once we can exclude rxvt and kitty we can issue DA3
      // to be sure.
      // +----------------+-----------+-----------+-----------+-----------+-----------+------------+
      // | Name           |    DA1    |    DA2    |    DA3    |     Q     |    TN     |   OSC702   |
      // +----------------+-----------+-----------+-----------+-----------+-----------+------------+
      // |                |           |           |           |           |           |            |
      // | Alacritty      | 6         | 0;VERS;1  | no resp   | no resp   | no resp   |            |
      // | Contour        | a lot     | 65;VERS;0 | C0000000  | contour * | ""        |            |
      // | Foot           | 62;4;22   | 1;VERS;0  | 464f4f54  | foot(*    | 666F6F74  |            |
      // | Kitty          | 62;       | 1;4000;29 | no resp   | kitty*    | 78746572* |            |
      // | Konsole        | 62;1;4    | 1;VERS;0  | 7E4B4445  | Konsole*  | no esp    |            |
      // | rxvt           | 1;2       | 85;VERS;0 | no resp   | no resp   | no resp   | rxvt*      |
      // | ST             | 6         | no resp   | no resp   | no resp   | no resp   |            |
      // | Terminology    | a lot     | 61;VERS;0 | 7E7E5459  | terminolo*| no resp   |            |
      // | VTE            | 65;1;9    | 65;VERS;1 | 7E565445  | no resp   | no resp   |            |
      // | xterm          |           |           |           |           |           |            |
      // |                |           |           |           |           |           |            |
      // +----------------+-----------+-----------+-----------+-----------+-----------+------------+

      // Detecting ST is with the currently used requests not possible without a delay.  It only
      // responds to DA1 and its answer to that request (= "6") is not unique (same as Alacritty).
      // Unless there is something else that can be done the best we can do is to limit the number
      // of delays to one by determining the emulator type based on the DA2 request timeout.
      if (! is_st() && ! is_alacritty()) {

      }

      ::close(fd);

      // We are ready to determine the emulators.
      if (is_st())
        implementation = implementations::st;
      else if (da3_reply == "464f4f54")
        implementation = implementations::foot;
      else if (is_terminology())
        implementation = implementations::terminology;
      else if (is_contour())
        implementation = implementations::contour;
      else if (is_xterm())
        implementation = implementations::xterm;
      else if (osc702_reply.starts_with("rxvt"))
        implementation = implementations::rxvt;
      else if (is_kitty())
        implementation = implementations::kitty;
      else if (is_alacritty())
        implementation = implementations::alacritty;
      else if (is_konsole())
        implementation = implementations::konsole;
    }
  }


  const std::shared_ptr<info> info::get()
  {
    return std::make_shared<info_impl>();
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

    for (auto b : real_this->da2_reply)
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
    default:
      return std::format("unknown{}", std::to_underlying(feature));
    }
  }


} // namespace terminal
