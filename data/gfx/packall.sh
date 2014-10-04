#!/bin/bash

for i in *.pack
do
  echo "LDAT: $i"
  ldat --pack --index $i
done
