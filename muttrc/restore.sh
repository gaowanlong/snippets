#!/bin/bash

[[ "$PWD" =~ "git/snippets/muttrc" ]] || {
	echo "WARN: avoid removing wrong directory!"
	exit 1
}

cp -av .procmailrc ~/.procmailrc

cp -av .muttrc ~/.muttrc
mkdir -p ~/.mutt-rc
cp -av .mutt-rc/*.muttrc ~/.mutt-rc/
cp -av .mutt-rc/new-mail.sh ~/.mutt-rc/new-mail.sh
cp -av .mutt-rc/x-label-toggler ~/.mutt-rc/x-label-toggler
cp -av .mailcap ~/.mailcap

scp big:~/.msmtprc ~/.msmtprc
scp big:~/.mpoprc ~/.mpoprc
