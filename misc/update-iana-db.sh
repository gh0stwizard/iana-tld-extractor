#!/bin/sh

#
# Update tld.csv & raw.csv files automatically.
#

CURDIR=`dirname $0`
# assuming that this script is placed in "misc/" directory.
SRCDIR=$(realpath "${CURDIR}/../")
# customize this if built with cmake
APP=$(realpath ${SRCDIR}/iana-tld-extractor)
HTML_FILE=last.html
LOG="logger -s update-iana-db.sh: "

cd "${SRCDIR}" || exit 1
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:myhtml/lib

git pull || exit 1

curl -s -o ${HTML_FILE} -L https://www.iana.org/domains/root/db || exit 1

if [ -x ${APP} ]; then
	./build/iana-tld-extractor ${HTML_FILE} > tld.csv
	./build/iana-tld-extractor -r ${HTML_FILE} > raw.csv
elif [ -x iana-tld-extractor ]; then
	./iana-tld-extractor ${HTML_FILE} > tld.csv
	./iana-tld-extractor -r ${HTML_FILE} > raw.csv
fi

git diff --exit-code && $LOG "Nothing to update" && exit 0

$LOG "Updating IANA database ..."
datetime=`date -u`
git add tld.csv raw.csv
git commit -m "update database ${datetime}"
git push || exit 1
exit 0
