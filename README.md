# StompTuner

![StompTuner](https://github.com/brummer10/StompTuner/blob/main/StompTuner.png?raw=true)

StompTuner, a Strobe Tuner in Stomp Box Format. The Strobe provide 2 indicators. The outer ring 
have a accuracy of 1.0 Cent, the inner ring have a accuracy at 0.1 Cent. 
The working frequency range is from 24 - 998 Hz.
The reference Pitch could be selected between 432 - 452 Hz.

## Formats

StompTuner come in the following plug-in formats:

* [LV2]
* [VST2]
* [VST3]
* [CLAP]
* [AU]

## Compiling

Make sure you have installed the required build tools and libraries (see
section "Prerequisites" below) and then clone this repository (including
sub-modules) and simply run `make` in the project's root directory:

```con
git clone --recursive https://github.com:brummer10/StompTuner
cd StompTuner
make
```

## Installation

To install all plugin formats to their appropriate system-wide location, run
the following command (root priviledges may be required):

```con
make install
```

The makefiles support the usual `PREFIX` and `DESTDIR` variables to change the
installation prefix and set an installation root directory (defaulty: empty).
`PREFIX` defaults to `/usr/local`, but on macOS and Windows it is not used,
since the system-wide installation directories for plugins are fixed.

Use make's `-n` option to see where the plugins would be installed without
actually installing them.

You can also set the installation directory for each plugin format with a
dedicated makefile variable.

* LV2: `LV2_DIR` (`<prefix>/lib/lv2`)
* VST2: `VST3_DIR` (`<prefix>/lib/vst2`)
* VST3: `VST3_DIR` (`<prefix>/lib/vst3`)
* CLAP: `CLAP_DIR` (`<prefix>/lib/clap`)
* AU:   `AU_DIR` (`/Library/Audio/Plug-Ins/Components`)

Example:

```con
make DESTDIR=/tmp/build-root PREFIX=/usr VST2_DIR=/usr/lib/lxvst install
```

To install the plugins only for your current user account, run:

```con
make install-user
```

Again, you can also set the installation directory for each plugin format with
a dedicated makefile variable.

* LV2: `USER_LV2_DIR` (`$HOME/.lv2`)
* VST2: `USER_VST2_DIR` (`$HOME/.vst2`)
* VST3: `USER_VST3_DIR` (`$HOME/.vst3`)
* CLAP: `USER_CLAP_DIR` (`$HOME/.clap`)
* AU:   `AU_DIR` (`$HOME/Library/Audio/Plug-Ins/Components`)

*Note: The given default values for all of the above listed environment
variables differ depending on the target OS.*


## Prerequisites

* The GCC C++ compiler, library and the usual associated software build tools
  (GNU `make`, etc.).

  Debian / Ubuntu users should install the `build-essential` package
  to get these, Arch users the `base-devel` package group.

* [pkgconf]

The [LV2], [VST2] (Xaymar/vst2sdk) and [VST3] (travesty) headers are included in the
[DPF] framework, which is integrated as a Git sub-module. These need not be
installed separately to build the software in the respective plug-in formats.


## Author

This software was created by *brummer*.


## Acknowledgements

This project is built using the DISTRHO Plugin Framework ([DPF])

[DPF]: https://github.com/DISTRHO/DPF
[LV2]: http://lv2plug.in/
[pkgconf]: https://github.com/pkgconf/pkgconf
[VST2]: https://en.wikipedia.org/wiki/Virtual_Studio_Technology
[VST3]: https://en.wikipedia.org/wiki/Virtual_Studio_Technology
[CLAP]:https://en.wikipedia.org/wiki/CLever_Audio_Plug-in
[AU]:https://en.wikipedia.org/wiki/Audio_Unit
