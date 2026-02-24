#ifndef _TERMDETECT_HH
# define _TERMDETECT_HH 1

# include <cstdint>
# include <memory>
# include <optional>
# include <set>
# include <string>
# include <tuple>

# include <unistd.h>


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
    rio,
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
    decckm = 1,          // Cursor keys mode
    devanm = 2,          // ANSI/VT52 mode
    deccollm = 3,        // Column
    decsclm = 4,         // Scrolling
    decscnm = 5,         // Screen mode (light or dark screen)
    decom = 6,           // Origin mode
    decawm = 7,          // Auto wrap mode
    decarm = 8,          // Auto repeat mode
    decinlm = 9,         // Interlace mode
    decedm = 10,         // Editing mode
    decltm = 11,         // Line transmit mode
    deckanam = 12,       // Katakana shift mode
    decscfdm = 13,       // Space compressiion/field delimiter mode
    dectem = 14,         // Transmit execution mode
    decekem = 16,        // Edit key execution mode
    decpff = 18,         // Print form feed
    decpex = 19,         // Printer extend
    ov1 = 20,            // Overstrike
    ba1 = 21,            // Local BASIC
    ba2 = 22,            // Host BASIC
    pk1 = 23,            // Programmable keypad
    ah1 = 24,            // Auto hardcopy
    dectcem = 25,        // Text cursor enable mode
    decpsp = 27,         // Proportional spacing
    decpsm = 29,         // Pitch select mode
    show_scrollbar = 30, // Show scrollbar (rxvt)
    decrlm = 34,         // Cursor right to left mode
    dechebm = 35,        // Hebrew (keyboard) mode
    dechem = 36,         // Hebrew encoding mode
    dectek = 38,         // Tektronix 4010/4014 mode
    deccrnlm = 40,       // Carriage return/new line mode
    devupm = 41,         // Unidirectional print mode
    decnrcm = 42,        // National replacement character set mode
    decgepm = 43,        // Graphics expanded print mode
    decgpcm = 44,        // Graphics print color mode
    decgpcs = 45,        // Graphics print color syntax
    decgpbm = 46,        // Graphics print background mode
    decgrpm = 47,        // Graphics rotated print mode
    decthaim = 49,       // Thai input mode
    decthaicm = 50,      // Thai cursor mode
    decbwrm = 51,        // Black/white reversal mode
    decopm = 52,         // Origin placement mode
    dec131tm = 53,       // VT131 transmit mode
    decbpm = 55,         // Bold page mode
    decnakb = 57,        // Greek/N-A keyboard mapping mode
    decipem = 58,        // Enter IBM proprinter emulation mode
    deckkdm = 59,        // Kanji/katakana display mode
    dechccm = 60,        // Horizontal cursor coupling mode
    decvccm = 61,        // Vertical cursor coupling mode
    decpccm = 64,        // Page cursor coupling mode
    decbcmm = 65,        // Business color matching mode
    decnkm = 66,         // Numeric keypad mode
    decbkm = 67,         // Backarrow key mode
    deckbum = 68,        // Keyboard usage mode
    decvssm = 69,        // Vertical split screen mode
    declrmm = 69,        // Left right margin mode
    decfpm = 70,         // Force plot mode
    decxrlm = 73,        // Transmission rate limiting
    decsdm = 80,         // Sixel display mode
    sixel = 80,
    deckpm = 81,                      // Key position mode
    wy52line = 83,                    // 52 line (WY-370)
    wyenat = 84,                      // Erasable/nonerasable WNEAT off attribute select (WY-370)
    replacement_color = 85,           // replacement character color (WY-370)
    decthaiscm = 90,                  // That space compensating mode
    decncsm = 95,                     // No clearing screen on column change mode
    decrlcm = 96,                     // right to left copy mode
    deccrtsm = 97,                    // CRT save mode
    decarsm = 98,                     // Auti resize mode
    decmcm = 99,                      // Modem control mode
    decaam = 100,                     // Auto answerback mode
    deccansm = 101,                   // Conceal answerback message mode
    decnulm = 102,                    // Ignore null mode
    dechdpxm = 103,                   // Half duplex mode
    deceskm = 104,                    // Secondary keyboard language mode
    decoscnm = 106,                   // Overscan mode
    decnumlk = 108,                   // NumLock mode
    deccapslk = 109,                  // Caps lock mode
    decklhim = 110,                   // Keyboard LESs host indicator mode
    decfwm = 111,                     // Framed window mode
    decrpl = 112,                     // Review previous lines mode
    dechwum = 113,                    // Host wake-up mode
    decatcum = 114,                   // Alternate text color underline mode
    decatcbm = 115,                   // Alternate text color blink mode
    decbbsm = 116,                    // Bold and blink style mode
    dececm = 117,                     // Erase color mode
    mouse_event = 1000,               // Send mouse x&y on button press (xterm)
    hilite_mouse = 1001,              // Use hilite mouse tracking (xterm)
    cell_motion = 1002,               // Use cell motion mouse tracking (xterm)
    all_motion = 1003,                // Use all motion mouse tracking (xterm)
    focus_in_out = 1004,              // Send focusin/focusout events (xterm)
    utf8_mouse = 1005,                // Enable UTF-8 mouse tracking (xterm)
    sgr_mouse = 1006,                 // Enable SGR mouse mode (xterm)
    alternate_scroll = 1007,          // Enable alternate scoll mode (xterm)
    scroll_bottom_output = 1010,      // Scroll to bottom on tty output (rxvt)
    scroll_bottom_key = 1011,         // Scroll to bottom on key press (rxvt)
    fastscroll = 1014,                // Enable fastscroll resource (xterm)
    urxvt_mouse = 1015,               // Enable urxvt mouse mode (urxvt)
    sgr_pixel = 1016,                 // Enable SGR mouse pixelmode (xterm)
    bold_high = 1021,                 // Bold/Italic implies high intensity (rxvt)
    meta_key = 1034,                  // Interpret "meta" key (xterm)
    alt_numlock_mod = 1035,           // Enable special modifiers for alt and numlock keys (xterm)
    esc_meta = 1036,                  // Send ESC when meta modifies a key (xterm)
    keypad_del = 1037,                // Send DEL from the editing-keypad delete key (xterm)
    esc_alt = 1039,                   // Send ESC when alt modifies a key (xterm)
    selection_not_highlighted = 1040, // Keep selection even if not highlighted (xterm)
    clipboard_selection = 1041,       // Use the clipboard selection (xterm)
    ctrlg_hint = 1042,                // Enable urgency window manager hint when control-G is received (xterm)
    ctrlg_raise = 1043,               // Enable raising of the window when control-G is received (xterm)
    reuse_clipboard = 1044,           // Reuse the most reven data copied to clipboard (xterm)
    xtrevwrap2 = 1045,                // Extended reverse-wraparound mode (xterm)
    switch_alt = 1046,                // Enable switching to/from alternate screen buffer (xterm)
    use_alt = 1047,                   // Use alternate screen buffer (xterm)
    cursor_save = 1048,               // Save cursor as in DECSC (xterm)
    cursor_alt_save = 1049,           // Save cursor as in DECSC and use alternate screen buffer (xterm)
    terminfo_fnkey = 1050,            // Set terminfo/termcap function-key mode (xterm)
    sun_fnkey = 1051,                 // Set Sun function-key mode (xterm)
    hp_fnkey = 1052,                  // Set HP function-key mode (xterm)
    sco_fnkey = 1053,                 // Set SCO function-key mode (xterm)
    legacy_keyboard = 1060,           // Set legacy keyboard emulation , i.e., X11R6 (xterm)
    vt220_keyboard = 1061,            // Set VT220 keyboard emulation (xterm)
    private_color = 1070,             // Use private color registers for each graphics (xterm)
    arrow_key_swap = 1243,            // Arrow keys swapping (BiDi) (VTE)
    key_up = 1337,                    // Report key up (iTerm2)
    readline_mouse_1 = 2001,          // Enable readline mouse button-1 (xterm)
    readline_mouse_2 = 2002,          // Enable readline mouse button-2 (xterm)
    readline_mouse_3 = 2003,          // Enable readline mouse button-3 (xterm)
    bracket_paste = 2004,             // Set bracketed paste mode (xterm)
    char_quoting = 2005,              // Enable readline character-quoting (xterm)
    newline_pasting = 2006,           // Enable readline newline pasting (xterm)
    sync_output = 2026,               // Synchronized output (contour)
    grapheme = 2028,                  // Grapheme cluster processing (contour)
    passive_mouse = 2029,             // Passive mouse tracking (contour)
    report_grid = 2030,               // Report grid cell selection (contour)
    palette_updates = 2031,           // Color palette updates (contour)
    mirror_box_drawing = 2500,        // Mirror box drawing characters (VTE)
    bidi_auto = 2501,                 // BiDi autodetection (VTE)
    report_ambiguous = 7700,          // Ambiguous width reporting (mintty)
    scroll_markers = 7711,            // Scroll markers (prompt start) (mintty) also used for OSC133
    rewrap_resize = 7723,             // Rewrap in resize (mintty)
    app_esc_key = 7727,               // Application escape key mode (mintty)
    backslash_esc = 7728,             // Send ^\ instead of ^[ for ESC key (mintty)
    graphics_pos = 7730,              // Graphics position (mintty)
    alt_mode_wheel = 7765,            // Alt-modified mousewheel mode (mintty)
    show_hide_scrollbar = 7766,       // Show/hide scrollbar (mintty)
    font_change = 7767,               // Font change reporting (mintty)
    shortcut_key = 7783,              // Shortcut key mode (mintty)
    mousewheel = 7786,                // Mousewheel reporting (mintty)
    app_mousewheel = 7787,            // Application mousewheel reporting (mintty)
    current_bidi = 7796,              // BiDi in current line (mintty)

    mouse_tracking = 20009,        // Mouse tracking (xterm)
    show_toolbar = 20010,          // Show toolbar (rxvt)
    blinking_cursor = 20012,       // Blinking cursor (xterm)
    start_blinking_cursor = 20013, // Start blinking cursor (xterm)
    xor_blinking = 20014,          // Enable XOR of blinking cursor control sequence and menu
    font_shifting = 20035,         // Enable font-shifting functions (rxvt)
    col132 = 20040,                // Allow 80 ⇒ 132 mode (xterm)
    more_fix = 20041,              // more fix (xterm)
    margin_bell = 20044,           // Turn on margin bell (xterm)
    reverse_wrap = 20045,          // Reverse-wraparound mode (xterm)
    start_logging = 20046,         // Start logging (xterm)
    alternative_buffer = 20047,    // Use alternative scsreen buffer (xterm)

    // XYZ Old features, merge
    printer = 10000,
    regis,
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
    underlinecolors,
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

    struct color {
      uint8_t r;
      uint8_t g;
      uint8_t b;
      bool operator==(const color& other) const { return other.r == r && other.g == g && other.b == b; }
    };
    color default_foreground{};
    color default_background{};

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
