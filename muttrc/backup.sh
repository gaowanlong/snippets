#!/bin/bash

[ "$PWD" = "/git/snippets/muttrc" ] || {
	echo "WARN: avoid removing wrong directory!"
	exit 1
}

rm -rf .??*

mkdir -p .mutt-rc

cp -av ~/.procmailrc .

cp -av ~/.mpoprc .
cp -av ~/.msmtprc .

cp -av ~/.muttrc .
cp -av ~/.mutt-rc/*.muttrc .mutt-rc/
cp -av ~/.mutt-rc/new-mail.sh .mutt-rc/
cp -av ~/.mutt-rc/x-label-toggler .mutt-rc/
cp -av ~/.mailcap .

for i in $(find . -type f); do sed -i 's/\(^password \)\(= \)*.*/\1\2xxx/' $i; done
