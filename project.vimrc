let g:netrw_liststyle=2
let g:netrw_sort_sequence='[\/]$,\<core\%(\.\d\+\)\=\>,\.[ch]$,\.cpp$,*,\.o$,\.obj$,\.info$,\.swp$,\.bak$,\~$'
let g:netrw_list_hide='\.\(o\|lo\|la\|m4\|guess\|log\|sub\)$,^Makefile$'

nmap <C-F6> :call UpdateTags(ProjectVimrc_GetProjectDir())<CR>
