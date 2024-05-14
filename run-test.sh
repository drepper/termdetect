#!/bin/bash

srcdir=$(dirname $0)
outfile=$(mktemp -p $PWD)

if type -p gnome-terminal > /dev/null; then
  echo 'gnome-terminal' > run-test.out
  echo '--------------' >> run-test.out
  gnome-terminal -- bash -c "cd $PWD && ./inittest > $outfile; exit"
  sleep 0.5
  sed 's/\(implementation version = \)[[:digit:]]\{1,\}\.[[:digit:]]\{1,\}/\1XXXXXX/' $outfile |
  sed 's/\(DA2=[^;]*;\)[[:digit:]]*/\1XXXXXX/' |
  sed 's/\(Q=VTE(\)[^)]*)[[:digit:]]*/\1XXXXXX)/' |
  sed 's/\(^columns * = \).*/\1CCC/' |
  sed 's/\(^rows * = \).*/\1RRR/' >> run-test.out
  echo -n > $outfile
fi

if type -p foot > /dev/null; then
  echo >> run-test.out
  echo 'foot' >> run-test.out
  echo '----' >> run-test.out
  foot bash -c "cd $PWD && ./inittest > $outfile; exit" 2> /dev/null
  sleep 0.1
  sed 's/\(implementation version = \)[[:digit:]]\{1,\}\.[[:digit:]]\{1,\}\.[[:digit:]]\{1,\}/\1XXXXXX/' $outfile |
  sed 's/\(DA2=[^;]*;\)[[:digit:]]*/\1XXXXXX/' |
  sed 's/\(Q=foot(\)[^)]*)[[:digit:]]*/\1XXXXXX)/' |
  sed 's/\(^columns * = \).*/\1CCC/' |
  sed 's/\(^rows * = \).*/\1RRR/' >> run-test.out
  echo -n > $outfile
fi

if type -p alacritty > /dev/null; then
  echo >> run-test.out
  echo 'alacritty' >> run-test.out
  echo '---------' >> run-test.out
  alacritty -e bash -c "cd $PWD && ./inittest > $outfile; exit" 2> /dev/null
  sleep 0.1
  sed 's/\(implementation version = \)[[:digit:]]\{1,\}\.[[:digit:]]\{1,\}\.[[:digit:]]\{1,\}/\1XXXXXX/' $outfile |
  sed 's/\(DA2=[^;]*;\)[[:digit:]]*;[[:digit:]]*/\1XXXX;XX/' |
  sed 's/\(^columns * = \).*/\1CCC/' |
  sed 's/\(^rows * = \).*/\1RRR/' >> run-test.out
  echo -n > $outfile
fi

rm -f $outfile

colordiff -u run-test.out ${srcdir:-.}/run-test.expect
