#!/bin/sh

#
# Update tld.csv & raw.csv files automatically.
#

CURDIR=`dirname $0`
# assuming that this script is placed in "misc/" directory.
SRCDIR=$(realpath "${CURDIR}/../")
# customize this if built with cmake
APP=$(realpath ${SRCDIR}/iana-tld-extractor)
#APP=$(realpath ${SRCDIR}/build/iana-tld-extractor)
HTML_FILE="last.html"
IANA_URL="https://www.iana.org/domains/root/db"
LOG="logger -s update-iana-db.sh: "
CURL=`which curl`
WGET=`which wget`


download() {
	if [ -n "${CURL}" ]; then
		${CURL} -S -s -f -o ${HTML_FILE} -L ${IANA_URL} || exit 1
	elif [ -n "${WGET}" ]; then
		${WGET} -q -O ${HTML_FILE} ${IANA_URL} || exit 1
	else
		$LOG "ERROR: neither curl or wget has installed."
		exit 1
	fi

	if [ ! -s ${HTML_FILE} ]; then
		$LOG "ERROR: ${HTML_FILE} is empty!"
		exit 1
	fi
}


cd "${SRCDIR}" || exit 1
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${SRCDIR}/myhtml/lib"


if [ -x ${APP} ]; then
	git pull || exit 1
	download
	# update CSV files
	${APP} ${HTML_FILE} > tld.csv
	${APP} -r ${HTML_FILE} > raw.csv
else
	$LOG "${APP} does not exist or is not executable"
	exit 1
fi

git diff --exit-code tld.csv raw.csv && \
	$LOG "Nothing to update" && \
	exit 0

if [ -s "tld.csv" ] && [ -s "raw.csv" ]; then
	$LOG "Updating IANA database ..."
	datetime=`date -u`
	git add tld.csv raw.csv
	git commit -m "update database ${datetime}"
	git push || exit 1
else
	$LOG "ERROR: tld.csv and raw.csv files are empty!"
	git checkout tld.csv raw.csv
	exit 1
fi
