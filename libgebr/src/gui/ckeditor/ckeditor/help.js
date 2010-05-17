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
var editor = null;
var editing_element = null;
function UpdateDocumentClone() {
	editor.updateElement();
	var content = GetEditableElements(document_clone)[0];
	while (content.firstChild)
		content.removeChild(content.firstChild);
	for (var i = GetEditableElements(document)[0].firstChild; i; i = i.nextSibling)
		content.appendChild(i.cloneNode(true));
}
function ToggleVisible(element) {
	var hidden = element.style.visibility == 'hidden';
	if (hidden) {
		element.style.visibility = 'visible';
		element.style.display = 'block';
	} else {
		element.style.visibility = 'hidden';
		element.style.display = 'none';
	}
}
function GetEditableElements(doc) {
	var content = doc.getElementsByClassName('content')[0];
	if (!menu_edition && !content) {
		content = doc.createElement('div');
		content.setAttribute('class', 'content');
		content.innerHTML = doc.body.innerHTML;
		doc.body.innerHTML = '';
		doc.body.appendChild(content);
	}
	return [content];
}
function UpgradeHelpFormat(doc) {
	var content = GetEditableElements(doc)[0];
	var links = content.getElementsByTagName('a');
	var blacklist = [];
	for (var i = 0; i < links.length; i++)
		if (links[i].innerHTML.search(/^\s*$/) == 0)
			blacklist.push(links[i]);
	for (var i = 0; i < blacklist.length; i++)
		blacklist[i].parentNode.removeChild(blacklist[i]);

	GenerateNavigationIndex(doc);
}
function GenerateNavigationIndex(doc) {
	var navbar = doc.getElementsByClassName('navigation')[0];
	if (!navbar) return;
	var headers = GetEditableElements(doc)[0].getElementsByTagName('h2');
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
/**
 * Get a list of title-link pairs and generate an index on \p doc.
 */
function GenerateLinksIndex2(doc, links) {
	var linkbar = doc.getElementsByClassName('links')[0];
	if (!linkbar) {
		linkbar = doc.createElement('div');
		linkbar.setAttribute('class', 'links');
		var navbar = doc.getElementsByClassName('navigation')[0];
		navbar.parentNode.insertBefore(linkbar, navbar.nextSibling);
	}
	linkbar.innerHTML = '<h2>Links</h2><ul></ul>';
	var linklist = linkbar.getElementsByTagName('ul')[0];
	if (!linklist)
		return;
	for (var i = 0; i < links.length; ++i) {
		var link = doc.createElement('a');
		link.setAttribute('href', links[i][1]);
		link.innerHTML = links[i][0];
		var li = doc.createElement('li');
		li.appendChild(link);
		linklist.appendChild(li);
	}
}
/**
 * Use GenerateLinksIndex above to generate indexes for \p document and \p document_clone.
 */
function GenerateLinksIndex(links) {
	GenerateLinksIndex2(document, links);
	GenerateLinksIndex2(document_clone, links);
}
function OpenCkEditor(element) {
	if (editor) return;
	editing_element = element;
	editor = CKEDITOR.replace(element, {
		fullpage: true,
		height: 300,
		width: menu_edition ? 390 : '100%',
		resize_enabled: false,
		toolbarCanCollapse: false,
		toolbar:[['Source','-','Bold','Italic','Underline','-',
			'Subscript','Superscript','-','Undo','Redo'],'/',
			['NumberedList','BulletedList','Blockquote','Styles','-',
			'Link','Unlink','-','RemoveFormat','-','Find','Replace', '-']]});
}
function onCkEditorLoadFinished() {
	if (menu_edition) {
		UpgradeHelpFormat(document);
		UpgradeHelpFormat(document_clone);
	}
	OpenCkEditor(GetEditableElements(document)[0]);
}
function isContentSaved() {
	if (menu_refresh)
		return false;
	return !editor.checkDirty();
}
function forceUtf8() {
	var head = getHead(document);
	var meta = head.getElementsByTagName('meta');
	var black_list = [];
	for (var i = 0; i < meta.length; i++) {
		var attr = meta[i].getAttribute('http-equiv');
		if (attr && attr.toLowerCase() == 'content-type')
			black_list.push(meta[i]);
	}
	for (var i = 0; i < black_list.length; i++)
		black_list[i].parentNode.removeChild(black_list[i]);
	meta = document.createElement('meta');
	meta.setAttribute('http-equiv', 'Content-Type');
	meta.setAttribute('content', 'text/html; charset=UTF-8');
	if (head.firstChild) {
		head.insertBefore(meta, head.firstChild);
	} else {
		head.appendChild(meta);
	}
}

function closeEditor() {
	editor.resetDirty();
	UpdateDocumentClone();
	if (menu_edition) {
		GenerateNavigationIndex(document);
		GenerateNavigationIndex(document_clone);
	}
	return document_clone.documentElement.outerHTML;
}

function toggleEditor() {
	var content = GetEditableElements(document)[0];
	var cke_editor = document.getElementById('cke_' + editor.name);
	editor.updateElement();
	GenerateNavigationIndex(document);
	ToggleVisible(content);
	ToggleVisible(cke_editor);
}

function refreshEditor() {
	UpdateDocumentClone();
	return document_clone.documentElement.outerHTML;
}

helpJSFinished();
