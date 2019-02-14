#!/usr/bin/env bash

for f in tests/*.txt; do
  echo ">----------------------<"
  echo "  Launch '$f'"
  ./sdriver.pl -t $f -s ./shell
done