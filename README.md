# iana-tld-extractor -- [IANA Root Zone Database][1] extractor


## Description

This is a small utility derives data from HTML and print out
the result in [CSV][5] format.

If you need just the result CSV file, see [this file][7].


## Features

* Converts IDN domains to Punycode
* Save result to a CSV file


## Dependencies

* [libcurl][2] (optionally)
* [myhtml][3]
* [idnkit][4]
* [GNU make][6] to build


## Build

Before building the tool, install the [idnkit][4] library.
By default `iana-tld-extractor` expects that `idnkit` was
installed to `/usr/local`.

Then prepare `myhtml` git submodule, see [README.git][9] file
for details. After that build the tool.

```
% git submodule init
% git submodule update --checkout
% make
```

If `idnkit` was installed to a different path, then use
`IDNKIT_DIR` variable as shown below.

```
% make IDNKIT_DIR=/path/to/idnkit
```


## Usage

Ensure that LD_LIBRARY_PATH contains path to `libmyhtml.so`.
The example below expecting that you are in the same dir,
where you built `iana-tld-extractor`.

```
% export LD_LIBRARY_PATH=myhtml/lib
% ./iana-tld-extractor
usage: ./iana-tld-extractor [-d] HTML_FILE
```

Where `-d` option to download a fresh copy of the HTML page
from the IANA website and save that HTML data into
specified `FILE`.

To save the result CSV, please use pipe redirection to a file,
as shown below.

```
% ./iana-tld-extractor -d last.html > last_tld.csv
```


## Results

You may find out the latest parsed CSV file in this repository: [tld.csv][7].


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
