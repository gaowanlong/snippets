#!/bin/sh

find . -name "*.h" -o -name "*.c"-o -name "*.cc" -o -name "*.cpp" -o -name "*.hpp" > cscope.files
cscope -bkq -i cscope.files
ctags -R --c++-kinds=+p --fields=+iaS --extra=+q .
