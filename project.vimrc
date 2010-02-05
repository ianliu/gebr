let my_ctags_options += ['--languages=c']

nmap <C-F6> :call UpdateTags(ProjectVimrc_GetProjectDir())<CR>
