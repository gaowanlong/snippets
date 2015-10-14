#!/bin/bash
# usage: apply-redhat-patches.sh </path/to/patches> <start_num> <end_num>
# example: /path/to/apply-redhat-patches.sh ../v3.10-to-kernel-3.10.0-322.el7 0001 2800

for i in $(seq -w $2 $3); do
	patch=$1/$i-*
	echo Applying patch: $patch
	git am $patch || break
done
git am --abort
