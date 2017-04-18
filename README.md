# iana-tld-extractor -- [IANA Root Zone Database][1] extractor


## Description

This is a small utility derives data from HTML and print out
the result in [CSV][5] format.


## Features

* Sanitize data from uneccessary characters (e.g. LF)
* Converts IDN domains to Punycode


## Usage

```
shell> ./iana-tld-extractor
usage: ./iana-tld-extractor [-d] FILE
```

Where `-d` option to download a fresh copy from the IANA website and 
save that HTML data into specified `FILE`.


### Download and print CSV to STDOUT

```
shell> ./iana-tld-extractor -d my.db
shell> # output CSV data
```


### Download and save to file

```
shell> ./iana-tld-extractor -d my.db > tld.csv
```


## Results

You may find out the latest parsed CSV file in this repository: [tld.cvs][7].


## Dependencies

* [libcurl][2] (optionally)
* [myhtml][3]
* [idnkit][4]
* [GNU make][6] to build


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
