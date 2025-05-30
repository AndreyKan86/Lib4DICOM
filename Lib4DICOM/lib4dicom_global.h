#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(LIB4DICOM_LIB)
#  define LIB4DICOM_EXPORT Q_DECL_EXPORT
# else
#  define LIB4DICOM_EXPORT Q_DECL_IMPORT
# endif
#else
# define LIB4DICOM_EXPORT
#endif
