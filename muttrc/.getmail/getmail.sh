#!/bin/sh
export PATH=/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin
setlock -n ~/.mutt/getmail_gmail.lock getmail -n -r ~/.getmail/getmailrc.gmail >>~/.mutt/getmaillog 2>&1
#setlock -n ~/.mutt/getmail_qq.lock getmail -n -r ~/.getmail/getmailrc.qq >>~/.mutt/getmaillog 2>&1
