#!/bin/sh

ICONS=" \
	actions/document-import \
	actions/document-export \
	actions/edit-clear \
	actions/folder-new \
	actions/system-switch-user \
	actions/tab-new-background"
SIZES="16x16 22x22 48x48"

for SIZE in $SIZES; do
	for ICON in $ICONS; do
		cp -v /usr/share/icons/oxygen/$SIZE/$ICON.png $SIZE/$ICON.png
	done
done

#list files
for SIZE in $SIZES; do
	LIST=""
	for ICON in $ICONS; do
		LIST="$LIST $SIZE/$ICON.png"
	done
	echo $LIST
done
