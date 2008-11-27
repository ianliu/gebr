#!/bin/sh

OXYGEN_STOCK_MAP=(
	gtk-about actions/rating
	gtk-add actions/list-add
	gtk-apply actions/dialog-ok-apply
	gtk-bold actions/format-text-bold
	gtk-cancel actions/dialog-cancel
	gtk-cdrom devices/drive-optical
	gtk-clear actions/edit-clear
	gtk-close actions/dialog-close
	gtk-color-picker actions/color-picker
	gtk-connect actions/network-connect
	gtk-convert actions/svn_switch
	gtk-copy actions/edit-copy
	gtk-cut actions/edit-cut
	gtk-delete actions/edit-delete
	gtk-dialog-authentication status/object-locked
	gtk-dialog-error status/dialog-error
	gtk-dialog-info status/dialog-information
	gtk-dialog-question categories/system-help
	gtk-dialog-warning status/dialog-warning
	gtk-directory places/folder
	gtk-discard _
	gtk-disconnect actions/network-disconnect
	gtk-dnd _
	gtk-dnd-multiple _
	gtk-edit actions/document-properties
	gtk-execute actions/run
	gtk-find actions/edit-find
	gtk-find-and-replace actions/edit-find
	gtk-floppy devices/media-floppy
	gtk-fullscreen actions/view-fullscreen
	gtk-go-back actions/go-previous
	gtk-go-down actions/go-down
	gtk-go-next actions/go-next
	gtk-go-up actions/go-up
	gtk-goto-bottom actions/go-bottom
	gtk-goto-first actions/go-first
	gtk-goto-last actions/go-last
	gtk-goto-top actions/go-top
	gtk-harddisk devices/drive-harddisk
	gtk-help actions/help-contents
	gtk-home actions/go-home
	gtk-stock-indent actions/format-indent-more
	gtk-info status/dialog-information
	gtk-italic actions/format-text-italic
	gtk-jump-to actions/go-jump
	gtk-justify-center actions/format-justify-center
	gtk-justify-fill actions/format-justify-fill
	gtk-justify-left actions/format-justify-left
	gtk-justify-right actions/format-justify-right
	gtk-leave-fullscreen actions/view-fullscreen
	gtk-media-forward actions/media-skip-forward
	gtk-media-pause actions/media-playback-pause
	gtk-media-play actions/media-playback-start
	gtk-media-previous actions/media-skip-backward
	gtk-media-record actions/media-record
	gtk-media-rewind actions/media-seek-backward
	gtk-media-stop actions/media-playback-stop
	gtk-network _
	gtk-no actions/dialog-cancel
	gtk-ok actions/dialog-ok
	gtk-open actions/document-open-folder
	gtk-orientation-landscape _
	gtk-orientation-portrait _
	gtk-orientation-reverse-landscape _
	gtk-orientation-reverse-portrait _
	gtk-page-setup _
	gtk-paste actions/edit-paste
	gtk-preferences actions/configure
	gtk-print actions/document-print
	gtk-print-error _
	gtk-print-paused _
	gtk-print-preview actions/document-preview
	gtk-print-report _
	gtk-print-warning _
	gtk-properties actions/document-properties
	gtk-quit actions/application-exit
	gtk-refresh actions/view-refresh
	gtk-remove actions/list-remove
	gtk-revert-to-saved actions/document-revert
	gtk-save actions/document-save
	gtk-save-as actions/document-save-as
	gtk-select-all actions/edit-select-all
	gtk-select-color actions/format-stroke-color
	gtk-select-font _
	gtk-sort-ascending actions/view-sort-ascending
	gtk-sort-descending actions/view-sort-descending
	gtk-spell-check actions/tools-check-spelling
	gtk-stop actions/process-stop
	gtk-strikethrough actions/format-text-strikethrough
	gtk-undelete _
	gtk-underline actions/format-text-underline
	gtk-undo actions/edit-undo
	gtk-unindent actions/format-indent-less
	gtk-yes actions/dialog-ok-apply
	gtk-zoom-100 actions/zoom-original
	gtk-zoom-fit actions/zoom-fit-best
	gtk-zoom-in actions/zoom-in
	gtk-zoom-out actions/zoom-out)
SIZES="16x16 22x22 48x48"

declare -i I

for SIZE in $SIZES; do
	for (( I=0; $I!=${#OXYGEN_STOCK_MAP[*]}; I=$I+2 )); do
		if test ${OXYGEN_STOCK_MAP[$I+1]} = "_"; then
			continue
		fi
		cp -v /usr/share/icons/oxygen/$SIZE/${OXYGEN_STOCK_MAP[$I+1]}.png $SIZE/stock/${OXYGEN_STOCK_MAP[$I]}.png
	done
done

#list files to add to Makefile.am
# for SIZE in $SIZES; do
# 	LIST=""
# 	for (( I=0; $I!=${#OXYGEN_STOCK_MAP[*]}; I=$I+2 )); do
# 		if test ${OXYGEN_STOCK_MAP[$I+1]} = "_"; then
# 			continue
# 		fi
# 		LIST="$LIST $SIZE/${OXYGEN_STOCK_MAP[$I]}.png"
# 	done
# 	echo $LIST
# done
