let g:netrw_liststyle=2
let g:netrw_sort_sequence='[\/]$,\<core\%(\.\d\+\)\=\>,\.[ch]$,\.cpp$,*,\.o$,\.obj$,\.info$,\.swp$,\.bak$,\~$'
let g:netrw_list_hide='\.\(o\|lo\|la\|m4\|guess\|log\|sub\)$,^Makefile$'

" Sets the environment variables
let g:build_prefix = $PWD . "/build"
let g:run_env_vars = 'PATH=gebrd:$PATH LD_LIBRARY_PATH='
let g:run_env_vars .= 'libgebr/.libs:'
let g:run_env_vars .= 'libgebr/geoxml/.libs:'
let g:run_env_vars .= 'libgebr/comm/.libs:'
let g:run_env_vars .= 'libgebr/gui/.libs:'
let g:run_env_vars .= 'libgebr/json/.libs'

let g:run_commands = []
let g:debug_commands = []

fun! AddGebrCommand(name, cmd, killd, label, debug)
  if a:killd
    let cmd = 'killall gebrd lt-gebrd 2> /dev/null ; '.g:run_env_vars.' '
  else
    let cmd = g:run_env_vars.' '
  endif
  let cmd .= 'gnome-terminal -e "'.a:cmd.'"'
  let g:run_commands += [[a:name, cmd]]
  exe "menu <silent> &Run." . a:label . " :call system('".cmd."')<cr>"
  if a:debug
    if a:killd
      let cmd = 'killall gebrd lt-gebrd 2> /dev/null ; '
    else
      let cmd = ''
    endif
    let cmd .= g:run_env_vars.' gnome-terminal -e "libtool --mode execute gdb '.a:cmd.'"'
    let g:debug_commands += [[a:name, cmd]]
    exe "menu <silent> &Debug." . a:label . " :call system('".cmd."')<cr>"
  endif
endf

call AddGebrCommand('gebr', 'gebr/gebr &', 1, '&Gebr\ (Kill\ Daemon)', 1)
call AddGebrCommand('gebr', 'gebr/gebr &', 0, 'Gebr', 1)
call AddGebrCommand('gebrd', 'gnome-terminal -e "gebrd/gebrd -i" &', 1, 'Gebr&d\ (Interactive)', 0)
call AddGebrCommand('debr', 'debr/debr &', 0, 'D&ebr', 1)

fun! RunWhat()
  let entries = ['Run:']
  let i = 1
  for [target,cmd] in g:run_commands
    let entries += [i.'. '.target]
    let i += 1
  endfor
  let ind = inputlist(entries)
  if ind > 0 && ind < i
    call system(g:run_commands[ind-1][1])
  endif
endf

fun! DebugWhat()
  let entries = ['Debug:']
  let i = 1
  for [target,cmd] in g:debug_commands
    let entries += [i.'. '.target]
    let i += 1
  endfor
  let ind = inputlist(entries)
  if ind > 0 && ind < i
    call system(g:debug_commands[ind-1][1])
  endif
endf

nmap <F12> :call RunWhat()<CR>
nmap <S-F12> :call DebugWhat()<CR>
