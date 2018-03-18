#!/bin/bash


if [ "$#" -ne 2 ]; then
	echo "Usage: ./convert.sh path/to/unified_diff.patch 050-output-patchname"
	exit
fi

splitdiff -a $1
mkdir $2
for file in $1.part*.patch
do
	mkdir -p $(dirname $2/$(lsdiff --strip 1 $file))
	mv $file $2/$(lsdiff --strip 1 $file).patch
done
