"colorscheme desert

set nocompatible              " be iMproved, required
filetype off                  " required

" should manually install Vundle.vim first
" $ git clone https://github.com/gmarik/Vundle.vim.git ~/.vim/bundle/Vundle.vim
" set the runtime path to include Vundle and initialize
 set rtp+=~/.vim/bundle/Vundle.vim
 call vundle#begin()
" " alternatively, pass a path where Vundle should install plugins
" "call vundle#begin('~/some/path/here')
"
" " let Vundle manage Vundle, required
Plugin 'gmarik/Vundle.vim'
" Plugin 'neilagabriel/vim-geeknote'
Plugin 'bling/vim-airline'
Plugin 'tomasr/molokai'
Plugin 'vim-scripts/winmanager--Fox'
Plugin 'vim-scripts/taglist.vim'
Plugin 'scrooloose/nerdtree'
Plugin 'scrooloose/nerdcommenter'
Plugin 'Shougo/neocomplete.vim'
Plugin 'kien/ctrlp.vim'
Plugin 'fatih/vim-go'
Plugin 'chazy/cscope_maps'
Plugin 'godlygeek/tabular'
Plugin 'plasticboy/vim-markdown'
Plugin 'hynek/vim-python-pep8-indent'
"
"
" " The following are examples of different formats supported.
" " Keep Plugin commands between vundle#begin/end.
" " plugin on GitHub repo
" Plugin 'tpope/vim-fugitive'
" " plugin from http://vim-scripts.org/vim/scripts.html
" Plugin 'L9'
" " Git plugin not hosted on GitHub
" Plugin 'git://git.wincent.com/command-t.git'
" " git repos on your local machine (i.e. when working on your own plugin)
" Plugin 'file:///home/gmarik/path/to/plugin'
" " The sparkup vim script is in a subdirectory of this repo called vim.
" " Pass the path to set the runtimepath properly.
" Plugin 'rstacruz/sparkup', {'rtp': 'vim/'}
" " Avoid a name conflict with L9
" Plugin 'user/L9', {'name': 'newL9'}
"
" " All of your Plugins must be added before the following line
 call vundle#end()            " required
 filetype plugin indent on    " required
" " To ignore plugin indent changes, instead use:
" "filetype plugin on
" "
" " Brief help
" " :PluginList       - lists configured plugins
" " :PluginInstall    - installs plugins; append `!` to update or just
" :PluginUpdate
" " :PluginSearch foo - searches for foo; append `!` to refresh local cache
" " :PluginClean      - confirms removal of unused plugins; append `!` to
" auto-approve removal
" "
" " see :h vundle for more details or wiki for FAQ
" " Put your non-Plugin stuff after this line

" tomasr/molokai
colorscheme molokai
let g:molokai_original = 1
let g:rehash256 = 1

" airline
" Add powerline patched fonts
" git clone https://github.com/powerline/fonts.git && cd fonts && ./install.sh
let g:airline_powerline_fonts = 1
" set guifont=Ubuntu\ Mono\ derivative\ Powerline\ 10
" set guifont=Literation\ Mono\ Powerline\ 9
" set guifont=Dejavu\ Sans\ Mono\ for\ Powerline\ 10
set guifont=Dejavu\ Sans\ Mono\ for\ Powerline\ 9
" set guifont=Sauce\ Code\ Powerline\ 9
" set guifont=Anonymice\ Powerline\ 9
" set guifont=Anonymice\ Powerline\ 9
" set guifont=Monofur\ for\ Powerline\ 9
" set guifont=Cousine\ for\ Powerline\ 9
let g:airline_detect_paste=1
let g:airline#extensions#tabline#enabled = 1
let g:airline#extensions#tabline#enabled = 1
let g:airline#extensions#tabline#show_buffers = 1
let g:airline#extensions#tabline#buffer_nr_show = 1
let g:airline#extensions#tabline#show_tabs = 1
let g:airline#extensions#tabline#show_tab_nr = 1
let g:airline#extensions#tabline#buffer_idx_mode = 1
nmap <leader>1 <Plug>AirlineSelectTab1
nmap <leader>2 <Plug>AirlineSelectTab2
nmap <leader>3 <Plug>AirlineSelectTab3
nmap <leader>4 <Plug>AirlineSelectTab4
nmap <leader>5 <Plug>AirlineSelectTab5
nmap <leader>6 <Plug>AirlineSelectTab6
nmap <leader>7 <Plug>AirlineSelectTab7
nmap <leader>8 <Plug>AirlineSelectTab8
nmap <leader>9 <Plug>AirlineSelectTab9
let g:airline#extensions#tabline#show_close_button = 1
let g:airline#extensions#tabline#close_symbol = 'X'
let g:airline#extensions#tagbar#enabled = 1
let g:airline#extensions#tagbar#flags = 'f'

" neocomplete
let g:neocomplete#enable_at_startup = 1

" ctrlp setup
let g:ctrlp_working_path_mode = 'a'

" vim-go
let g:go_highlight_functions = 1
let g:go_highlight_methods = 1
let g:go_highlight_structs = 1
let g:go_highlight_operators = 1
let g:go_highlight_build_constraints = 1

" vim-markdown
let g:vim_markdown_folding_disabled=1
let g:vim_markdown_math=1
let g:vim_markdown_frontmatter=1


" normal setup
set gcr=a:block-blinkon0
set nu
"if has("gui_running")
"  set guifont=Monospace\ 9
"endif
if &t_Co > 2 || has("gui_running")
  set hlsearch
  set incsearch
  set guioptions-=m
  set guioptions-=T
  "set lines=999 columns=999
endif
set tw=0 " do not break my line
set colorcolumn=80
"set spell " enable spell check, disable by set nospell

" for mac slow scroll
set lazyredraw
set ttyfast

" Just set encodings for chinese
set fileencodings=utf-8,chinese
