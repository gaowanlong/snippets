Kernel development process
==========================

## Release big picture

v4.0-->v4.1-rc1...-->v4.1-rc8-->v4.2
(merge window two weeks)


## Patch life cycle

- Design
- early review -- mail list
- wider review -- -next tree
- rebase and rebase again -- low priority
- go into mainline
- stable release - if critical
- Longterm maintain


## Patch path

![kernel-dev model](http://7xky0k.com1.z0.glb.clouddn.com/dev-model.png)

0Day kernel performance testing
=============================

## summary

- why start this?
- do what? 
  - test latest kernel
  - bisect changes
  - report changes

- category
  - build(include code static analysis, smatch, sparse..)
  - boot
  - performance

- show report sample (from kernel mail list)

## the git tree covered

400+ trees included

- mainline (linus tree)
- maintainers tree (vfs, ext4, kvm, net...)
- developer's tree (...)

## git merge

		A---B---C topic
		/
	    D---E---F---G master

	TO=>

		A---B---C topic
		/         \
	    D---E---F---G---H master

## The old test way

test each rc releases, while issues:

- not in time, rc1 too many commits
- test machines are mostly idle


## what we want?

test and report TODAY\'s change => 0Day

- test latest code
- bisect - find the FIRST bad commit (and report to developer or mail lists)

## challenge of testing latest code

- only test mainline is not enough
- 400+ git trees => impossible to test one by one

## solution

git hourly merge =>

	v3.12-rc5
	|------------devel-hourly-2013101501
	|
	|------------devel-hourly-2013101502
	|
	|------------devel-hourly-2013101503
	|
	|------------devel-hourly-2013101504
	...

## howto bisect

how to find first bad commit:

---A---B---C---D---E---F---G---H---I---J---K HEAD

## benchmarks result

(refer to kernel mail list to see the pictures)
- O -- hourly merged branch HEAD
- * -- hourly merged branch BASE


## bisect challenge

- too many test each day, can not bisect one by one
- bisect take long time (2 - 10 hours, even more)

Should choose the bisect items

## choose factors:

- change percent
- variation

	variation = (X + Z) / (X + Y + Z)

- run time

	less run time, faster for bisect

- stats field weight

- timeliness

	today change > few days ago change


Score based priority: (by experience)

	change percent 0-20
	variation      0-10
	run time       0-10
	stats field weight 0-30
	timeliness     0-10
