dnl GEBR_PYTHON_CHECK([MODULE], [IF-FOUND], [IF-NOT-FOUND])
# serial 1 GEBR_PYTHON_CHECK
AC_DEFUN([GEBR_PYTHON_CHECK], [
AC_MSG_CHECKING(for module $1 in python)
echo "import $1" | ${PYTHON} -
if test $? -ne 0 ; then
	AC_MSG_RESULT(not found)
	AC_MSG_ERROR(You need the module $1 available to python for this package)
fi
AC_MSG_RESULT(found)
])
