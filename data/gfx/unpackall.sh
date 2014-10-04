#!/bin/bash

for i in *.pack
do
  echo "LDAT: $i"
  ldat --extract --index $i
done
