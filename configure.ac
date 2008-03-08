AC_INIT(FAST-ER,1.0)
AC_COPYRIGHT(Copyright Los Alamos National Laboratory)


orig_CXXFLAGS="$CXXFLAGS"

AC_PROG_AWK
AC_PROG_CXX

AC_LANG(C++)


################################################################################
#
# Check for libcvd
#
cvd=yes
AC_CHECK_HEADERS(cvd/image_io.h, [], [cvd=no])

AC_MSG_CHECKING(libCVD)

LDFLAGS="$LDFLAGS -lcvd"

comp=no
AC_COMPILE_IFELSE([
	#include <cvd/image_io.h>
	int main(){ CVD::Image<CVD::byte> i = CVD::img_load(""); return 0;}
], [comp=yes], [cvd=no])

AC_MSG_RESULT([$comp])

if test $cvd = no
then
	AC_MSG_ERROR([A working copy of libCVD is required for this program.
Get libCVD here: http://savannah.nongnu.org/projects/libcvd
])
fi


################################################################################
#
# Check for TooN
#
toon=yes
AC_CHECK_HEADERS(TooN/TooN.h, [], [toon=no])

comp=no
AC_MSG_CHECKING([TooN])
AC_COMPILE_IFELSE([
	#include <TooN/TooN.h>
	int main(){TooN::Vector<2> v;return 0;}
], [comp=yes], [toon=no])

AC_MSG_RESULT($comp)

if test $toon = no
then
	AC_MSG_ERROR([A working copy of TooN is required for this program.
Get TooN here: http://savannah.nongnu.org/projects/TooN
])
fi


################################################################################
#
# Check for TaG
#
tag=yes
AC_CHECK_HEADERS(tag/printf.h, [], [tag=no])

comp=no
AC_MSG_CHECKING([tag])
AC_COMPILE_IFELSE([
	#include <tag/printf.h>
	int main(){Printf("Hello, world.\n");}
], [comp=yes], [tag=no])

AC_MSG_RESULT($comp)

if test $tag = no
then
	AC_MSG_ERROR([A working copy of tag is required for this program.
Get tag here: http://savannah.nongnu.org/projects/TooN
])
fi



################################################################################
#
# Check for GVars
#
gvars=yes
AC_CHECK_HEADERS(gvars3/instances.h, [], [gvars=no])

AC_ARG_ENABLE(headless, [  --enable-headless-gvars       Use headless GVars (useful for batch jobs)])

if test "$enable_headless" = yes
then
	LDFLAGS="$LDFLAGS -lGVars3_headless"
else
	LDFLAGS="$LDFLAGS -lGVars3"
fi


comp=no
AC_MSG_CHECKING([gvars])
AC_COMPILE_IFELSE([
	#include <gvars3/instances.h>
	int main(){GVars3::GUI.LoadFile("");}
], [comp=yes], [gvars=no])

AC_MSG_RESULT($comp)

if test $gvars = no
then
	AC_MSG_ERROR([A working copy of GVars3 is required for this program.
Get GVars3 here: http://savannah.nongnu.org/projects/libcvd
])
fi


################################################################################
#
# JIT compiler
#

code=yes
comp=no
AC_MSG_CHECKING(32 bit x86 machine code)

AC_RUN_IFELSE([
#include <sys/mman.h>
int main()
{
	unsigned char* c = (unsigned char*) mmap(0, 1, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	*c = 0xc3; //Ret
	
	__asm__ __volatile__(
		"	call *%0	"
		:
		: "m"(c)
	);

	return 0;
}], [comp=yes], [code=no]);

AC_MSG_RESULT($comp)

if test $code = no
then
	echo
	echo Warning:
	echo JIT compilation is not enabled: the program will run very slowly.
	echo You need a working 32 bit x86 executable to enable JIT.
	echo
	CXXFLAGS="$CXXFLAGS -DNOJIT"
else
	CXXFLAGS="$CXXFLAGS -DJIT"
fi

AC_ARG_ENABLE(debug, [  --enable-debugging            compile with debugging enabled])
if test "$enable_debug" = yes
then
	CXXFLAGS="$CXXFLAGS -DSAFE_VECTOR -DTOON_DEBUG -DCVD_IMAGE_DEBUG"
else
	CXXFLAGS="$CXXFLAGS -DNDEBUG"
fi


AC_OUTPUT(Makefile)
