# termdetect

[![License: CC BY-NC-ND 4.0](https://img.shields.io/badge/License-CC_BY--NC--ND_4.0-lightgrey.svg)](https://creativecommons.org/licenses/by-nc-nd/4.0/)

This package contains a simple library which allows to discover the terminal emulator
used by the program.  This allows to discover the features of the terminal emulator the
program can take advantage of.  The terminfo data (specified by the `$TERM` environment
variable) contains only a small subset of the data.


## Supported Emulators

The following is a list of the emulators which are currently supported.  This does not mean
that the code will works as the emulator developers change their code and it also does not
mean that all features are correctly recognized yet.  In alphabetical order:

- Alacritty
- Contour
- Foot
- Kitty
- Konsole
- rxvt
- ST
- Terminology
- VTE (e.g., gnome-terminal)
- XTerm


## Methods used

So far only the following ways are used to discover details of the emulator:

- DA1 (`CSI c`)
- DA2 (`CSI > c`)
- DA3 (`CSI = c`)
- Q (`CSI > q`)
- TN (`DCS + q 5 4 4 e \e \`)
- OSC702 (`OSC 7 0 2 ; ?`)

More might be used in the future.

The supported emulators respond as follows:

+----------------+-----------+-----------+-----------+-----------+-----------+------------+
| Name           |    DA1    |    DA2    |    DA3    |     Q     |    TN     |   OSC702   |
+----------------+-----------+-----------+-----------+-----------+-----------+------------+
|                |           |           |           |           |           |            |
| Alacritty      | 6         | 0;VERS;1  | no resp   | no resp   | no resp   |            |
| Contour        | a lot     | 65;VERS;0 | C0000000  | contour * | ""        |            |
| Foot           | 62;4;22   | 1;VERS;0  | 464f4f54  | foot(*    | 666F6F74  |            |
| Kitty          | 62;       | 1;4000;29 | no resp   | kitty*    | 78746572* |            |
| Konsole        | 62;1;4    | 1;VERS;0  | 7E4B4445  | Konsole*  | no esp    |            |
| rxvt           | 1;2       | 85;VERS;0 | no resp   | no resp   | no resp   | rxvt*      |
| ST             | 6         | no resp   | no resp   | no resp   | no resp   |            |
| Terminology    | a lot     | 61;VERS;0 | 7E7E5459  | terminolo*| no resp   |            |
| VTE            | 65;1;9    | 65;VERS;1 | 7E565445  | no resp   | no resp   |            |
| XTerm          | a lot     | 41;VERS;0 | 00000000  | XTerm(*   | no resp   |            |
|                |           |           |           |           |           |            |
+----------------+-----------+-----------+-----------+-----------+-----------+------------+

This means that the time of this writing these rules are followed in the discovery:

- Alacritty does not handle `CSI > q`, `DCS + q T N`, DA3, nor OSC702
- VTE does not understand `CSI > q` but that is the ultimate informer for XTerm
- alternatively DA3 can be used as a weak signal for xterm but DA3 does not work for Kitty nor rxvt
- Kitty needs the `DCS + q T N` request but this also does not work for VTE
- ST only responds to DA1 and its answer to that request (= "6") is not unique (same as Alacritty)


## To Do

- [ ] Add features beyond those from DA2 to the feature set
