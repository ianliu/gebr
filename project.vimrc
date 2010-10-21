let g:netrw_liststyle=2
let g:netrw_list_hide='\.\(in\|o\|lo\|la\|in\|m4\|guess\|log\|sub\)$,^Makefile$'

nmap <C-F6> :call UpdateTags(ProjectVimrc_GetProjectDir())<CR>
