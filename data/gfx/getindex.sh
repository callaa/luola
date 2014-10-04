#!/bin/bash
# Get the index files for each LDAT file

for i in *.ldat
do
  indexfile=`echo $i | sed 's/ldat/pack/'`
  echo "INDEX: $indexfile"
  ldat --extract $i INDEX
  mv INDEX $indexfile
done
