#pragma once
#include <QtCore/qglobal.h>

#if defined(LIB4DICOM_LIBRARY)
#  define LIB4DICOM_EXPORT Q_DECL_EXPORT
#else
#  define LIB4DICOM_EXPORT Q_DECL_IMPORT
#endif
