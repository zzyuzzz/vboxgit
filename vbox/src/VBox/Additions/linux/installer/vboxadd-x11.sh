#! /bin/sh
# Sun VirtualBox
# Linux Additions X11 setup init script ($Revision: 24967 $)
#

#
# Copyright (C) 2006-2009 Sun Microsystems, Inc.
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#
# Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
# Clara, CA 95054 USA or visit http://www.sun.com if you need
# additional information or have any questions.
#


# chkconfig: 35 30 70
# description: VirtualBox Linux Additions kernel modules
#
### BEGIN INIT INFO
# Provides:       vboxadd-x11
# Required-Start:
# Required-Stop:
# Default-Start:
# Default-Stop:
# Description:    VirtualBox Linux Additions X11 setup
### END INIT INFO

PATH=$PATH:/bin:/sbin:/usr/sbin
LOG="/var/log/vboxadd-install-x11.log"
CONFIG_DIR="/var/lib/VBoxGuestAdditions"
CONFIG="config"

# Find the version of X installed
# The last of the three is for the X.org 6.7 included in Fedora Core 2
xver=`X -version 2>&1`
x_version=`echo "$xver" | sed -n 's/^X Window System Version \([0-9.]\+\)/\1/p'``echo "$xver" | sed -n 's/^XFree86 Version \([0-9.]\+\)/\1/p'``echo "$xver" | sed -n 's/^X Protocol Version 11, Revision 0, Release \([0-9.]\+\)/\1/p'``echo "$xver" | sed -n 's/^X.Org X Server \([0-9.]\+\)/\1/p'`
# Version of Redhat or Fedora installed.  Needed for setting up selinux policy.
redhat_release=`cat /etc/redhat-release 2> /dev/null`
# All the different possible locations for XFree86/X.Org configuration files
# - how many of these have ever been used?
x11conf_files="/etc/X11/xorg.conf /etc/X11/xorg.conf-4 /etc/X11/.xorg.conf \
    /etc/xorg.conf /usr/etc/X11/xorg.conf-4 /usr/etc/X11/xorg.conf \
    /usr/lib/X11/xorg.conf-4 /usr/lib/X11/xorg.conf /etc/X11/XF86Config-4 \
    /etc/X11/XF86Config /etc/XF86Config /usr/X11R6/etc/X11/XF86Config-4 \
    /usr/X11R6/etc/X11/XF86Config /usr/X11R6/lib/X11/XF86Config-4 \
    /usr/X11R6/lib/X11/XF86Config"

if [ -f /etc/arch-release ]; then
    system=arch
elif [ -f /etc/redhat-release ]; then
    system=redhat
elif [ -f /etc/SuSE-release ]; then
    system=suse
elif [ -f /etc/gentoo-release ]; then
    system=gentoo
elif [ -f /etc/lfs-release -a -d /etc/rc.d/init.d ]; then
    system=lfs
else
    system=other
fi

if [ "$system" = "arch" ]; then
    USECOLOR=yes
    . /etc/rc.d/functions
    fail_msg() {
        stat_fail
    }

    succ_msg() {
        stat_done
    }

    begin() {
        stat_busy "$1"
    }
fi

if [ "$system" = "redhat" ]; then
    . /etc/init.d/functions
    fail_msg() {
        echo_failure
        echo
    }
    succ_msg() {
        echo_success
        echo
    }
    begin() {
        echo -n "$1"
    }
fi

if [ "$system" = "suse" ]; then
    . /etc/rc.status
    fail_msg() {
        rc_failed 1
        rc_status -v
    }
    succ_msg() {
        rc_reset
        rc_status -v
    }
    begin() {
        echo -n "$1"
    }
fi

if [ "$system" = "gentoo" ]; then
    if [ -f /sbin/functions.sh ]; then
        . /sbin/functions.sh
    elif [ -f /etc/init.d/functions.sh ]; then
        . /etc/init.d/functions.sh
    fi
    fail_msg() {
        eend 1
    }
    succ_msg() {
        eend $?
    }
    begin() {
        ebegin $1
    }
    if [ "`which $0`" = "/sbin/rc" ]; then
        shift
    fi
fi

if [ "$system" = "lfs" ]; then
    . /etc/rc.d/init.d/functions
    fail_msg() {
        echo_failure
    }
    succ_msg() {
        echo_ok
    }
    begin() {
        echo $1
    }
fi

if [ "$system" = "other" ]; then
    fail_msg() {
        echo " ...fail!"
    }
    succ_msg() {
        echo " ...done."
    }
    begin() {
        echo -n $1
    }
fi

dev=/dev/vboxguest
userdev=/dev/vboxuser
owner=vboxadd
group=1

fail()
{
    if [ "$system" = "gentoo" ]; then
        eerror $1
        exit 1
    fi
    fail_msg
    echo "($1)"
    exit 1
}

# Install an X11 desktop startup application.  The application should be a shell script.
# Its name should be purely alphanumeric and should start with a two digit number (preferably
# 98 or thereabouts) to indicate its place in the Debian Xsession startup order.
#
# syntax: install_x11_startup_app app desktop service_name
install_x11_startup_app() {
    self="install_x11_startup_app"
    app_src=$1
    desktop_src=$2
    service_name=$3
    alt_command=$4
    test -r "$app_src" ||
        { echo >> $LOG "$self: no script given"; return 1; }
    test -r "$desktop_src" ||
        { echo >> $LOG "$self: no desktop file given"; return 1; }
    test -n "$service_name" ||
        { echo >> $LOG "$self: no service given"; return 1; }
    test -n "$alt_command" ||
        { echo >> $LOG "$self: no service given"; return 1; }
    app_dest=`basename $app_src sh`
    app_dest_sh=`basename $app_src sh`.sh
    desktop_dest=`basename $desktop_src`
    found=0
    x11_autostart="/etc/xdg/autostart"
    kde_autostart="/usr/share/autostart"
    redhat_dir=/etc/X11/Xsession.d
    mandriva_dir=/etc/X11/xinit.d
    debian_dir=/etc/X11/xinit/xinitrc.d
    if [ -d "$mandriva_dir" -a -w "$mandriva_dir" -a -x "$mandriva_dir" ]
    then
        install -m 0644 $app_src "$mandriva_dir/$app_dest"
        found=1
    fi
    if [ -d "$x11_autostart" -a -w "$x11_autostart" -a -x "$x11_autostart" ]
    then
        install -m 0644 $desktop_src "$x11_autostart/$desktop_dest"
        found=1
    fi
    if [ -d "$kde_autostart" -a -w "$kde_autostart" -a -x "$kde_autostart" ]
    then
        install -m 0644 $desktop_src "$kde_autostart/$desktop_dest"
        found=1
    fi
    if [ -d "$redhat_dir" -a -w "$redhat_dir" -a -x "$redhat_dir" ]
    then
        install -m 0644 $app_src "$redhat_dir/$app_dest"
        found=1
    fi
    if [ -d "$debian_dir" -a -w "$debian_dir" -a -x "$debian_dir" ]
    then
        install -m 0755 $app_src "$debian_dir/$app_dest_sh"
        found=1
    fi
    if [ $found -eq 1 ]; then
        return 0
    fi
    cat >> $LOG << EOF
Could not set up the $service_name desktop service.
To start it at log-in for a given user, add the command $alt_command
to the file .xinitrc in their home directory.
EOF
    return 1
}


start()
{
    # Todo: check configuration and correct it if necessary
    return 0
}

stop()
{
    return 0
}

restart()
{
    stop && start
    return 0
}

# setup_script
setup()
{
    echo "VirtualBox Guest Additions installation, Window System and desktop setup" > $LOG
    begin "Installing the Window System drivers"
    if [ "$ARCH" = "amd64" ]
    then
        LIB=/usr/lib64
    else
        LIB=/usr/lib
    fi
    lib_dir="$LIB/VBoxGuestAdditions"
    share_dir="/usr/share/VBoxGuestAdditions"
    test -x "$lib_dir" -a -x "$share_dir" ||
        fail "Invalid Guest Additions configuration found"
    # By default we want to configure X
    dox11config="true"
    # By default, we want to run our xorg.conf setup script
    setupxorgconf="true"
    # But without the workaround for SUSE 11.1 not doing input auto-detection
    newmouse=""
    # By default we want to use hal for auto-loading the mouse driver
    usehal="--useHal"
    # And on newer servers, we want to test whether dynamic resizing will work
    testrandr="true"
    # The video driver to install for X.Org 6.9+
    vboxvideo_src=
    # The mouse driver to install for X.Org 6.9+
    vboxmouse_src=

    modules_dir=`X -showDefaultModulePath 2>&1` || modules_dir=
    if [ -z "$modules_dir" ]; then
        for dir in /usr/lib64/xorg/modules /usr/lib/xorg/modules /usr/X11R6/lib64/modules /usr/X11R6/lib/modules /usr/X11R6/lib/X11/modules; do
            if [ -d $dir ]; then
                modules_dir=$dir
                break
            fi
        done
    fi

    test -z "$x_version" -o -z "$modules_dir" &&
        fail "Could not find the X.Org or XFree86 Window System."

    echo
    case $x_version in
        1.7.99.* )
            echo "Warning: unsupported pre-release version of X.Org Server installed.  Not"
            echo "installing the X.Org drivers."
            dox11config=""
            ;;
        1.6.99.* | 1.7.* )
            begin "Installing experimental X.Org Server 1.7 modules"
            vboxvideo_src=vboxvideo_drv_17.so
            vboxmouse_src=vboxmouse_drv_17.so
            setupxorgconf=""
            ;;
        1.5.99.* | 1.6.* )
            begin "Installing X.Org Server 1.6 modules"
            vboxvideo_src=vboxvideo_drv_16.so
            vboxmouse_src=vboxmouse_drv_16.so
            # SUSE with X.Org Server 1.6 knows about vboxvideo
            test "$system" = "suse" && setupxorgconf=""
            ;;
        1.4.99.* | 1.5.* )
            # Fedora 9 shipped X.Org Server version 1.4.99.9x (1.5.0 RC)
            # in its released version
            begin "Installing X.Org Server 1.5 modules"
            vboxvideo_src=vboxvideo_drv_15.so
            vboxmouse_src=vboxmouse_drv_15.so
            # SUSE with X.Org 1.5 is a special case, and is handled specially
            test -r /etc/SuSE-release &&
            { usehal=""; newmouse="--newMouse"; }
            ;;
        1.4.* )
            begin "Installing X.Org Server 1.4 modules"
            vboxvideo_src=vboxvideo_drv_14.so
            vboxmouse_src=vboxmouse_drv_14.so
            usehal=""
            ;;
        1.3.* )
            # This was the first release which gave the server version number
            # rather than the X11 release version when you did 'X -version'.
            begin "Installing X.Org Server 1.3 modules"
            vboxvideo_src=vboxvideo_drv_13.so
            vboxmouse_src=vboxmouse_drv_71.so
            usehal=""
            ;;
        7.1.* | 7.2.* )
            begin "Installing X.Org 7.1 modules"
            vboxvideo_src=vboxvideo_drv_71.so
            vboxmouse_src=vboxmouse_drv_71.so
            usehal=""
            testrandr=""
            ;;
        6.9.* | 7.0.* )
            begin "Installing X.Org 6.9/7.0 modules"
            vboxvideo_src=vboxvideo_drv_70.so
            vboxmouse_src=vboxmouse_drv_70.so
            usehal=""
            testrandr=""
            ;;
        6.7* | 6.8.* | 4.2.* | 4.3.* )
            # Assume X.Org post-fork or XFree86
            begin "Installing XFree86 4.2/4.3 and X.Org 6.7/6.8 modules"
            ln -s "$lib_dir/vboxvideo_drv.o" "$modules_dir/drivers/vboxvideo_drv.o"
            ln -s "$lib_dir/vboxmouse_drv.o" "$modules_dir/input/vboxmouse_drv.o"
            usehal=""
            testrandr=""
            succ_msg
            ;;
        * )
            echo "Warning: unknown version of the X Window System installed.  Not installing"
            echo "X Window System drivers."
            dox11config=""
            ;;
    esac
    if [ -n "$vboxvideo_src" -a -n "$vboxmouse_src" ]; then
        rm "$modules_dir/drivers/vboxvideo_drv.so" 2>/dev/null
        rm "$modules_dir/input/vboxmouse_drv.so" 2>/dev/null
        ln -s "$lib_dir/$vboxvideo_src" "$modules_dir/drivers/vboxvideo_drv.so"
        ln -s "$lib_dir/$vboxmouse_src" "$modules_dir/input/vboxmouse_drv.so" &&
        succ_msg
    fi
    if test -n "$testrandr"; then
        # Run VBoxRandR in test mode as it prints out useful information if
        # dynamic resizing can't be used.  Don't fail here though.
        /usr/bin/VBoxRandR --test 1>&2
    else
        cat << EOF

You appear to be running an older version of the X Window system in your
guest.  Seamless mode and dynamic resizing will not work!

EOF
    fi

    if test -n "$dox11config"; then
        begin "Setting up the Window System to use the Guest Additions"
        # Certain Ubuntu/Debian versions use a special PCI-id file to identify
        # video drivers.  Some versions have the directory and don't use it.
        # Those versions can autoload vboxvideo though, so we don't need to
        # hack the configuration file for them.
        test -f /etc/debian_version -a -d /usr/share/xserver-xorg/pci &&
        {
            rm -f "/usr/share/xserver-xorg/pci/vboxvideo.ids"
            ln -s "$share_dir/vboxvideo.ids" /usr/share/xserver-xorg/pci 2>/dev/null
            test -n "$usehal" && setupxorgconf=""
        }

        # Do the XF86Config/xorg.conf hack for those versions that require it
        configured=""
        generated=""
        if test -n "$setupxorgconf"; then
            for i in $x11conf_files; do
                if test -r "$i"; then
                    if grep -q "VirtualBox generated" "$i"; then
                        generated="$generated  `printf "$i\n"`"
                    else
                        "$lib_dir/x11config-new.pl" $newmouse $usehal "$i"
                    fi
                    configured="true"
                fi
                # Timestamp, so that we can see if the config file is changed
                # by someone else later
                test -r "$i.vbox" && touch "$i.vbox"
            done
            # X.Org Server 1.5 and 1.6 can detect hardware they know, but they
            # need a configuration file for VBoxVideo.
            main_cfg="/etc/X11/xorg.conf"
            nobak="/etc/X11/xorg.vbox.nobak"
            if test -z "$configured" -a ! -r "$nobak"; then
                touch "$main_cfg"
                "$lib_dir/x11config-new.pl" --useHal --noBak "$main_cfg"
                touch "$nobak"
            fi
        fi
        # X.Org Server versions starting with 1.5 can do mouse auto-detection,
        # to make our lives easier and spare us the nasty hacks.
        test -n "$usehal" &&
            if [ -d /etc/hal/fdi/policy ]
            then
                # Install hal information about the mouse driver so that X.Org
                # knows to load it.
                install -o 0 -g 0 -m 0644 "$share_dir/90-vboxguest.fdi" /etc/hal/fdi/policy
                # Delete the hal cache so that it notices our fdi file
                rm -r /var/cache/hald/fdi-cache 2> /dev/null
            fi
        succ_msg
        test -n "$generated" &&
            cat << EOF
The following X.Org/XFree86 configuration files were originally generated by
the VirtualBox Guest Additions and were not modified:

$generated

EOF
        cat << EOF
You may need to restart the hal service and the Window System to enable the
Guest Additions.

EOF
    fi

    begin "Installing graphics libraries and desktop services components"
    # Install selinux policy for Fedora 7 and 8 to allow the X server to
    # open device files
    case "$redhat_release" in
        Fedora\ release\ 7* | Fedora\ release\ 8* )
            semodule -i vbox_x11.pp > /dev/null 2>&1
            ;;
    esac

    # Our logging code generates some glue code on 32-bit systems.  At least F10
    # needs a rule to allow this.  Send all output to /dev/null in case this is
    # completely irrelevant on the target system.
    chcon -t unconfined_execmem_exec_t '/usr/bin/VBoxClient' > /dev/null 2>&1
    semanage fcontext -a -t unconfined_execmem_exec_t '/usr/bin/VBoxClient' > /dev/null 2>&1
    # Install the guest OpenGL drivers
    if [ -d /usr/lib64/dri ]
    then
        rm -f /usr/lib64/dri/vboxvideo_dri.so
        ln -s $LIB/VBoxOGL.so /usr/lib64/dri/vboxvideo_dri.so
    elif [ -d /usr/lib/dri ]
    then
        rm -f /usr/lib/dri/vboxvideo_dri.so
        ln -s $LIB/VBoxOGL.so /usr/lib/dri/vboxvideo_dri.so
    fi

    # And set up VBoxClient to start when the X session does
    install_x11_startup_app "$lib_dir/98vboxadd-xclient" "$share_dir/vboxclient.desktop" VBoxClient VBoxClient-all ||
        fail "See the log file $LOG for more information."
    succ_msg
}

# cleanup_script
cleanup()
{
    # Restore xorg.conf files as far as possible
    ## List of generated files which have been changed since we generated them
    newer=""
    ## Are we dealing with a legacy information which didn't support
    # uninstallation?
    legacy=""
    ## Do any of the restored configuration files still reference our drivers?
    failed=""
    test -r "$CONFIG_DIR/$CONFIG" || legacy="true"
    main_cfg="/etc/X11/xorg.conf"
    nobak="/etc/X11/xorg.vbox.nobak"
    if test -r "$nobak"; then
        test -r "$main_cfg" &&
            if test -n "$legacy" -o ! "$nobak" -ot "$main_cfg"; then
                rm -f "$nobak" "$main_cfg"
            else
                newer="$newer`printf "  $main_cfg (no original)\n"`"
            fi
    else
        for i in $x11conf_files; do
            if test -r "$i.vbox"; then
                if test "$i" -ot "$i.vbox" -o -n "$legacy"; then
                    mv -f "$i.vbox" "$i"
                    grep -q -E 'vboxvideo|vboxmouse' "$i" &&
                        failed="$failed`printf "  $i\n"`"
                else
                    newer="$newer`printf "  $i ($i.vbox)\n"`"
                fi
            fi
        done
    fi
    test -n "$newer" && cat << EOF

The following X.Org/XFree86 configuration files were not restored, as they may
have been changed since they were generated by the VirtualBox Guest Additions.
You may wish to restore these manually.  The file name in brackets is the
original version.

$newer

EOF
    test -n "$failed" && cat << EOF

The following X.Org/XFree86 configuration files were restored, but still
contain references to the Guest Additions drivers.  You may wish to check and
possibly correct the restored configuration files to be sure that the server
will continue to work after it is restarted.

$newer

EOF

    # Remove X.Org drivers
    find "$x11_modules_dir" /usr/lib64/xorg/modules /usr/lib/xorg/modules \
        /usr/X11R6/lib64/modules /usr/X11R6/lib/modules \
        /usr/X11R6/lib/X11/modules \
        '(' -name 'vboxvideo_drv*' -o -name 'vboxmouse_drv*' ')' \
        -exec rm -f '{}' ';' 2>/dev/null

    # Remove the link to vboxvideo_dri.so
    rm -f /usr/lib/dri/vboxvideo_dri.so /usr/lib64/dri/vboxvideo_dri.so 2>/dev/null

    # Remove VBoxClient autostart files
    rm /etc/X11/Xsession.d/98vboxadd-xclient 2>/dev/null
    rm /etc/X11/xinit.d/98vboxadd-xclient 2>/dev/null
    rm /etc/X11/xinit/xinitrc.d/98vboxadd-xclient.sh 2>/dev/null
    rm /etc/xdg/autostart/vboxclient.desktop 2>/dev/null
    rm /usr/share/autostart/vboxclient.desktop 2>/dev/null

    # Remove other files
    rm /etc/hal/fdi/policy/90-vboxguest.fdi 2>/dev/null
    rm /usr/share/xserver-xorg/pci/vboxvideo.ids 2>/dev/null
}

dmnstatus()
{
    /bin/true
}

case "$1" in
start)
    start
    ;;
stop)
    stop
    ;;
restart)
    restart
    ;;
setup)
    setup
    ;;
cleanup)
    cleanup
    ;;
status)
    dmnstatus
    ;;
*)
    echo "Usage: $0 {start|stop|restart|status}"
    exit 1
esac

exit
