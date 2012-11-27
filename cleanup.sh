#!/bin/bash
AUTH="$HOME/.ssh/authorized_keys"
shopt -s nullglob
shopt -s extglob
signal=${2--15}

function killd() {
echo Killing $1...
for lock in $HOME/.gebr/$1/*/lock; do
    host=`basename ${lock%lock}`;
    echo -en "$host\t"
    ssh $host killall -q mpich-hello \; fuser -sk $signal '$(cat '$lock')/tcp'
    if [ $? -eq 1 ]; then
	echo -e "CRASH"; rm -rf $lock
    else
	echo -e "OK"
    fi
done
}

function killm() {
    echo Killing $1...
    lock=$HOME/.gebr/$1/singleton_lock;
    [ -e $lock ] && _lock=`cat $lock` || return
    _lock=(${_lock//:/ });
    host=${_lock[0]};
    port=${_lock[1]};
    echo -en "$host\t"
    ssh $host killall -q mpich-hello \; fuser -sk $signal $port/tcp
    if [ $? -eq 1 ]; then
        echo -e "CRASH"; rm -rf $lock
    else
        echo -e "OK"
    fi
}

sync

if [ -f $AUTH.cleanup ]; then
	[ -f $AUTH ] && mv $AUTH $AUTH.cleanup.tmp
	mv $AUTH.cleanup $AUTH
	UNDO_AUTH=1
fi

case x$1 in
    xgebrd | xd)
      killd gebrd
      ;;
    xgebrm | xm)
      killm gebrm
      ;;
    x-h | xhelp)
      echo "Usage: [-h|gebrd|gebrm|all|ALL]"
      ;;
    xall | x)
      killd gebrd
      killm gebrm
      ;;
    xALL)
      killd gebrd
      killm gebrm
      [ -d ~/.gebr ] && mv ~/.gebr ~/.gebr-`date +%Y%m%d-%H%M%S`
      ;;
esac

if [ $UNDO_AUTH ]; then
	mv $AUTH $AUTH.cleanup
	[ -f $AUTH.cleanup.tmp ] && mv $AUTH.cleanup.tmp $AUTH
fi
