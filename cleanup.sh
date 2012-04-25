#!/bin/bash
shopt -s nullglob
shopt -s extglob
signal=${2--15}

function killg() {
echo Killing $1...
for lock in $HOME/.gebr/$1/*/lock; do
    host=`basename ${lock%lock}`;
    echo -en "$host\t"
    ssh $host fuser -sk $signal '$(cat '$lock')/tcp' \; killall -q mpich-hello
    if [ $? -eq 1 ]; then
	echo -e "CRASH"; rm -rf $lock
    else
	echo -e "OK"
    fi
done
}

sync

case x$1 in
    xgebrd | xd)
      killg gebrd
      ;;
    xgebrm | xm)
      killg gebrm
      ;;
    x-h | xhelp)
      echo "Usage: [-h|gebrd|gebrm|all|ALL]"
      ;;
    xall | x)
      killg gebrd
      killg gebrm
      ;;
    xALL)
      killg gebrd
      killg gebrm
      [ -d ~/.gebr ] && mv ~/.gebr ~/.gebr-`date +%Y%m%d-%H%M%S`
      ;;
esac
