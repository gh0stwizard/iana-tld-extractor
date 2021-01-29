# iana-tld-extractor -- [IANA Root Zone Database][1] extractor

![build](https://github.com/gh0stwizard/iana-tld-extractor/workflows/build/badge.svg)

## Description

This is a small utility derives data from [IANA Root Zone Database][1]
and prints out the result to a file in [CSV][5] format.

If you need just the result CSV file, see [tld.csv][7] (punycode version)
or [raw.csv][13] ("as is" version).


## Features

* Converts IDN domains to Punycode
* Saves the result to a CSV file in raw (as is) & punycode formats


## Dependencies

* [libcurl][2] (optionally)
* [myhtml][3] (included, see [README.git][9] for details)
* [idnkit][4] or [libidn2][12]
* [GNU make][6] or [cmake][14]
* pkg-config


## Build

Before building the application, please, prepare `myhtml`
git submodule, see [README.git][9] file for details.
After that build the tool.

```
% git submodule init
% git submodule update --checkout
```

By default the `Makefile` assumes that you are building
the application with `idnkit`. See sections below to change
the behavior.


### Build with idnkit

Before building the tool, install the [idnkit][4] library.
By default `iana-tld-extractor` expects that `idnkit` was
installed to `/usr/local`.

If `idnkit` was installed to a different path, then use
`IDNKIT_DIR` variable as shown below.


```
% make IDNKIT_DIR=/path/to/idnkit
```


### Build with libidn2

Use the option `FORCE_IDN=idn2` to build the application:

```
% make FORCE_IDN=idn2
```

In the case when there is no `libidn2.pc` file on your system
(Debian 8, for instance), you may do the next thing:

```
% make FORCE_IDN=idn2 DEFS="-I/usr/include" LIBS="-lidn2"
```


### Build without libcurl

Pass `WITH_CURL=NO` to `make`:

```
% make WITH_CURL=NO
```


### Build using cmake

Update git submodule as show above first, if you did not this already.
If you need to build the application with `libidn2` pass `WITH_IDN2=ON`
to `cmake` as show below.

To build with cmake 3.10+:

```
% rm -rf build
% cmake -S . -B build -D WITH_IDN2=ON
% cmake --build build/
```

To build with cmake 3.0x:

```
% rm -rf build
% mkdir build
% cd build
% cmake -D WITH_IDN2=ON ..
% make
```

To build without `libcurl` support use the option `WITH_CURL=OFF`,
for instance:

```
% cmake -D WITH_CURL=OFF ..
```


## Usage

Ensure that LD_LIBRARY_PATH contains path to `libmyhtml.so`
and to `libidnkit.so` when the application built with `idnkit`.
The example below expecting that you are in the same dir,
where you built `iana-tld-extractor`.

```
% export LD_LIBRARY_PATH=myhtml/lib
% ./iana-tld-extractor
Usage: iana-tld-extractor [OPTIONS] HTML_FILE
Options:
  --help, -h                   print this help
  --download, -d               download from IANA site
  --raw-domains, -r            print raw domains instead of punycode
  --version, -v                print version
```

Where `-d` option to download a fresh copy of the HTML page
from the IANA website and save that HTML data into
the specified `HTML_FILE`.

To save the result to a CSV file, please use pipe redirection
to the file, as shown below (without download).

```
% ./iana-tld-extractor last.html > last.csv
```

By default, `iana-tld-extractor` converts domain names to punycode.
If you wish to save a CSV file with raw domain names ("as is"), use
the `--raw-domains` option. The example below shows you how to get
the latest list of the TLDs in the raw format:

```
% ./iana-tld-extractor -d -r last.html > raw.csv
```


## Results

* Punycode: [tld.csv][7]
* Raw (as is): [raw.csv][13]


## SEE ALSO

* [github:incognico/list-of-top-level-domains][10]
* [Mozilla effective tld names][11]


## Credits

[UTF-8 decoder][8] by JSON.org


## License

This software is licensed under "The 2-Clause BSD License".


[1]: https://www.iana.org/domains/root/db
[2]: https://curl.haxx.se/
[3]: https://github.com/lexborisov/myhtml
[4]: https://jprs.co.jp/idn/index-e.html
[5]: https://en.wikipedia.org/wiki/Comma-separated_values
[6]: https://www.gnu.org/software/make/
[7]: /tld.csv
[8]: http://www.json.org/JSON_checker/
[9]: /README.git
[10]: https://github.com/incognico/list-of-top-level-domains
[11]: http://mxr.mozilla.org/mozilla-central/source/netwerk/dns/effective_tld_names.dat?raw=1
[12]: https://gitlab.com/libidn/libidn2
[13]: /raw.csv
[14]: https://cmake.org/
