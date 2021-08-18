#!/usr/bin/env bash

# $Id$

export LANG=C
export LC_ALL=C

GIT_CWD=$1

try_git() {
	[ -n "GIT_CWD" ] || GIT_CWD="."
	git -C "$GIT_CWD" rev-parse --git-dir >/dev/null 2>&1 || return 1
	STAR=$(git -C "$GIT_CWD" status --porcelain 2>/dev/null)
	BRANCH=$(git -C "$GIT_CWD"  rev-parse --abbrev-ref HEAD 2>/dev/null)
	[ -n "$STAR" ] && STAR="*"
	CNT=$(git -C "$GIT_CWD" rev-list HEAD | wc -l | awk '{print $1}')
	let CNT=CNT-1 2>/dev/null || ((CNT=CNT-1)) 2>/dev/null
	HASH=$(git -C "$GIT_CWD" log -n 1 --format="%h")
	DATE=$(git -C "$GIT_CWD" log -n 1 --format="%cd" --date="format:%Y-%m-%d %H:%M" 2>/dev/null)
	REV="r$CNT-$HASH$STAR $BRANCH $DATE"
	[ -n "$HASH" ]
	
}

try_git || REV="Unknown"
echo "GIT Rev. $REV"
echo "#define VERSION \"$REV\"" > .git.h
