autocmd FileType * setlocal formatoptions-=c formatoptions-=r formatoptions-=o

function! s:unalign() range
    for l:line in range(a:firstline, a:lastline)
        let l:text = getline(l:line)
        let l:subst = substitute(l:text, '\(\S\+\)\s\{2,}', '\1 ', 'g')
        call setline(l:line, l:subst)
    endfor
endfunction

command! -range UnAlign <line1>,<line2>call s:unalign()
