#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
m4_include(m4/acx_pthread.m4)

# Autoconf
AC_PREREQ(2.59)

# NOTE: This version is _only_ used for naming the tarball.  The
# VERSION define is not used by the code.  It gets a version string
# from 'git describe'; see src/ceph_ver.[ch]
AC_INIT([ceph], [0.52], [ceph-devel@vger.kernel.org])

AC_CONFIG_SUBDIRS([src/gtest])
AC_CONFIG_SUBDIRS([src/leveldb])

# Environment
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

# Automake
AM_INIT_AUTOMAKE
AM_PROG_CC_C_O
AM_PROG_LIBTOOL

# enable make V=0 (if automake >1.11)
AM_INIT_AUTOMAKE([foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Platform
case "${target_os}" in
darwin*)
	AC_DEFINE([DARWIN], [1], [Define if darwin/osx])
	;;
linux*)
	linux="yes"
	;;
freebsd*)
	freebsd="yes"
	;;
esac
AM_CONDITIONAL(LINUX, test x"$linux" = x"yes")
AM_CONDITIONAL(FREEBSD, test x"$freebsd" = x"yes")

# Checks for programs.
AC_PROG_CXX
#AC_PROG_CC
#AC_PROG_RANLIB
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL

# Checks for compiler warning types

# AC_CHECK_CC_FLAG(FLAG_TO_TEST, VARIABLE_TO_SET_IF_SUPPORTED)
# ---------
AC_DEFUN([AC_CHECK_CC_FLAG],
[{
	AC_LANG_PUSH([C])
	my_cflags_save="$CFLAGS"
	CFLAGS="$my_cflags_save $1"
	AC_MSG_CHECKING([whether $CC accepts $1])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]], [[ ]])],
			  [AC_MSG_RESULT([yes]); AC_SUBST([$2], ["$1"])],
			  [AC_MSG_RESULT([no])]
			 )
	CFLAGS="$my_cflags_save"
	AC_LANG_POP([C])
}])

AC_CHECK_CC_FLAG([-Wtype-limits], [WARN_TYPE_LIMITS])
AC_CHECK_CC_FLAG([-Wignored-qualifiers], [WARN_IGNORED_QUALIFIERS])

# Checks for libraries.
ACX_PTHREAD
AC_CHECK_LIB([uuid], [uuid_parse], [true], AC_MSG_FAILURE([libuuid not found]))
AC_CHECK_LIB([m], [pow], [true], AC_MSG_FAILURE([libm not found]))
if test x"$linux" = x"yes"; then
	AC_CHECK_LIB([keyutils], [add_key], [true], AC_MSG_FAILURE([libkeyutils not found]))
fi
AC_CHECK_FUNCS([syncfs], AC_DEFINE([HAVE_SYS_SYNCFS], [1], [we have syncfs]), [])

# Find some crypto library for us to use, while letting user to decide which one to use.
AC_ARG_WITH([cryptopp],
	[AS_HELP_STRING([--with-cryptopp], [Use cryptographic functions from cryptopp])],
	[],
	[with_cryptopp=check])
have_cryptopp=no
# this looks clumsy but it's just if A then { success } else { if B then success }
AS_IF([test "x$with_cryptopp" != "xno"],
    [PKG_CHECK_MODULES([CRYPTOPP],
	    [libcrypto++],
	    [have_cryptopp=yes],
	    [
		   AC_LANG_PUSH([C++])
		   SAVED_CXXFLAGS="${CXXFLAGS}"
		   SAVED_LIBS="${LIBS}"
		   LIBS="${LIBS} ${PTHREAD_LIBS}"
		   CXXFLAGS="${CXXFLAGS} ${PTHREAD_CFLAGS}"
		   AC_SEARCH_LIBS([_ZTIN8CryptoPP14CBC_EncryptionE], [crypto++ cryptopp],
			  [have_cryptopp=yes],
			  [true], [])
		   CRYPTOPP_LIBS="${ac_cv_search__ZTIN8CryptoPP14CBC_EncryptionE}"
		   LIBS="${SAVED_LIBS}"
		   CXXFLAGS="${SAVED_CXXFLAGS}"
		   AC_LANG_POP([C++])
	   ])])
# bail out if given explicit --with-cryptopp
if test "x$have_cryptopp" = "xno" -a "x$with_cryptopp" != "xcheck" -a "x$with_cryptopp" != "xno"; then
    AC_MSG_FAILURE([--with-cryptopp was given, but library was not found])
fi

AC_ARG_WITH([nss],
	[AS_HELP_STRING([--with-nss], [Use cryptographic functions from nss])],
	[],
	[with_nss=check])
have_nss=no
AS_IF([test "x$with_nss" != "xno"],
    [PKG_CHECK_MODULES([NSS], [nss], [have_nss=yes], [true])])
# bail out if given explicit --with-nss
if test "x$have_nss" = "xno" -a "x$with_nss" != "xcheck" -a "x$with_nss" != "xno"; then
    AC_MSG_FAILURE([--with-nss was given, but library was not found])
fi

# now decide which crypto library to really use
if test "x$have_cryptopp" = "xyes"; then
    AC_MSG_NOTICE([using cryptopp for cryptography])
    AC_DEFINE([USE_CRYPTOPP], [1], [Define if using CryptoPP.])
    AC_SUBST([CRYPTO_CFLAGS], [$CRYPTOPP_CFLAGS])
    AC_SUBST([CRYPTO_CXXFLAGS], [$CRYPTOPP_CXXFLAGS])
    AC_SUBST([CRYPTO_LIBS], [$CRYPTOPP_LIBS])
elif test "x$have_nss" = "xyes"; then
    AC_MSG_NOTICE([using nss for cryptography])
    AC_DEFINE([USE_NSS], [1], [Define if using NSS.])
    AC_SUBST([CRYPTO_CFLAGS], [$NSS_CFLAGS])
    # this needs CFLAGS too in practise to get the includes right. ugly.
    AC_SUBST([CRYPTO_CXXFLAGS], [$NSS_CFLAGS $NSS_CXXFLAGS])
    AC_SUBST([CRYPTO_LIBS], [$NSS_LIBS])
else
    AC_MSG_FAILURE([no suitable crypto library found])
fi

# profiler?
AC_ARG_WITH([profiler],
            [AS_HELP_STRING([--with-profiler], [build extra profiler binaries])],
	    [case "${withval}" in
	    	  yes) with_profiler=yes ;;
		  no)  with_profiler=no ;;
		  *) AC_MSG_ERROR([bad value ${withval} for --with-profiler]) ;;
	     esac],
            [with_profiler=no])
AS_IF([test "x$with_profiler" = xyes],
	    [AC_CHECK_LIB([profiler], [ProfilerFlush], [],
	        	  [AC_MSG_FAILURE([--with-profiler was given but libprofiler (libgoogle-perftools-dev on debian) not found])])],
	    [])
AM_CONDITIONAL(WITH_PROFILER, test "$with_profiler" = "yes")
AS_IF([test "$with_profiler" = "yes"],
      [AC_DEFINE([HAVE_PROFILER], [1], [Define if you have perftools profiler enabled])],
      [])

# debug crap?
AC_ARG_WITH([debug],
            [AS_HELP_STRING([--with-debug], [build extra debug binaries])],
            [case "${withval}" in
		  yes) with_debug=yes ;;
		  no)  with_debug=no ;;
		  *) AC_MSG_ERROR([bad value ${withval} for --with-debug]) ;;
	     esac],
            [with_debug=no])
AM_CONDITIONAL(WITH_DEBUG, test "$with_debug" = "yes")

AC_DEFINE([DEBUG_GATHER], [1], [Define if you want C_Gather debugging])

# code coverage?
AC_ARG_ENABLE([coverage],
            [AS_HELP_STRING([--enable-coverage], [enable code coverage tracking])],
            [],
            [enable_coverage=no])
AM_CONDITIONAL(ENABLE_COVERAGE, test "x$enable_coverage" != xno)
if test "x$enable_coverage" != xno; then
   AC_DEFINE([ENABLE_COVERAGE], [1], [Define if enabling coverage.])
fi
AC_SUBST(GCOV_PREFIX_STRIP, `echo $(pwd)/src | tr -dc / | wc -c`)

# radosgw?
AC_ARG_WITH([radosgw],
            [AS_HELP_STRING([--with-radosgw], [build RADOS gateway])],
            [],
            [with_radosgw=check])
RADOSGW=0
AS_IF([test "x$with_radosgw" != xno],
	    [AC_CHECK_LIB([fcgi], [FCGX_Init],
             [AC_CHECK_LIB([expat], [XML_Parse],
              [AC_CHECK_LIB([curl], [curl_easy_init],
               [RADOSGW=1
	        AC_CHECK_HEADER([fastcgi/fcgiapp.h],
		 [AC_DEFINE([FASTCGI_INCLUDE_DIR], [1], [FastCGI headers are in /usr/include/fastcgi])])
	       ],
	       [if test "x$with_radosgw" != "xcheck"; then
		    AC_MSG_FAILURE([--with-radosgw was given but libcurl (libcurl-dev on debian) not found])
	       fi])
              ],
	      [if test "x$with_radosgw" != "xcheck"; then
		   AC_MSG_FAILURE([--with-radosgw was given but libexpat (libexpat1-dev on debian) not found])
	      fi])
             ],
	     [if test "x$with_radosgw" != "xcheck"; then
		AC_MSG_FAILURE([--with-radosgw was given but libfcgi (libfcgi-dev on debian) not found])
	     fi])])
AM_CONDITIONAL(WITH_RADOSGW, test "$RADOSGW" = "1")

AS_IF([test "$RADOSGW" = "1"], [AC_DEFINE([WITH_RADOSGW], [1], [define if radosgw enabled])])

# fuse?
AC_ARG_WITH([fuse],
            [AS_HELP_STRING([--without-fuse], [disable FUSE userspace client])],
            [],
            [with_fuse=yes])
LIBFUSE=
AS_IF([test "x$with_fuse" != xno],
	    [AC_CHECK_LIB([fuse], [fuse_main],
             [AC_SUBST([LIBFUSE], ["-lfuse"])
               AC_DEFINE([HAVE_LIBFUSE], [1],
                         [Define if you have fuse])
               HAVE_LIBFUSE=1
              ],
             [AC_MSG_FAILURE(
                   [no FUSE found (use --without-fuse to disable)])])])
AM_CONDITIONAL(WITH_FUSE, [test "$HAVE_LIBFUSE" = "1"])

# tcmalloc?
AC_ARG_WITH([tcmalloc],
	    [AS_HELP_STRING([--without-tcmalloc], [disable tcmalloc for memory allocations])],
	    [],
	    [with_tcmalloc=yes])
TCMALLOC=
AS_IF([test "x$with_tcmalloc" != xno],
	    [AC_CHECK_LIB([tcmalloc], [malloc],
	     [AC_SUBST([LIBTCMALLOC], ["-ltcmalloc"])
	       AC_DEFINE([HAVE_LIBTCMALLOC], [1],
	       		 [Define if you have tcmalloc])
	       HAVE_LIBTCMALLOC=1
	     ],
	    [AC_MSG_FAILURE(
		  [no tcmalloc found (use --without-tcmalloc to disable)])])])
AM_CONDITIONAL(WITH_TCMALLOC, [test "$HAVE_LIBTCMALLOC" = "1"])

# jni?
AC_ARG_WITH([hadoop],
            [AS_HELP_STRING([--with-hadoop], [build hadoop client])],
            [],
            [with_hadoop=check])
AS_IF([test "x$with_hadoop" != xno],
	    [AC_CHECK_HEADER([jni.h],
             [HAVE_JNI=1],
             [if test "x$with_hadoop" != xcheck; then
                 AC_MSG_FAILURE(
                   [--with-hadoop was given but jni.h not found])
               fi
             ])])
AM_CONDITIONAL(WITH_HADOOPCLIENT, [test "$HAVE_JNI" = "1"])

#
# FreeBSD has it in base.
#
if test x"$freebsd" != x"yes"; then
PKG_CHECK_MODULES([LIBEDIT], [libedit >= 2.11],
                [], AC_MSG_FAILURE([No usable version of libedit found.]))
else
	LIBEDIT_LIBS="-ledit"
fi

#libatomic-ops? You want it!
AC_ARG_WITH([libatomic-ops],
            [AS_HELP_STRING([--without-libatomic-ops],
	    [disable libatomic-ops for the atomic_t type])],
            [],
            [with_libatomic_ops=yes])
AS_IF([test "x$with_libatomic_ops" != xno],
	    [AC_CHECK_HEADER([atomic_ops.h],
             [HAVE_ATOMIC_OPS=1],
             [AC_MSG_FAILURE(
                   [no libatomic-ops found (use --without-libatomic-ops to disable)])
              ])])
AS_IF([test "$HAVE_ATOMIC_OPS" = "1"],
	[],
	AC_DEFINE([NO_ATOMIC_OPS], [1], [Defined if you don't have atomic_ops]))
AM_CONDITIONAL(WITH_LIBATOMIC, [test "$HAVE_ATOMIC_OPS" = "1"])

# newsyn?  requires mpi.
#AC_ARG_WITH([newsyn],
#            [AS_HELP_STRING([--with-newsyn], [build newsyn target requires mpi])],
#            [],
#            [with_newsyn=no])

AC_ARG_WITH([ocf],
            [AS_HELP_STRING([--with-ocf], [build OCF-compliant cluster resource agent])],
            ,
            [with_ocf=no])
AM_CONDITIONAL(WITH_OCF, [ test "$with_ocf" = "yes" ])

# use system leveldb?
AC_ARG_WITH([system-leveldb],
	[AS_HELP_STRING([--with-system-leveldb], [use system leveldb])],
	,
	[with_system_leveldb=no])
AS_IF([test "x$with_system_leveldb" = xyes],
	    [AC_CHECK_LIB([leveldb], [leveldb_open], [], [AC_MSG_FAILURE([libleveldb not found])], [-lsnappy -lpthread])])
AS_IF([test "x$with_system_leveldb" = xcheck],
	    [AC_CHECK_LIB([leveldb], [leveldb_open], [with_system_leveldb=yes], [], [-lsnappy -lpthread])])
AM_CONDITIONAL(WITH_SYSTEM_LEVELDB, [ test "$with_system_leveldb" = "yes" ])

# use system libs3?
AC_ARG_WITH([system-libs3],
	[AS_HELP_STRING([--with-system-libs3], [use system libs3])],
	,
	[with_system_libs3=no])
AS_IF([test "x$with_system_libs3" = xyes],
            [AC_CHECK_LIB([s3], [S3_initialize], [true], [AC_MSG_FAILURE([libs3 not found])], [-lpthread])])
AS_IF([test "x$with_system_libs3" = xcheck],
            [AC_SEARCH_LIBS([S3_initialize], [s3], [with_system_libs3=yes], [true], [-lpthread])])
AM_CONDITIONAL(WITH_SYSTEM_LIBS3, [ test "$with_system_libs3" = "yes" ])

# rest-bench?
AC_ARG_WITH([rest-bench],
	    [AS_HELP_STRING([--with-rest-bench], [enables rest-bench])],
	    [],
	    [with_rest_bench=no])
AM_CONDITIONAL(WITH_REST_BENCH, [ test "$with_rest_bench" = "yes" ])

# use libaio?
AC_ARG_WITH([libaio],
            [AS_HELP_STRING([--without-libaio], [disable libaio use by journal])],
            ,
            [with_libaio=yes])
AS_IF([test "x$with_libaio" != xno],
	    [AC_CHECK_LIB([aio], [io_submit], [true], AC_MSG_FAILURE([libaio not found]))])
AS_IF([test "x$with_libaio" != xno],
	    [AC_CHECK_HEADER([libaio.h])])	
AS_IF([test "$with_libaio" = "yes"],
	    [AC_DEFINE([HAVE_LIBAIO], [1], [Defined if you don't have atomic_ops])])
AM_CONDITIONAL(WITH_LIBAIO, [ test "$with_libaio" = "yes" ])

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT


# spirit?
AC_LANG([C++])

AC_CHECK_HEADER([boost/spirit/include/classic_core.hpp], [],
  [AC_CHECK_HEADER([boost/spirit.hpp], [use_bspirit_old_hdr=yes],
    AC_MSG_FAILURE(["Can't find boost spirit headers"]))])
AM_CONDITIONAL(USE_BOOST_SPIRIT_OLD_HDR, [test "$use_bspirit_old_hdr" = "yes"])

AC_CHECK_HEADER([boost/random/discrete_distribution.hpp],
	[AC_DEFINE([HAVE_BOOST_RANDOM_DISCRETE_DISTRIBUTION], [], [have boost::random::discrete_distrubition])],
	[])

AC_CHECK_HEADER([boost/statechart/state.hpp], [],
    AC_MSG_FAILURE(["Can't find boost statechart headers; need 1.34 or later"]))

AC_LANG([C])

AC_CHECK_MEMBER([struct fiemap_extent.fe_logical],
	[AC_DEFINE([HAVE_FIEMAP_H], [], [linux/fiemap.h was found, fiemap ioctl will be used])],
	[AC_MSG_NOTICE([linux/fiemap.h was not found or not usable; using local Ceph copy])],
	[[#include <linux/fiemap.h>]])

AC_CHECK_HEADERS([sys/xattr.h arpa/inet.h netdb.h netinet/in.h sys/file.h sys/ioctl.h sys/mount.h sys/param.h sys/socket.h sys/statvfs.h sys/time.h sys/vfs.h syslog.h utime.h])

# sync_file_range
AC_CHECK_FUNC([sync_file_range],
	[AC_DEFINE([HAVE_SYNC_FILE_RANGE], [], [sync_file_range(2) is supported])],
	[])

# fallocate
AC_CHECK_FUNC([fallocate],
	[AC_DEFINE([CEPH_HAVE_FALLOCATE], [], [fallocate(2) is supported])],
	[])


# Checks for typedefs, structures, and compiler characteristics.
#AC_HEADER_STDBOOL
#AC_C_CONST
#AC_TYPE_UID_T
#AC_C_INLINE
#AC_TYPE_INT16_T
#AC_TYPE_INT32_T
#AC_TYPE_INT64_T
#AC_TYPE_INT8_T
#AC_TYPE_MODE_T
#AC_TYPE_OFF_T
#AC_TYPE_PID_T
#AC_TYPE_SIZE_T
#AC_TYPE_SSIZE_T
#AC_CHECK_MEMBERS([struct stat.st_blksize])
#AC_STRUCT_ST_BLOCKS
#AC_CHECK_MEMBERS([struct stat.st_rdev])
#AC_HEADER_TIME
#AC_STRUCT_TM
#AC_TYPE_UINT16_T
#AC_TYPE_UINT32_T
#AC_TYPE_UINT64_T
#AC_TYPE_UINT8_T

# Checks for library functions.
#AC_FUNC_CHOWN
#AC_FUNC_CLOSEDIR_VOID
#AC_FUNC_ERROR_AT_LINE
#AC_FUNC_FORK
#AC_PROG_GCC_TRADITIONAL
#AC_FUNC_LSTAT
#AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
#AC_FUNC_MALLOC   # this causes annoying rpl_malloc error on some machines; skip it
#AC_FUNC_MEMCMP
#AC_FUNC_MMAP
#AC_FUNC_REALLOC
#AC_FUNC_SELECT_ARGTYPES
#AC_TYPE_SIGNAL
#AC_FUNC_STAT
#AC_FUNC_UTIME_NULL
#AC_CHECK_FUNCS([bzero fchdir fdatasync floor ftruncate getcwd gethostbyname gethostname gettimeofday inet_ntoa localtime_r memmove memset mkdir munmap pow rmdir select socket sqrt strcasecmp strchr strerror strstr utime])

AM_CONDITIONAL(WITH_BUILD_TESTS, test "$WITH_BUILD_TESTS" = "1")

AM_PATH_PYTHON([2.4],
	[], [AC_MSG_FAILURE([Failed to find Python 2.4 or newer])])

AC_CONFIG_HEADERS([src/acconfig.h])
AC_CONFIG_FILES([Makefile
	src/Makefile
	src/ocf/Makefile
	src/ocf/ceph
	src/ocf/rbd
	man/Makefile
	ceph.spec])
AC_OUTPUT
