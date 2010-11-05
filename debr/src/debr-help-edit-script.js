/*   libgebr - GeBR Library
 *   Copyright (C) 2007-2009 GeBR core team (http://www.gebrproject.com/)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//==============================================================================
// PRIVATE VARIABLES: meant to be used in this file only		       =
//==============================================================================

/*
 * Holds the singleton editor instance.
 */
var editor = null;

/*
 * The tool bar definition for the CKEditor instance.
 */
var toolbar = [
	['Source'],
	['Bold','Italic','Underline'],
	['Subscript','Superscript'],
	['Undo','Redo'],
	['JustifyLeft','JustifyCenter','JustifyRight','JustifyBlock'],
	['NumberedList','BulletedList'],
	['Outdent','Indent','Blockquote','Styles'],
	['Link','Unlink'],
	['RemoveFormat'],
	['Find','Replace','Table']
];

/*
 * Tells we are editing a menu or program.
 * Needed by styles.js file in CKEditor.
 */
var menu_edition = true;

//==============================================================================
// UTILITY FUNCTIONS: meant to be used in this file only		       =
//==============================================================================

/*
 * removeChildren:
 * Removes all children elements from @node.
 */
function removeChildren(node) {
	while (node.firstChild)
		node.removeChild(node.firstChild);
}

/*
 * getEditableElement:
 * Returns the div which will be replaced by the CKEditor instance.
 */
function getEditableElement(doc) {
	return doc.getElementsByClassName('content')[0];
}

//==============================================================================
// PRIVATE FUNCTIONS: meant to be used in this file only		       =
//==============================================================================
document.oncontextmenu = function() {
	return false;
}

document.onclick = function() {
	return false;
}

/**
 * generateNavigationIndex:
 * Generate the navigation index menu with a link to all headers from @doc.
 */
function generateNavigationIndex(doc) {
	var navbar = doc.getElementsByClassName('navigation')[0];
	if (!navbar) return;
	var headers = getEditableElement(doc).getElementsByTagName('h2');
	navbar.innerHTML = '<h2>Index</h2><ul></ul>';
	var navlist = navbar.getElementsByTagName('ul')[0];
	for (var i = 0; i < headers.length; i++) {
		var anchor = 'header_' + i;
		var link = doc.createElement('a');
		link.setAttribute('href', '#' + anchor);
		for (var j = 0; j < headers[i].childNodes.length; j++) {
			var clone = headers[i].childNodes[j].cloneNode(true);
			link.appendChild(clone);
		}
		var li = doc.createElement('li');
		li.appendChild(link);
		navlist.appendChild(li);
		headers[i].setAttribute('id', anchor);
	}
}
/*
 * upgradeHelpFormat:
 * Removes all anchors from @doc.
 */
function upgradeHelpFormat(doc) {
	var content = getEditableElement(doc);
	var links = content.getElementsByTagName('a');
	var blacklist = [];
	for (var i = 0; i < links.length; i++)
		if (links[i].innerHTML.search(/^\s*$/) == 0)
			blacklist.push(links[i]);
	for (var i = 0; i < blacklist.length; i++)
		blacklist[i].parentNode.removeChild(blacklist[i]);

	generateNavigationIndex(doc);
}

/*
 * openCKEditor:
 * Replaces @element with a CKEditor instance. If the CKEditor was already opened, do nothing.
 */
function openCKEditor(element) {
	if (editor) return;
	editor = CKEDITOR.replace(element, {
		height: 300,
		width: '76%',
		resize_enabled: false,
		toolbarCanCollapse: false,
		toolbar:toolbar
	});
}

/*
 * updateDocumentClone:
 * Updates the clone document with the edited data.
 */
function updateDocumentClone() {
	editor.updateElement();
	var content = getEditableElement(document_clone);
	while (content.firstChild)
		content.removeChild(content.firstChild);
	for (var i = getEditableElement(document).firstChild; i; i = i.nextSibling)
		content.appendChild(i.cloneNode(true));
}

/*
 * onCkEditorLoadFinished:
 * Called when CKEditor finishes loading.
 * This function is added by hand into the end of ckeditor.js script.
 */
function onCkEditorLoadFinished() {
	upgradeHelpFormat(document);
	upgradeHelpFormat(document_clone);
	openCKEditor(getEditableElement(document));
}

//==============================================================================
// PUBLIC FUNCTIONS: can be used externaly				       =
//==============================================================================

/*
 * checkEditorDirty:
 * Proxy function for CKEDITOR.editor.checkDirty function.
 *
 * Returns: true if the content has been changed, false otherwise.
 */
function checkEditorDirty() {
	return editor.checkDirty();
}

/*
 * resetEditorDirty:
 * Proxy function for CKEDITOR.editor.resetDirty function.
 */
function resetEditorDirty() {
	editor.resetDirty();
}

/*
 * getEditorContent:
 * Returns: The hole HTML of the help being edited.
 */
function getEditorContent() {
	updateDocumentClone();
	generateNavigationIndex(document);
	generateNavigationIndex(document_clone);
	return document_clone.documentElement.outerHTML;
}
