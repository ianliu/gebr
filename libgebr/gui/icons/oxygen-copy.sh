#!/bin/sh

ICONS=" \
	actions/chronometer \
	actions/document-open-recent \
	actions/document-import \
	actions/document-export \
	actions/document-save-all \
	actions/mail-queue \
	actions/edit-clear \
	actions/folder-new \
	actions/svn_add \
	actions/system-switch-user \
	actions/tab-new-background \
	actions/window-new \
	apps/accessories-dictionary"
SIZES="16x16 22x22 48x48"

for SIZE in $SIZES; do
	for ICON in $ICONS; do
		cp -v /usr/share/icons/oxygen/$SIZE/$ICON.png `echo $SIZE/$ICON.png | sed s/_/-/`
	done
done

#list files
for SIZE in $SIZES; do
	LIST=""
	for ICON in $ICONS; do
		LIST="$LIST `echo $SIZE/$ICON.png | sed s/_/-/`"
	done
	echo $LIST
done
