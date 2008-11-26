#!/bin/sh

ICONS="actions/document-import"
SIZES="16x16 22x22 48x48"

for SIZE in $SIZES; do
	for ICON in $ICONS; do
		cp -v /usr/share/icons/oxygen/$SIZE/$ICON.png $SIZE/$ICON.png
	done
done

#list files
for SIZE in $SIZES; do
	for ICON in $ICONS; do
		echo $SIZE/$ICON.png
	done
done
