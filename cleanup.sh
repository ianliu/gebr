#!/bin/bash
shopt -s nullglob
shopt -s extglob
signal=${2--15}

function killg() {
echo Killing $1...
for lock in $HOME/.gebr/$1/*/lock; do
    host=`basename ${lock%lock}`;
    echo -en "$host\t"
    ssh $host fuser -sk $signal '$(cat '$lock')/tcp'
    sync
    if [ -e $lock ]; then
	echo -e "CRASH"; rm -rf $lock
    else
	echo -e "OK"
    fi
done
}

sync

case x$1 in
    xgebrd)
      killg gebrd
      ;;
    xgebrm)
      killg gebrm
      ;;
    x-h)
      echo "Usage: [-h|gebrd|gebrm|all]"
      ;;
    xall | x)
      killg gebrd
      killg gebrm
      ;;
esac
