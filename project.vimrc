let NERDTreeIgnore = ['\.o$', '\.in$', '^Makefile$', '\.cache$']
nmap <C-F6> :call UpdateTags(ProjectVimrc_GetProjectDir())<CR>

"silent call UpdateTags(ProjectVimrc_GetProjectDir())
