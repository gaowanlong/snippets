#!/bin/bash

[ "$PWD" = "/git/snippets/muttrc" ] || {
	echo "WARN: avoid removing wrong directory!"
	exit 1
}

cp -av .procmailrc ~/.procmailrc

cp -av .muttrc ~/.muttrc
mkdir -p ~/.mutt-rc
cp -av .mutt-rc/*.muttrc ~/.mutt-rc/
cp -av .mailcap ~/.mailcap
