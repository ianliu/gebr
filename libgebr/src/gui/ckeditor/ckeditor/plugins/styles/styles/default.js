/*
Copyright (c) 2003-2010, CKSource - Frederico Knabben. All rights reserved.
For licensing, see LICENSE.html or http://ckeditor.com/license
*/
var styles = [{name:'Normal',element:'p'}];
if (!menu_edition)
	styles.push({name:'Title',element:'h1'});
styles.push({name:'Section',element:'h2'},{name:'Subsection',element:'h3'});
if (menu_edition)
	styles.push({name:'Parameter',element:'span',attributes:{'class':'label'}});
styles.push({name:'Code',element:'pre'});
CKEDITOR.stylesSet.add('default', styles);
