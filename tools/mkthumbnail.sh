#!/bin/bash

while [ $# -ne 0  ]
do
	thumb="`basename $1 | sed 's/\..*$//'`.thumb.png"
	echo "$1 -> $thumb"

	convert -resize x120 $1 $thumb

	shift
done
