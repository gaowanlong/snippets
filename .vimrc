if v:lang =~ "utf8$" || v:lang =~ "UTF-8$"
   set fileencodings=utf-8,gb2312,gb18030,gbk,ucs-bom,cp936,latin1
endif

set bs=indent,eol,start		" allow backspacing over everything in insert mode
"set ai			" always set autoindenting on
"set backup		" keep a backup file
set viminfo='20,\"50	" read/write a .viminfo file, don't store more
			" than 50 lines of registers
set history=50		" keep 50 lines of command line history
set ruler		" show the cursor position all the time

" Only do this part when compiled with support for autocommands
if has("autocmd")
  augroup fedora
  autocmd!
  " In text files, always limit the width of text to 78 characters
  " autocmd BufRead *.txt set tw=78
  " When editing a file, always jump to the last cursor position
  autocmd BufReadPost *
  \ if line("'\"") > 0 && line ("'\"") <= line("$") |
  \   exe "normal! g'\"" |
  \ endif
  " don't write swapfile on most commonly used directories for NFS mounts or USB sticks
  autocmd BufNewFile,BufReadPre /media/*,/run/media/*,/mnt/* set directory=~/tmp,/var/tmp,/tmp
  " start with spec file template
  autocmd BufNewFile *.spec 0r /usr/share/vim/vimfiles/template.spec
  augroup END
  autocmd FileType cpp setlocal shiftwidth=2 tabstop=8 smarttab
  "autocmd FileType c setlocal shiftwidth=4 tabstop=4 softtabstop=4 expandtab smarttab
  autocmd FileType java setlocal shiftwidth=4 tabstop=4 softtabstop=4 expandtab
  autocmd FileType c setlocal shiftwidth=8 tabstop=8 noexpandtab
  "autocmd FileType cpp setlocal shiftwidth=8 tabstop=8 noexpandtab

endif

" Switch syntax highlighting on, when the terminal has colors
" Also switch on highlighting the last used search pattern.
if &t_Co > 2 || has("gui_running")
  syntax on
  set hlsearch
endif

filetype plugin on

if &term=="xterm"
     set t_Co=8
     set t_Sb=[4%dm
     set t_Sf=[3%dm
endif

" Don't wake up system with blinking cursor:
" http://www.linuxpowertop.org/known.php
let &guicursor = &guicursor . ",a:blinkon0"
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
Plugin 'Vundle/Vundle.vim'
" Plugin 'neilagabriel/vim-geeknote'
Plugin 'vim-airline/vim-airline'
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
"Plugin 'Rip-Rip/clang_complete'
Plugin 'mhinz/vim-grepper'
Plugin 'octol/vim-cpp-enhanced-highlight'
Plugin 'morhetz/gruvbox'
Plugin 'gaowanlong/delimitMate'
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
let g:airline#extensions#syntastic#enabled = 1

" neocomplete
let g:neocomplete#enable_at_startup = 1

" ctrlp setup
let g:ctrlp_working_path_mode = 'a'
let g:ctrlp_map = '<c-p>'
let g:ctrlp_cmd = 'CtrlP'
map <leader>f :CtrlPMRU<CR>
let g:ctrlp_custom_ignore = {
    \ 'dir':  '\v[\/]\.(git|hg|svn|rvm)$',
    \ 'file': '\v\.(exe|so|dll|zip|tar|tar.gz|pyc)$',
    \ }
let g:ctrlp_working_path_mode=0
let g:ctrlp_match_window_bottom=1
let g:ctrlp_max_height=15
let g:ctrlp_match_window_reversed=0
let g:ctrlp_mruf_max=500
let g:ctrlp_follow_symlinks=1

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

let delimitMate_expand_cr = 1

" grepper
nmap <leader>g :Grepper -tool git -open -switch -highlight -cword -noprompt<cr>
nmap <leader>G :Grepper -tool git -open -switch -highlight -query<cr>


" vim cpp inhance
let g:cpp_class_scope_highlight = 1
let g:cpp_experimental_template_highlight = 1

let g:rbpt_colorpairs = [
    \ ['brown',       'RoyalBlue3'],
    \ ['Darkblue',    'SeaGreen3'],
    \ ['darkgray',    'DarkOrchid3'],
    \ ['darkgreen',   'firebrick3'],
    \ ['darkcyan',    'RoyalBlue3'],
    \ ['darkred',     'SeaGreen3'],
    \ ['darkmagenta', 'DarkOrchid3'],
    \ ['brown',       'firebrick3'],
    \ ['gray',        'RoyalBlue3'],
    \ ['black',       'SeaGreen3'],
    \ ['darkmagenta', 'DarkOrchid3'],
    \ ['Darkblue',    'firebrick3'],
    \ ['darkgreen',   'RoyalBlue3'],
    \ ['darkcyan',    'SeaGreen3'],
    \ ['darkred',     'DarkOrchid3'],
    \ ['red',         'firebrick3'],
    \ ]

let g:rbpt_max = 16
let g:rbpt_loadcmd_toggle = 0
au VimEnter * RainbowParenthesesToggle
au Syntax * RainbowParenthesesLoadRound
au Syntax * RainbowParenthesesLoadSquare
au Syntax * RainbowParenthesesLoadBraces


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

" show lists
set listchars=eol:$,tab:<-,trail:~,extends:>,precedes:<
set list

" for mac slow scroll
set lazyredraw
set ttyfast

" Just set encodings for chinese
set fileencodings=utf-8,chinese
"set cursorline
"hi CursorLine   cterm=NONE ctermbg=darkred ctermfg=white guibg=darkred guifg=white
"set cursorcolumn
"hi CursorColumn cterm=NONE ctermbg=darkred ctermfg=white guibg=darkred guifg=white
"set mouse=a
colorscheme gruvbox
highlight WhitespaceEOL ctermbg=red guibg=red
match WhitespaceEOL /\s\+$/
set background=dark
""inoremap ' ''<ESC>i
""inoremap ''' ''<ESC>i
""inoremap " ""<ESC>i
""inoremap "" ""<ESC>i
""inoremap ( ()<ESC>i
""inoremap () ()<ESC>i
""inoremap [ []<ESC>i
""inoremap [] []<ESC>i
""inoremap { {<CR>}<ESC>O
""inoremap {} {<CR>}<ESC>O
"inoremap <C-h> <Left>
""inoremap <C-j> <Down>
""inoremap <C-k> <Up>

inoremap <C-l> <Right>
function! HiInterestingWord(n) " {{{2
	" Save our location.
	normal! mz
	" Yank the current word into the z register.
	normal! "zyiw
	" Calculate an arbitrary match ID.  Hopefully nothing else is using it.
	let mid = 77750 + a:n
	" Clear existing matches, but don't worry if they don't exist.
	"silent! call matchdelete(mid)
	try
		call matchdelete(mid)
	catch 'E803'
		" Construct a literal pattern that has to match at boundaries.
		let pat = '\V\<' . escape(@z, '\') . '\>'
		" Actually match the words.
		call matchadd("InterestingWord" . a:n, pat, 1, mid)
	endtry
	" Move back to our original location.
	normal! `z
endfunction 

"clear all highlighting
function! ClearAllHi()
	for i in range(1,6)
		let mid = 77750 + i
		silent! call matchdelete(mid)
	endfor
endfunction

nnoremap <silent> <leader>0 :call ClearAllHi()<cr>
nnoremap <silent> <leader>1 :call HiInterestingWord(1)<cr>
nnoremap <silent> <leader>2 :call HiInterestingWord(2)<cr>
nnoremap <silent> <leader>3 :call HiInterestingWord(3)<cr>
nnoremap <silent> <leader>4 :call HiInterestingWord(4)<cr>
nnoremap <silent> <leader>5 :call HiInterestingWord(5)<cr>
nnoremap <silent> <leader>6 :call HiInterestingWord(6)<cr>

hi def InterestingWord1 guifg=#000000 ctermfg=16 guibg=#ffa724 ctermbg=214
hi def InterestingWord2 guifg=#000000 ctermfg=16 guibg=#aeee00 ctermbg=154
hi def InterestingWord3 guifg=#000000 ctermfg=16 guibg=#8cffba ctermbg=121
hi def InterestingWord4 guifg=#000000 ctermfg=16 guibg=#b88853 ctermbg=137
hi def InterestingWord5 guifg=#000000 ctermfg=16 guibg=#ff9eb8 ctermbg=211
hi def InterestingWord6 guifg=#000000 ctermfg=16 guibg=#ff2c4b ctermbg=195
"}}}
