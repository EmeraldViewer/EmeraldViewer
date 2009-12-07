#!/bin/bash

# fetch_bins.sh
# Added utility script to grab Vivox and Kakadu components from within a Linden Lab client package ~N

# Necessary files
BINS="bin/libllkdu.so bin/SLVoice lib/libkdu.so lib/libortp.so lib/libvivoxsdk.so"

# Locations of client to use
URL="http://download.cloud.secondlife.com/SecondLife-i686-1.23.5.136262.tar.bz2"
ARCHIVE="${URL##*/}"
FOLDER="${ARCHIVE%.*.*}"

missing_bins() {
	for file in $BINS; do
		if [[ ! -f $file ]]; then
			echo "INFO: Missing binary ./$file"
			return 0
		fi
	done
	
	return 1
}


echo "INFO: Looking for missing binaries.."
if missing_bins; then
	echo "INFO: Fetching Linden Lab client for missing files.."
	if wget -nc --random-wait $URL; then
		echo "INFO: Extracting binaries.."
		if tar -xjv --strip-components=1 -f $ARCHIVE $FOLDER/${BINS// / $FOLDER/}; then
			echo "INFO: All binaries accounted for"
		fi
	fi
else
	echo "INFO: All binaries found"
fi
