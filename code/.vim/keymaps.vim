let mapleader = ','

" core
nnoremap <silent> <Bs> :bd<Cr>
nnoremap <silent> <Esc><Esc> :nohlsearch<Cr>
nnoremap <silent> <leader>n :set relativenumber!<Cr>
nnoremap <silent> Q <Nop>
nnoremap <silent> j gj
nnoremap <silent> k gk
noremap <silent> ; :

" nerdtree
nnoremap <silent> <leader>t :NERDTreeToggle<Cr>

" vim-argwrap
nnoremap <silent> <leader>a :ArgWrap<Cr>

" vim-easy-align
vmap <Cr> <Plug>(EasyAlign)

" open-browser.vim
nmap <leader>o <Plug>(openbrowser-smart-search)
vmap <leader>o <Plug>(openbrowser-smart-search)
