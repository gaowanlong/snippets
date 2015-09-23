#!/bin/bash

[ "$PWD" = "/git/snippets/muttrc" ] || {
	echo "WARN: avoid removing wrong directory!"
	exit 1
}

rm -rf .??*

mkdir -p .getmail
mkdir -p .mutt-rc

cp -av ~/.getmail/getmailrc.* .getmail/
cp -av ~/.getmail/getmail.sh .getmail/
cp -av ~/.procmailrc .

cp -av ~/.msmtprc .

cp -av ~/.muttrc .
cp -av ~/.mutt-rc/*.muttrc .mutt-rc/
cp -av ~/.mailcap .

for i in $(find . -type f); do sed -i 's/\(^password \)\(= \)*.*/\1\2xxx/' $i; done
