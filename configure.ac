# Process this file with autoconf to produce a configure script.

# AC_PREREQ(2.59)
AC_INIT([qore-oracle-module], [1.0.9],
        [David Nichols <nicholsman@gmail.com>],
        [qore-oracle-module])
AM_INIT_AUTOMAKE
AC_CONFIG_HEADER([config.h])

AC_LANG(C++)
AC_PROG_CXX
AC_PROG_CC
AC_DISABLE_STATIC
AC_LIBTOOL_DLOPEN
AM_PROG_LIBTOOL
AC_PROG_INSTALL

# fix for building with older autotools
if test ! -d m4; then mkdir m4; fi

AC_CONFIG_MACRO_DIR([m4])

# set system variables
OS=`uname -s`
if test "$OS" = "HP-UX"; then
   if test "$host_cpu" = "ia64"; then
      ARCH=itanium
   else
      ARCH=PA-RISC
   fi
else
   ARCH=`uname -p`
   if test $ARCH = i686 -o $ARCH = i586 -o $ARCH = athlon ; then
      ARCH=i386
   fi
fi

# set find prefix
if test -n "$prefix" -a "$prefix" != "NONE"; then
   find_prefix="$prefix"
fi

#AC_CANONICAL_HOST
# get CPU family
case "${host_cpu}" in
   i[[3456]]86) cpu_family=i386 ;;
   *)           cpu_family=${host_cpu} ;;
esac

# set OS-specific variables
SHLIB_SUFFIX=so
#echo host_os=${host_os}sss
case "${host_os}" in
     *linux*)   # set compile flags for Linux
		CPPFLAGS="${CPPFLAGS} -D_GNU_SOURCE"
		SHLIB_SUFFIX=so
		AC_DEFINE(LINUX, 1, if compiling on Linux)
		;;
     *solaris*) # set compile flags for Solaris
		CPPFLAGS="${CPPFLAGS} -D_POSIX_C_SOURCE=199506L -D_XPG4_2 -D_XPG5 -D__EXTENSIONS__"
		# for checking for some network functions on Solaris
		LDFLAGS="$LDFLAGS -lnsl"
		# assume we are using CC if not using g++, add -norunpath to link flags in this case
		if test "$GXX" != "yes"; then
		   LDFLAGS="$LDFLAGS -norunpath"
		fi
		AC_DEFINE(SOLARIS, 1, if compiling on Solaris)
		# add -ldl to libs if on Solaris 8
		if test "${host_os}" = "solaris2.8"; then
		   OTHER_LIBS="$OTHER_LIBS -ldl"
		fi
	        ;;
     *darwin*)  SHLIB_SUFFIX=dylib
		# libtool 1.5.* is creating modules on Darwin 8.* (OS/X 10.4.*) with a .so suffix for some reason
		MODULE_SUFFIX=so
		AC_DEFINE(DARWIN, 1, [if compiling on Darwin])
		# use gnutar on darwin to avoid getting those pesky resource files
		AMTAR=gnutar
		darwin=true
		;;
     *hpux*)    if test "$host_cpu" = "ia64"; then
     		   SHLIB_SUFFIX=so
		else  # assume hppa (PA-RISC)
     		   SHLIB_SUFFIX=sl
		   AC_DEFINE(SHLIB_SUFFIX, "sl", shared library suffix)
		fi
		# add "-AA" and -D__STDC_EXT__ to aCC commandline to enable a clean compile
		if test -n "`echo $CXX|grep aCC`"; then
		   CXXFLAGS="$CXXFLAGS -AA -D__STDC_EXT__ -D_RWSTD_MULTI_THREAD"
		fi
		AC_DEFINE(HPUX, 1, if compiling on HP-UX)
		;;
esac
AC_SUBST(SHLIB_SUFFIX)
if test -z "$MODULE_SUFFIX" ; then
   MODULE_SUFFIX=$SHLIB_SUFFIX
fi
AC_SUBST(MODULE_SUFFIX)

AC_ARG_ENABLE([64bit],
  [AS_HELP_STRING([--enable-64bit],
		  [enable 64bit support (default: auto)])],
  [case "${enable_64bit}" in
       yes|no) ;;
       *)      AC_MSG_ERROR(bad value ${enable_64bit} for --enable-64bit) ;;
      esac],
  [enable_64bit=auto])

if test "$enable_64bit" = "auto"; then
   # turn on 64-bit by default if compiling on itanium or x86_64
   if test "$host_cpu" = "ia64" -o "$host_cpu" = "x86_64" -o "$host_cpu" = "amd64"; then
      enable_64bit=yes
   else
      case "${host_os}" in
          darwin*)   mv=`uname -r|cut -f1 -d.`
     	   	     if test $mv -ge 10 -a "${host_cpu}" = "i386"; then
		         enable_64bit=yes
		     fi
		     ;;
      esac
      if test "$enable_64bit" = "auto"; then
	  enable_64bit=no
      fi
   fi
fi

# see if we know how to set flags properly for different hosts and architectures
# FIXME: add test on HP-UX if the compiler can generate the appropriate binaries
if test "$enable_64bit" = "yes"; then
   case "${host_os}" in
      *linux*)	if test "$GXX" = "yes"; then
		   CXXFLAGS="$CXXFLAGS -m64"
		fi
		LIBSUFFIX=64
		AC_SUBST(LIBSUFFIX)
		;;
      *hpux*)	if test "$host_cpu" = "ia64"; then
                    if test -n "`echo $CXX|grep aCC`"; then
                        CXXFLAGS="$CXXFLAGS +DD64"
                    elif test "$GXX" = "yes"; then
                        CXXFLAGS="$CXXFLAGS -mlp64"
                    fi
                else
		    if test -n "`echo $CXX|grep aCC`"; then
		        CXXFLAGS="$CXXFLAGS +DA2.0W"
		    elif test "$GXX" = "yes"; then
			CXXFLAGS="$CXXFLAGS -march=2.0"
		    fi
		fi
		;;
     *solaris*) if test "$GXX" = "yes"; then
		    if test "$host_cpu" = "sparc"; then
		        CXXFLAGS="$CXXFLAGS -mcpu=ultrasparc3 -m64"  # NOTE we compile for minimum ultrasparc 3 in 64-bit mode
		    else
		        CXXFLAGS="$CXXFLAGS -m64"
		    fi
		else
		    CXXFLAGS="$CXXFLAGS -xarch=generic64"
		fi
		;;
     *darwin*)	if test "$GXX" = "yes"; then
		   CXXFLAGS="$CXXFLAGS -m64"
		fi
		# set ARCH to x86_64 to be consistent with other intel 64-bit builds
		if test "$host_cpu" = "i386"; then
		   ARCH=x86_64
		fi
		;;
   esac
   SYBLIBSUFFIX=64
   RVLIBSUFFIX=64
   bits=64
else
   case "${host_os}" in
     *linux*)	if test "$GXX" = "yes"; then
		   CXXFLAGS="$CXXFLAGS -m32"
		fi
		;;
     *darwin*)  if test "$GXX" = "yes"; then
                   CXXFLAGS="$CXXFLAGS -m32"
                fi
                ;;
     *hpux*)	if test "$host_cpu" != "ia64"; then
		    if test "$GXX" = "yes"; then
		        CXXFLAGS="$CXXFLAGS -march=2.0"
		    elif test -n "`echo $CXX|grep aCC`" -a "$host_cpu" != "ia64"; then
		        CXXFLAGS="$CXXFLAGS +DA2.0N"   # NOTE we compile for PA-RISC 2.0 32-bit, not PA-RISC 1.1
		    fi
		fi
		;;
     *solaris*) if test "$GXX" = "yes"; then
		    if test "$host_cpu" = "sparc"; then
		        CXXFLAGS="$CXXFLAGS -mcpu=v8 -m32"
		    else
		        CXXFLAGS="$CXXFLAGS -m32"
		    fi
		else
		    CXXFLAGS="$CXXFLAGS -xarch=generic"
		fi
		;;
   esac
   if test "$host_cpu" = "x86_64"; then
      cpu_family=i386
   fi
   bits=32
fi

# checks for how to build threads
ACX_PTHREAD
if test "$acx_pthread_ok" = "no"; then
   AC_MSG_ERROR(POSIX threads do not seem to be supported on this platform, aborting)
fi
CXXFLAGS="$CXXFLAGS $PTHREAD_CFLAGS"
#LDFLAGS="$LDFLAGS $PTHREAD_LIBS"

AC_ARG_ENABLE([single-compilation-unit],
  [AS_HELP_STRING([--enable-single-compilation-unit],
                  [enable compilation as a single unit (default: on)])],
  [case "${enable_single_compilation_unit}" in
       yes|no) ;;
       *)      AC_MSG_ERROR(bad value ${enable_single_compilation_unit} for --enable-single-compilation-unit) ;;
      esac],
  [enable_single_compilation_unit=yes])

AC_ARG_WITH([oracle],
  [AS_HELP_STRING([--with-oracle@<:@=DIR@:>@],
                  [directory for Oracle support (defaults to $ORACLE_HOME)])],
   , 
   [with_oracle=auto])

if test -z "$ORACLE_INSTANT_CLIENT" -a -n "INSTANT_CLIENT"; then
   ORACLE_INSTANT_CLIENT="$INSTANT_CLIENT"
fi

# set instant client variable for macports
if test -z "$ORACLE_INSTANT_CLIENT" -a -n "$find_prefix" -a -f "$find_prefix/lib/oracle/libclntsh.$SHLIB_SUFFIX"; then
   ORACLE_INSTANT_CLIENT="$find_prefix/lib/oracle"
fi

# check for macports instant client in default macports directory
if test -z "$ORACLE_INSTANT_CLIENT" -a -f "/opt/local/lib/oracle/libclntsh.$SHLIB_SUFFIX"; then
   ORACLE_INSTANT_CLIENT="/opt/local/lib/oracle"
fi

if test "$with_oracle" = "auto"; then
   if test -z "$ORACLE_HOME" -a -z "$ORACLE_INSTANT_CLIENT" -a -d "/usr/lib/oracle"; then
      dir=`ls -rt /usr/lib/oracle|tail -1`
      if test "$enable_64bit" = yes; then
      	 dir="/usr/lib/oracle/$dir/client64"
      else
	 dir="/usr/lib/oracle/$dir/client"
      fi
      if test -d "$dir"; then\
      	 ORACLE_INSTANT_CLIENT="$dir"
      fi
   fi
   if test -n "$ORACLE_HOME" -o -n "$ORACLE_INSTANT_CLIENT" -o "$with_oracle_instant_client"; then
      with_oracle=yes
   else
      with_oracle=no
   fi
fi

get_oracle_version() {
  AC_MSG_CHECKING([Oracle version])

  # find latest oracle client library
  if test -n "$2" -a -s "$1/orainst/unix.rgs"; then
    ORACLE_VERSION=`grep '"ocommon"' $1/orainst/unix.rgs | sed "s/[ ][ ]*/:/g" | cut -d: -f 6 | cut -c 2-4`
    test -z "$ORACLE_VERSION" && ORACLE_VERSION=7.3
  else
    lf=`ls $1/libclntsh.${SHLIB_SUFFIX}.@<:@1-9@:>@* 2>/dev/null|tail -1`
    if test -z "$lf"; then
       AC_MSG_ERROR([Oracle libraries not found in $1, check oracle installation])
    fi
  
    lf=`basename "$lf" | sed s/.*${SHLIB_SUFFIX}\.//`
    case "$lf" in  
       12*) ORACLE_VERSION=12 ;;
       11*) ORACLE_VERSION=11 ;;
       10*) ORACLE_VERSION=10 ;;
       9*) ORACLE_VERSION=9 ;;
       8*) ORACLE_VERSION=8 ;;
    esac

    if test -z "$ORACLE_VERSION" -a -f $1/libclntsh.a; then
        ORACLE_VERSION=8
    	#if test -f $1/libcore4.a; then
    	#  ORACLE_VERSION=8.0
    	#else
    	#  ORACLE_VERSION=8.1
    	#fi
    fi
  fi

  if test -z "$ORACLE_VERSION"; then
    AC_MSG_ERROR([unrecognized Oracle version in $1, check Oracle installation])
  fi
  if test $ORACLE_VERSION -lt 10; then
     AC_DEFINE(NEED_ORACLE_LOB_WORKAROUND, 1, [define if we need a workaround for the Oracle streaming LOB callback bug])
  fi
  AC_MSG_RESULT($ORACLE_VERSION)
}

case "$with_oracle" in
  yes) ORACLE_DIR=$ORACLE_HOME ;;
  no) ;;
  *) ORACLE_DIR=$with_oracle; with_oracle=yes ;;
esac

if test "$with_oracle" != "yes"; then
   AC_MSG_ERROR([oracle not found; cannot build module without oracle support])
fi

AC_ARG_WITH([oracle-instant-client],
	    [AS_HELP_STRING([--with-oracle-instant-client@<:@=DIR@:>@],
            [directory for Oracle instant client files])],
            ,
            [with_oracle_instant_client=$ORACLE_INSTANT_CLIENT])

if test -n "$with_oracle_instant_client"; then
      AC_MSG_CHECKING([for Oracle instant client SDK])      
      if test -f "$with_oracle_instant_client/sdk/include/oci.h"; then
	 AC_MSG_RESULT([ok])
	 ORACLE_INCLUDES="-I$with_oracle_instant_client/sdk/include"
	 oracle_lib_dir=$with_oracle_instant_client
	 get_oracle_version "$with_oracle_instant_client"
      elif test -d "/usr/include/oracle"; then
	 # see if oracle includes are in /usr/include/oracle/<ver>/client*/
      	 dir=`ls -rt /usr/include/oracle|tail -1`
	 if test "$enable_64bit" = yes; then
	    dir="/usr/include/oracle/$dir/client64"
	 else
	    dir="/usr/include/oracle/$dir/client"
         fi
	 if test ! -f "$dir/oci.h"; then
	    AC_MSG_ERROR([instant client SDK include files not found, check instant client and instant client SDK (header files) installation])
	 else
	     # try to find oracle libraries in instant client dir
	     if test ! -d "$with_oracle_instant_client/lib"; then
	     	AC_MSG_ERROR([instant client libraries not found in $with_oracle_instant_client/lib, check instant client installation])
	     fi
	     AC_MSG_RESULT([ok])
	     ORACLE_INCLUDES="-I$dir"
	     oracle_lib_dir=$with_oracle_instant_client/lib
	     get_oracle_version $with_oracle_instant_client/lib
	 fi
      fi
else
      if test -d "$ORACLE_DIR/rdbms/public"; then
         ORACLE_INCLUDES="-I$ORACLE_DIR/rdbms/public"
      fi
      if test -d "$ORACLE_DIR/rdbms/demo"; then
	 ORACLE_INCLUDES="$ORACLE_INCLUDES -I$ORACLE_DIR/rdbms/demo"
      fi
      if test -d "$ORACLE_DIR/network/public"; then
	 ORACLE_INCLUDES="$ORACLE_INCLUDES -I$ORACLE_DIR/network/public"
      fi
      if test -d "$ORACLE_DIR/plsql/public"; then
	 ORACLE_INCLUDES="$ORACLE_INCLUDES -I$ORACLE_DIR/plsql/public"
      fi
      if test "$enable_64bit" != "yes" -a -d "$ORACLE_DIR/lib32"; then
         oracle_lib_dir="$ORACLE_DIR/lib32"
      else
         oracle_lib_dir="$ORACLE_DIR/lib"
      fi
      get_oracle_version $oracle_lib_dir $ORACLE_DIR
fi

AC_SUBST(ORACLE_INCLUDES)

case "$ORACLE_VERSION" in
   9*|10*|11*|12*) ORACLE_LIBS="-L$oracle_lib_dir -lclntsh" ;; 
   *)      AC_MSG_ERROR(Unsupported Oracle version) ;;
esac
AC_SUBST(ORACLE_LIBS)

# check for 'oci.h'
SAVE_CXXFLAGS="$CXXFLAGS"
SAVE_LDFLAGS="$LDFLAGS"
CXXFLAGS="$CXXFLAGS $ORACLE_INCLUDES"
LDFLAGS="$LDFLAGS $ORACLE_LIBS"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <oci.h>
         ]],[[
        ]])], with_oracle=yes, AC_MSG_ERROR(file 'oci.h' not found in '$ORACLE_INCLUDES'))
AC_CHECK_FUNC([OCIClientVersion], [have_ociclientversion=yes], [have_ociclientversion=no])
if test "$have_ociclientversion" = "yes"; then
   AC_DEFINE(HAVE_OCICLIENTVERSION, 1, [define if OCIClientVersion is available])      
fi
CXXFLAGS="$SAVE_CXXFLAGS"
LDFLAGS="$SAVE_LDFLAGS"

set_qore_cppflags() {
    QORE_INC_DIR=$1
    if test "$1" != "/usr/include"; then
        QORE_CPPFLAGS=-I$1
    fi
}

find_qore() {
    lib=$1/lib${LIBSUFFIX}

    # if we only want the static qore libraries
    if test -n "$qore_static"; then
        if test -f "$lib/libqore.a"; then
            QORE_LDFLAGS="$lib/libqore.a"
            AC_MSG_RESULT([$lib (forced static)])
        fi
    else
        if test -f "$lib/libqore.${SHLIB_SUFFIX}"; then
            if test "$lib" != "/lib" -a "$lib" != "/usr/lib"; then
                QORE_LDFLAGS="-L$lib"
            fi
            QORE_LDFLAGS="$QORE_LDFLAGS -lqore"
            AC_MSG_RESULT([$lib (shared)])
        elif test -f "$lib/libqore.a"; then
            if test "$lib" != "/lib" -a "$lib" != "/usr/lib"; then
                QORE_LDFLAGS="-L$lib"
            fi
            QORE_LDFLAGS="$QORE_LDFLAGS -lqore"
            AC_MSG_RESULT([$lib (static)])
        fi
    fi
    if test -z "$QORE_LDFLAGS"; then
        return
    fi

    AC_MSG_CHECKING([for qore include files])
    # try to find include files
    if test "$1" = "/"; then
        inc=/usr/include
    else
        inc=$1/include
    fi
    if test -f "$inc/qore/Qore.h"; then
        AC_MSG_RESULT([found: $inc])
        set_qore_cppflags $inc
    else
        AC_MSG_ERROR([not found])
    fi

    AC_SUBST(QORE_LDFLAGS)
    AC_SUBST(QORE_CPPFLAGS)
}

# see if we can figure out where the qore library is
AC_ARG_WITH([qore-dir],
  [AS_HELP_STRING([--with-qore-dir@<:@=DIR@:>@],
                  [Qore directory])],
  [if test ! -d "${with_qore_dir}"; then AC_MSG_ERROR(bad value ${with_qore_dir} for --with-qore-dir); unset with_qore_dir; fi],
  [with_qore_dir="$QORE_DIR"])

AC_MSG_CHECKING([for qore libraries and header files])
for dir in "${with_qore_dir}" "${find_prefix}" /usr / /usr/local /opt/gnu /opt/qore /usr/local/qore /opt/local /sw /usr/sfw /opt/sfw; do
    find_qore $dir
    if test -n "$QORE_LDFLAGS"; then
        break
    fi
done
if test -z "$QORE_LDFLAGS"; then
    AC_MSG_ERROR([no qore library found])
else
   # test qore version
   AC_MSG_CHECKING([qore library version])

   # get qore version code
   qvc=`grep QORE_VERSION_CODE $QORE_INC_DIR/qore/qore-version.h|cut -f3 -d\  2>/dev/null`

   # get qore version string
   qore_version=`grep QORE_VERSION.\" $QORE_INC_DIR/qore/qore-version.h|cut -f3 -d\ |sed s/\"//g 2>/dev/null`

   # if version code not present, may be qore 0.7.0 or earlier
   if test -z "$qvc"; then
      qv_major=`grep QORE_VERSION_MAJOR $QORE_INC_DIR/qore/qore-version.h|cut -f3 -d\  2>/dev/null`
      if test -z "$qv_major"; then
      	 AC_MSG_ERROR([cannot determine; check Qore header files in $QORE_INC_DIR])
      fi

      qv_minor=`grep QORE_VERSION_MINOR $QORE_INC_DIR/qore/qore-version.h|cut -f3 -d\  2>/dev/null`
      qv_sub=`grep QORE_VERSION_SUB $QORE_INC_DIR/qore/qore-version.h|cut -f3 -d\  2>/dev/null`

      qvc=`printf "%02d%02d%02d" $qv_major $qv_minor $qv_sub`
      AC_DEFINE_UNQUOTED([QORE_VERSION_CODE], $qvc, [define when missing in Qore headers])
   fi

   if test $qvc -lt 000701; then
      AC_MSG_ERROR([found $qore_version; this module requires at least version 0.7.1])
   fi

   AC_MSG_RESULT([${qore_version} OK])

   MODULE_API=`qore --latest-module-api`
   mapi_major=`qore --latest-module-api|cut -f1 -d.`
   mapi_minor=`qore --latest-module-api|cut -f2 -d.`

   if test "$mapi_major" -ge 0 -o "$mapi_minor" -ge 7; then
      AC_DEFINE(QORE_HAS_DATASOURCE_PORT, 1, define if qore supports ports in the Datasource object)
   fi

   AC_SUBST(MODULE_API)

   if test -z "${prefix}" -o "${prefix}" = "NONE"; then
      libdir=`qore --module-dir`
   else
      libdir="$prefix/lib${LIBSUFFIX}/qore-modules"
      qlibdir=`qore --module-dir`
      if test "$libdir" != "$qlibdir"; then
         AC_MSG_WARN([warning, installed qore's module dir is $qlibdir, this module will be installed in $libdir])
      fi
   fi
fi

AC_ARG_ENABLE([profile],
     [AS_HELP_STRING([--enable-profile],
		     [turn on profiling support (default=no)])],
     [case "${enable_profile}" in
       yes|no) ;;
       *)      AC_MSG_ERROR(bad value ${enable_profile} for --enable-profile) ;;
      esac],
     [enable_profile=no])

AC_ARG_ENABLE([debug],
     [AS_HELP_STRING([--enable-debug],
		     [turn on debugging (default=no)])],
     [case "${enable_debug}" in
       yes|no) ;;
       *)      AC_MSG_ERROR(bad value ${enable_debug} for --enable-debug) ;;
      esac],
     [enable_debug=yes])

if test "${enable_debug}" = yes; then
   AC_DEFINE(DEBUG, 1, Define if debugging support should be included)
   # remove -O2 from CXXFLAGS so valgrind can report properly
   CXXFLAGS=`echo $CXXFLAGS | sed 's/\-O2//'`
else
   AC_DEFINE(NDEBUG, 1, Define if assert() declarations should be suppressed)
fi

if test "${enable_profile}" = yes; then
   AC_DEFINE(PROFILE, 1, Define if profiling support should be included)
fi

AC_ARG_ENABLE([optimization],
     [AS_HELP_STRING([--enable-optimization],
		     [turn on optimization (default=auto (yes unless debugging is enabled)])],
     [case "${enable_optimization}" in
       yes|no) ;;
       *)      AC_MSG_ERROR(bad value ${enable_optimization} for --enable-optimization) ;;
      esac],
     [enable_optimization=auto])

if test "$enable_optimization" = "auto"; then
   if test "${enable_debug}" = yes; then
      enable_optimization=no
   else
      enable_optimization=yes
   fi
fi

# check for gcc visibility support
AC_MSG_CHECKING([for gcc visibility support])
if test "$GXX" = "yes"; then
   gcc_version=`$CXX -dumpversion`
   if test "$gcc_version" \> "4"; then
      AC_DEFINE(HAVE_GCC_VISIBILITY, 1, if compiling with g++ and visibility support is available)
      AC_MSG_RESULT([yes, $CXX $gcc_version])
      # we no longer set visibility to hidden by default because it makes RTTI symbols invisible
      # all functions in the library are tagged anyway
      #CXXFLAGS="$CXXFLAGS -fvisibility=hidden"
   else
      AC_MSG_RESULT([no, gcc $gcc_version])
   fi
else
   AC_MSG_RESULT([no, $CXX])
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_STRUCT_ST_BLOCKS
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_HEADER_TIME
AC_STRUCT_TM
AC_C_VOLATILE

# turn on optimizations if we know how
if test "$enable_optimization" = "yes"; then
    if test "$GXX" = "yes"; then
        CXXFLAGS="$CXXFLAGS -O3"  # gcc
    elif test "$CXX" = "CC" -a "`echo $host_os|cut -b-7`" = "solaris"; then
        # remove -g with Solaris CC as it reduces optimization levels
	CXXFLAGS="`echo $CXXFLAGS| sed s/-g//` -xO5" # Solaris CC
    elif test -n "`echo $CXX|grep aCC`"; then 
        # remove -g with HP-UX aCC as it reduces optimization levels
	CXXFLAGS="`echo $CXXFLAGS| sed s/-g//` +O4" # HP-UX aCC
    fi
else
    if test "$GXX" = "yes"; then
        CXXFLAGS="`echo $CXXFLAGS| sed s/-O2//`"  # gcc
    fi
fi

# turn on some build flags for non-debugging, non-profiling builds
if test "$enable_debug" = "no" -a "$enable_profile" = "no"; then
    # strip binaries and libraries on darwin with -x
    if test "`echo $host_os|cut -b-6`" = "darwin"; then
       LDFLAGS="$LDFLAGS -Wl,-x"
    fi	
fi

# turn on warnings if we know how
if test "$GXX" = "yes"; then
   CXXFLAGS="$CXXFLAGS -Wall"  # gcc
fi

AC_SUBST(OTHER_LIBS)

# save more version information in config.h
MODULE_VERSION_MAJOR=`echo $PACKAGE_VERSION | cut -f3 -d\  | sed s/\"//g | cut -f1 -d.`
MODULE_VERSION_MINOR=`echo $PACKAGE_VERSION | cut -f3 -d\  | sed s/\"//g | cut -f2 -d.`
MODULE_VERSION_SUB=`echo $PACKAGE_VERSION | cut -f3 -d\  | sed s/\"//g | cut -f3 -d.`

# set version information
AC_DEFINE_UNQUOTED([MODULE_VERSION_MAJOR], $MODULE_VERSION_MAJOR, [major version number])
AC_DEFINE_UNQUOTED([MODULE_VERSION_MINOR], $MODULE_VERSION_MINOR, [minor version number])
AC_DEFINE_UNQUOTED([MODULE_VERSION_SUB], $MODULE_VERSION_SUB, [sub version number])
AC_DEFINE_UNQUOTED([MODULE_TARGET_ARCH], "$ARCH", [host type])
AC_DEFINE_UNQUOTED([MODULE_TARGET_OS], "$OS", [host type])
AC_DEFINE_UNQUOTED([MODULE_TARGET_BITS], $bits, [32 or 64 bit build])

AM_CONDITIONAL([COND_DEBUG],      [test "$enable_debug" = yes])
AM_CONDITIONAL([COND_PROFILE],    [test "$enable_profile" = yes])
AM_CONDITIONAL([COND_SINGLE_COMPILATION_UNIT], [test "$enable_single_compilation_unit" = yes])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT

echo "*** MODULE FEATURES ***"
printf "%-24s: %s %s" module "$OS" "$cpu_family"
if test "$enable_64bit" = "yes"; then
   printf " (64-bit)\n"
else
   printf " (32-bit)\n"
fi

show_library_feature()
{
   printf "%-24s: " "$1"
   if test "$2" = "yes"; then
      printf "%-8s" yes
   else
      printf disabled
   fi
   if test -n "$3"; then
      printf " (%s)" $3
   fi
   echo
}

show_library_feature optimizations $enable_optimization
show_library_feature debug $enable_debug
show_library_feature profiling $enable_profile
