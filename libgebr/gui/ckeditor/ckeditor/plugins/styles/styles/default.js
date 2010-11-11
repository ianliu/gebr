var styles = [{name:'Normal',element:'p'}];

if (menu_edition == false)
	styles.push({name:'Title',element:'h1'});

styles.push({name:'Section',element:'h2'},{name:'Subsection',element:'h3'});

if (menu_edition == true) {
	styles.push({name:'Optional parameter',element:'span',attributes:{'class':'label'}});
	styles.push({name:'Required parameter',element:'span',attributes:{'class':'reqlabel'}});
}

styles.push({name:'Code',element:'pre'});
CKEDITOR.stylesSet.add('default', styles);
