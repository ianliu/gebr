#!/bin/bash
shopt -s nullglob
shopt -s extglob
signal=${2--15}

function kgebrd() {
echo Killing gebrd...
for lock in $HOME/.gebr/run/!(*fslock.run); do
    server=`basename ${lock/*-} .run`;
    echo -en "$server\t"
    ssh $server fuser -sk $signal '$(cat '$lock')/tcp';
    sync
    if [ -e $lock ]; then
	echo -e "CRASH"; rm -rf $lock
    else
	echo -e "OK"
    fi
done
}

function kgebrm() {
echo Killing gebrm...
for lock in $HOME/.gebr/gebrm/*/lock; do
    maestro=`basename ${lock%lock}`;
    echo -en "$maestro\t"
    ssh $maestro fuser -sk $signal '$(cat '$lock')/tcp'
    if [ -e $lock ]; then
	echo -e "CRASH"; rm -rf $lock
    else
	echo -e "OK"
    fi
done
}

case $1 in
    gebrd)
      kgebrd
      ;;
    gebrm)
      kgebrm
      ;;
    all)
      kgebrd
      kgebrm
      ;;
    *)
      echo "Usage: [gebrd|gebrm|all]"
esac
