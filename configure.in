AC_INIT([lightspeed], [13.8.0], [ondra-novak@email.cz])

echo "Exporting list of headers"
cd src
LIGHTSPEED_ALL_HEADERS=`find lightspeed \( -name "*.h" -or -name "*.tcc" \) -exec echo -n " {}" \;`
cd ..

AC_CONFIG_AUX_DIR([Build])
AC_CONFIG_MACRO_DIR([Build])
AC_SUBST(LIGHTSPEED_ALL_HEADERS,$LIGHTSPEED_ALL_HEADERS)
AM_INIT_AUTOMAKE([foreign])
# check for pipe2 function
AC_CHECK_FUNCS(pipe2)
# check for vfork function
AC_CHECK_FUNCS(vfork)
# check for modern atomic functions
AC_CHECK_DECLS_ONCE([__atomic_compare_exchange])
AC_PROG_INSTALL
AC_CONFIG_HEADER(src/lightspeed/platform.h:src/lightspeed/platform.in)
AC_DISABLE_SHARED
AC_PROG_LIBTOOL
AC_PROG_CXX
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
