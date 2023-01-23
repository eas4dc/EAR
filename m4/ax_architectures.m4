AC_DEFUN([AX_SET_ARCHITECTURES],
[
	AC_ARG_ENABLE([arm64],
		AS_HELP_STRING([--enable-arm64], [Compiles for ARM64 architecture])
        )

    if test "x$enable_arm64" = "xyes"; then
        ARCH_X86=0
        ARCH_ARM=1
    else
        ARCH_X86=1
        ARCH_ARM=0
    fi

    AC_SUBST(ARCH_X86)
    AC_SUBST(ARCH_ARM)
])
