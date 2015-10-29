#!/bin/sh
export PATH=/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin
#setlock -n ~/.mutt/getmail_gmail.lock getmail -n -r ~/.getmail/getmailrc.gmail >>~/.mutt/getmaillog 2>&1
#setlock -n ~/.mutt/getmail_allen_gmail.lock getmail -n -r ~/.getmail/getmailrc.allen_gmail >>~/.mutt/getmaillog 2>&1 &
timeout 300 setlock -n ~/.mutt/getmail_easystack.lock getmail -n -r ~/.getmail/getmailrc.easystack >>~/.mutt/getmaillog.easystack 2>&1
timeout 300 setlock -n ~/.mutt/getmail_allen_kernel.lock getmail -n -r ~/.getmail/getmailrc.allen_kernel >>~/.mutt/getmaillog.allen_kernel 2>&1
timeout 300 setlock -n ~/.mutt/getmail_outlook.lock getmail -n -r ~/.getmail/getmailrc.outlook >>~/.mutt/getmaillog.outlook 2>&1
