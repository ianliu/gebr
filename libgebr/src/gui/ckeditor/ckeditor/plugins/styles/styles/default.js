var styles = [{name:'Normal',element:'p'}];
if (typeof(menu_edition) == 'undefined')
	styles.push({name:'Title',element:'h1'});
styles.push({name:'Section',element:'h2'},{name:'Subsection',element:'h3'});
if (typeof(menu_edition) != 'undefined') {
	styles.push({name:'Optional parameter',element:'span',attributes:{'class':'label'}});
	styles.push({name:'Required parameter',element:'span',attributes:{'class':'reqlabel'}});
}
styles.push({name:'Code',element:'pre'});
CKEDITOR.stylesSet.add('default', styles);
