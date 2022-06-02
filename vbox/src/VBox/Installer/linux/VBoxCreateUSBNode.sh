#! /bin/sh
# $Id: VBoxCreateUSBNode.sh 34572 2010-12-01 14:23:21Z vboxsync $ */
## @file
# VirtualBox USB Proxy Service, Linux Specialization.
# udev helper for creating and removing device nodes for VirtualBox USB devices
#

#
# Copyright (C) 2010 Oracle Corporation
#
# Oracle Corporation confidential
# All rights reserved
#

do_remove=0
case "$1" in "--remove")
  do_remove=1; shift;;
esac
bus=`expr "$2" '/' 128 + 1`
device=`expr "$2" '%' 128 + 1`
class="$3"
group="$4"
if test "$class" -eq 9; then
  exit 0
fi
devdir="`printf "/dev/vboxusb/%.3d" $bus`"
devpath="`printf "/dev/vboxusb/%.3d/%.3d" $bus $device`"
if test "$do_remove" -eq 0; then
  if test -z "$group"; then
    group="vboxusers"
  fi
  mkdir /dev/vboxusb -m 0750 2>/dev/null
  chown root:$group /dev/vboxusb 2>/dev/null
  mkdir "$devdir" -m 0750 2>/dev/null
  chown root:$group "$devdir" 2>/dev/null
  mknod "$devpath" c $1 $2 -m 0660 2>/dev/null
  chown root:$group "$devpath" 2>/dev/null
else
  rm -f "$devpath"
fi

