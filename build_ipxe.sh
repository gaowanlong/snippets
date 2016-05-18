#!/bin/bash
# On RHEL7
# build ipxe-roms-qemu for qemu-2.5 which should support efi

yum install -y wget git rpm-build mtools syslinux genisoimage xz-devel \
	edk2-tools \
	binutils-devel \
	binutils-x86_64-linux-gnu \
	gcc-x86_64-linux-gnu

cd /tmp
git clone git://pkgs.fedoraproject.org/rpms/ipxe.git
mkdir -p ~/rpmbuild/SOURCES
wget --no-clobber -O ~/rpmbuild/SOURCES/ipxe-20150821-git4e03af8.tar.xz \
http://pkgs.fedoraproject.org/repo/pkgs/ipxe/ipxe-20150821-git4e03af8.tar.xz/0a4354afe007361980e45d5d5242f1ff/ipxe-20150821-git4e03af8.tar.xz
cp /tmp/ipxe/* ~/rpmbuild/SOURCES/
rpmbuild -bb /tmp/ipxe/ipxe.spec --define "dist .el7"

