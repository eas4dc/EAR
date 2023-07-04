AC_DEFUN([AX_SET_ARCHITECTURES],
[
	AC_ARG_ENABLE([arm64],
		AS_HELP_STRING([--enable-arm64], [Compiles for ARM64 architecture])
        )

    if test "x$enable_arm64" = "xyes"; then
        ARCH=ARM
    else
        ARCH=X86
    fi

    AC_SUBST(ARCH)
])
