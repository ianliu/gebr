let g:netrw_liststyle=2
let g:netrw_sort_sequence='[\/]$,\<core\%(\.\d\+\)\=\>,\.[ch]$,\.cpp$,*,\.o$,\.obj$,\.info$,\.swp$,\.bak$,\~$'
let g:netrw_list_hide='\.\(o\|lo\|la\|m4\|guess\|log\|sub\)$,^Makefile$'

let g:build_prefix = $PWD . "/build"
let g:run_env_vars = 'PATH=gebrd:$PATH LD_LIBRARY_PATH=libgebr/.libs:libgebr/geoxml/.libs:libgebr/comm/.libs:libgebr/gui/.libs:libgebr/json/.libs'
let g:run_commands = []
let g:run_commands += [['gebr', 'killall gebrd lt-gebrd 2> /dev/null ; ' . g:run_env_vars . ' gebr/gebr &']]
let g:run_commands += [['debr', g:run_env_vars . 'debr/debr &']]
let g:run_commands += [['gebrd', 'killall gebrd lt-gebrd 2> /dev/null ; ' . g:run_env_vars . ' gebrd/gebrd &']]

fun! CompleteRun(A,L,P)
  let filter = []
  for [target,_] in g:run_commands
    if target =~? a:A
      let filter += [target]
    endif
  endfor
  return filter
endf

fun! RunWhat()
  let ind = input("Run (press <Tab> to complete): ", "", "customlist,CompleteRun")
  for [target,cmd] in g:run_commands
    if ind == target
      call system(cmd)
    endif
  endfor
endf

nmap <F12> :call RunWhat()<CR>
