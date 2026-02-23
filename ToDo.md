- [ ] OSC52: set clipboard with escape sequence.  Disabled often for security reasons.  Check how to check at runtime

# From blessed

See [repo](https://github.com/jquast/blessed/blob/master/blessed/dec_modes.py)

- [ ] Use the DEC modes for the features
- [ ] not bit values, add interface to query specific features
- [ ] add caching of all requests, there should not be a second test

## Generalized query?

in `terminal.py` from blessed, the `get_dec_mode` function uses a query

```Python
f'\x1b[?{int(mode):d}$p'
```

and an expected result in the form of

```Python
re.compile(f'\x1b\\[\\?{int(mode):d};([0-4])\\$y')
```

The value returned after the semicolon is one of
- '0': not recognized
- '1': set
- '2': reset
- '3': permanenty set
- '4': permanenty reset

Check whether this applies to all emulators.  Notes from jqast's site:

- Terminology is inconsistent (corruptions?)
- iTerm2 has many "supported, but disabled, and cannot be changed"
- Konsole does not reply to these queries
- Contour returns sometimes replies with a different mode number (query: 25 -> reply: 80)


## Known modes

```
    DECCKM = 1
    DECANM = 2
    DECCOLM = 3  # https://vt100.net/docs/vt510-rm/DECCOLM.html
    DECSCLM = 4  # https://vt100.net/docs/vt510-rm/DECSCLM.html
    DECSCNM = 5  # https://vt100.net/docs/vt510-rm/DECSCNM.html
    DECOM = 6    # https://vt100.net/docs/vt510-rm/DECOM.html
    DECAWM = 7   # https://vt100.net/docs/vt510-rm/DECAWM.html
    DECARM = 8   # https://vt100.net/docs/vt510-rm/DECARM.html
    DECINLM = 9
    DECEDM = 10
    DECLTM = 11
    DECKANAM = 12
    DECSCFDM = 13
    DECTEM = 14
    DECEKEM = 16
    DECPFF = 18
    DECPEX = 19
    OV1 = 20
    BA1 = 21
    BA2 = 22
    PK1 = 23
    AH1 = 24
    DECTCEM = 25
    DECPSP = 27
    DECPSM = 29
    SHOW_SCROLLBAR_RXVT = 30
    DECRLM = 34
    DECHEBM = 35
    DECHEM = 36
    DECTEK = 38
    DECCRNLM = 40
    DECUPM = 41
    DECNRCM = 42
    DECGEPM = 43
    DECGPCM = 44
    DECGPCS = 45
    DECGPBM = 46
    DECGRPM = 47
    DECTHAIM = 49
    DECTHAICM = 50
    DECBWRM = 51
    DECOPM = 52
    DEC131TM = 53
    DECBPM = 55
    DECNAKB = 57
    DECIPEM = 58
    DECKKDM = 59
    DECHCCM = 60
    DECVCCM = 61
    DECPCCM = 64
    DECBCMM = 65
    DECNKM = 66
    DECBKM = 67
    DECKBUM = 68
    DECVSSM = 69
    DECFPM = 70
    DECXRLM = 73
    DECSDM = 80
    DECKPM = 81
    WY_52_LINE = 83
    WYENAT_OFF = 84
    REPLACEMENT_CHAR_COLOR = 85
    DECTHAISCM = 90
    DECNCSM = 95
    DECRLCM = 96
    DECCRTSM = 97
    DECARSM = 98
    DECMCM = 99
    DECAAM = 100
    DECCANSM = 101
    DECNULM = 102
    DECHDPXM = 103
    DECESKM = 104
    DECOSCNM = 106
    DECNUMLK = 108
    DECCAPSLK = 109
    DECKLHIM = 110
    DECFWM = 111
    DECRPL = 112
    DECHWUM = 113
    DECATCUM = 114
    DECATCBM = 115
    DECBBSM = 116
    DECECM = 117

    # Mouse reporting modes and xterm/rxvt extensions
    MOUSE_REPORT_CLICK = 1000
    MOUSE_HILITE_TRACKING = 1001
    MOUSE_REPORT_DRAG = 1002
    MOUSE_ALL_MOTION = 1003
    FOCUS_IN_OUT_EVENTS = 1004
    MOUSE_EXTENDED_UTF8 = 1005
    MOUSE_EXTENDED_SGR = 1006
    ALT_SCROLL_XTERM = 1007
    SCROLL_ON_TTY_OUTPUT_RXVT = 1010
    SCROLL_ON_KEYPRESS_RXVT = 1011
    FAST_SCROLL = 1014
    MOUSE_URXVT = 1015
    MOUSE_SGR_PIXELS = 1016
    BOLD_ITALIC_HIGH_INTENSITY = 1021

    # Keyboard and meta key handling modes
    META_SETS_EIGHTH_BIT = 1034
    MODIFIERS_ALT_NUMLOCK = 1035
    META_SENDS_ESC = 1036
    KP_DELETE_SENDS_DEL = 1037
    ALT_SENDS_ESC = 1039

    # Selection, clipboard, and window manager hint modes
    KEEP_SELECTION_NO_HILITE = 1040
    USE_CLIPBOARD_SELECTION = 1041
    URGENCY_ON_CTRL_G = 1042
    RAISE_ON_CTRL_G = 1043
    REUSE_CLIPBOARD_DATA = 1044
    EXTENDED_REVERSE_WRAPAROUND = 1045
    ALT_SCREEN_BUFFER_SWITCH = 1046

    # Alternate screen buffer and cursor save/restore combinations
    ALT_SCREEN_BUFFER_XTERM = 1047
    SAVE_CURSOR_DECSC = 1048
    ALT_SCREEN_AND_SAVE_CLEAR = 1049

    # Terminal info and function key emulation modes
    TERMINFO_FUNC_KEY_MODE = 1050
    SUN_FUNC_KEY_MODE = 1051
    HP_FUNC_KEY_MODE = 1052
    SCO_FUNC_KEY_MODE = 1053

    # Legacy keyboard emulation modes
    LEGACY_KBD_X11R6 = 1060
    VT220_KBD_EMULATION = 1061

    SIXEL_PRIVATE_PALETTE = 1070

    # VTE BiDi extensions
    BIDI_ARROW_KEY_SWAPPING = 1243

    # iTerm2 extensions
    ITERM2_REPORT_KEY_UP = 1337

    # XTerm readline and mouse enhancements
    READLINE_MOUSE_BUTTON_1 = 2001
    READLINE_MOUSE_BUTTON_2 = 2002
    READLINE_MOUSE_BUTTON_3 = 2003
    BRACKETED_PASTE = 2004
    READLINE_CHARACTER_QUOTING = 2005
    READLINE_NEWLINE_PASTING = 2006

    # Modern terminal extensions
    SYNCHRONIZED_OUTPUT = 2026
    GRAPHEME_CLUSTERING = 2027
    TEXT_REFLOW = 2028
    PASSIVE_MOUSE_TRACKING = 2029
    REPORT_GRID_CELL_SELECTION = 2030
    COLOR_PALETTE_UPDATES = 2031
    IN_BAND_WINDOW_RESIZE = 2048

    # VTE bidirectional text extensions
    MIRROR_BOX_DRAWING = 2500
    BIDI_AUTODETECTION = 2501

    # mintty extensions
    AMBIGUOUS_WIDTH_REPORTING = 7700
    SCROLL_MARKERS = 7711
    REWRAP_ON_RESIZE_MINTTY = 7723
    APPLICATION_ESCAPE_KEY = 7727
    ESC_KEY_SENDS_BACKSLASH = 7728
    GRAPHICS_POSITION = 7730
    ALT_MODIFIED_MOUSEWHEEL = 7765
    SHOW_HIDE_SCROLLBAR = 7766
    FONT_CHANGE_REPORTING = 7767
    GRAPHICS_POSITION_2 = 7780
    SHORTCUT_KEY_MODE = 7783
    MOUSEWHEEL_REPORTING = 7786
    APPLICATION_MOUSEWHEEL = 7787
    BIDI_CURRENT_LINE = 7796

    # Terminal-specific extensions
    TTCTH = 8200
    SIXEL_SCROLLING_LEAVES_CURSOR = 8452
    CHARACTER_MAPPING_SERVICE = 8800
    AMBIGUOUS_WIDTH_DOUBLE_WIDTH = 8840
    WIN32_INPUT_MODE = 9001
    KITTY_HANDLE_CTRL_C_Z = 19997
    MINTTY_BIDI = 77096
    INPUT_METHOD_EDITOR = 737769
```
