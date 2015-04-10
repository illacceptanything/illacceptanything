call plug#begin('~/.vim/plugged')

Plug 'https://github.com/FooSoft/vim-argwrap.git'
Plug 'https://github.com/Lokaltog/vim-easymotion.git'
Plug 'https://github.com/airblade/vim-gitgutter.git'
Plug 'https://github.com/antoyo/vim-licenses.git'
Plug 'https://github.com/bkad/CamelCaseMotion'
Plug 'https://github.com/bling/vim-airline.git'
Plug 'https://github.com/cespare/vim-toml.git'
Plug 'https://github.com/christoomey/vim-sort-motion.git'
Plug 'https://github.com/dhruvasagar/vim-table-mode.git'
Plug 'https://github.com/fatih/vim-go.git'
Plug 'https://github.com/inkarkat/argtextobj.vim.git'
Plug 'https://github.com/junegunn/vim-easy-align.git'
Plug 'https://github.com/kien/ctrlp.vim.git'
Plug 'https://github.com/mattn/emmet-vim.git'
Plug 'https://github.com/michaeljsmith/vim-indent-object.git'
Plug 'https://github.com/nathanaelkane/vim-indent-guides.git'
Plug 'https://github.com/ntpeters/vim-better-whitespace.git'
Plug 'https://github.com/othree/html5.vim.git'
Plug 'https://github.com/pangloss/vim-javascript.git'
Plug 'https://github.com/plasticboy/vim-markdown.git'
Plug 'https://github.com/scrooloose/nerdtree.git'
Plug 'https://github.com/tomasr/molokai.git'
Plug 'https://github.com/tpope/vim-abolish.git'
Plug 'https://github.com/tpope/vim-commentary.git'
Plug 'https://github.com/tpope/vim-eunuch.git'
Plug 'https://github.com/tpope/vim-fugitive.git'
Plug 'https://github.com/tpope/vim-repeat'
Plug 'https://github.com/tpope/vim-speeddating.git'
Plug 'https://github.com/tpope/vim-surround'
Plug 'https://github.com/tpope/vim-unimpaired.git'
Plug 'https://github.com/tyru/open-browser.vim.git'
Plug 'https://github.com/vim-latex/vim-latex.git'

if has('unix')
    Plug 'https://github.com/Valloric/YouCompleteMe.git', { 'do': './install.sh' }
    Plug 'https://github.com/marijnh/tern_for_vim', { 'do': 'npm install' }
    Plug 'https://github.com/scrooloose/syntastic.git', { 'do': 'sudo npm -g install jshint; sudo pip install pyflakes' }
endif

call plug#end()

" ctrlp.vim
let g:ctrlp_cmd = 'CtrlPMixed'

" syntastic
let g:syntastic_python_checkers = ['pyflakes']
let g:syntastic_always_populate_loc_list = 1

" vim-airline
let g:airline#extensions#tabline#enabled = 1

" vim-latex
let g:Tex_DefaultTargetFormat = 'pdf'
let g:Tex_MultipleCompileFormats = 'pdf, aux'

" vim-licenses
let g:licenses_authors_name = 'Alex Yatskov <alex@foosoft.net>'
let g:licenses_copyright_holders_name = g:licenses_authors_name

" vim-table-mode
let g:table_mode_corner = '|'
