#!/bin/bash

if [ "x$1" != x--test-case ]; then
	echo "Usage: ./ssh.sh --test-case TESTCASE"
	echo "Test cases:"
	echo "  password       Asks for a password"
	echo "  fingerprint    Asks if you want to trust a fingerprint"
	echo "  error          Reports a fingerprint mismatch"
	exit 1
fi

case $2 in
	password)
		stty -echo
		printf "foo@localhost's password: "
		read PASSWORD
		stty echo
		printf "\n"
		;;
	fingerprint)
		printf "The authenticity of host 'localhost (::1)' can't be established.\n"
		printf "RSA key fingerprint is 00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00.\n"
		printf "Are you sure you want to continue connecting (yes/no)? "
		read RESPONSE
		;;
	error)
		echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
		echo "@    WARNING: REMOTE HOST IDENTIFICATION HAS CHANGED!     @"
		echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"
		echo "IT IS POSSIBLE THAT SOMEONE IS DOING SOMETHING NASTY!"
		echo "Someone could be eavesdropping on you right now (man-in-the-middle attack)!"
		echo "It is also possible that the RSA host key has just been changed."
		echo "The fingerprint for the RSA key sent by the remote host is"
		echo "00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00."
		echo "Please contact your system administrator."
		echo "Add correct host key in /home/ian/.ssh/known_hosts to get rid of this message."
		echo "Offending key in /home/ian/.ssh/known_hosts:20"
		echo "RSA host key for athena has changed and you have requested strict checking."
		echo "Host key verification failed."
		;;
esac
