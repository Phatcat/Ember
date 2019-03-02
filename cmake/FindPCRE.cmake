# Copyright (C) 2007-2009 LuaDist.
# Created by Peter Kapec <kapecp@gmail.com>
# Redistribution and use of this file is allowed according to the terms of the MIT license.
# For details see the COPYRIGHT file distributed with LuaDist.
#	Note:
#		Searching headers and libraries is very simple and is NOT as powerful as scripts
#		distributed with CMake, because LuaDist defines directories to search for.
#		Everyone is encouraged to contact the author with improvements. Maybe this file
#		becomes part of CMake distribution sometimes.

# - Find pcre
# Find the native PCRE headers and libraries.
#
# PCRE_INCLUDE_DIRS	- where to find pcre.h, etc.
# PCRE_LIBRARIES	- List of libraries when using pcre.
# PCRE_FOUND	- True if pcre found.

# Look for the header file.
FIND_PATH(PCRE_ROOT_DIR
          NAMES pcre.h
          PATHS ENV PCREPROOT
          DOC "pcre root directory")

# Look for the libraries.
FIND_LIBRARY(PCRE_LIBRARY_RELEASE
             NAMES pcre
             HINTS ${PCRE_ROOT_DIR}
             PATH_SUFFIXES lib
             DOC "pcre release library")

FIND_LIBRARY(PCRE_LIBRARY_DEBUG
             NAMES pcred
             HINTS ${PCRE_ROOT_DIR}
             PATH_SUFFIXES lib
             DOC "pcre debug library")

# Handle the QUIETLY and REQUIRED arguments and set PCRE_FOUND to TRUE if all listed variables are TRUE.
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PCRE DEFAULT_MSG PCRE_LIBRARY PCRE_INCLUDE_DIR)

IF(PCRE_LIBRARY_DEBUG AND PCRE_LIBRARY_RELEASE)
  SET(PCRE_LIBRARY
      optimized ${PCRE_LIBRARY_RELEASE}
      debug ${PCRE_LIBRARY_DEBUG} CACHE DOC "pcre library")
ELSEIF(PCRE_LIBRARY_RELEASE)
  SET(PCRE_LIBRARY ${PCRE_LIBRARY_RELEASE} CACHE DOC "pcre library")
ENDIF(PCRE_LIBRARY_DEBUG AND PCRE_LIBRARY_RELEASE)

# Copy the results to the output variables.
IF(PCRE_FOUND)
	SET(PCRE_LIBRARIES ${PCRE_LIBRARY})
	SET(PCRE_INCLUDE_DIRS ${PCRE_ROOT_DIR})
ELSE(PCRE_FOUND)
	SET(PCRE_LIBRARIES)
	SET(PCRE_INCLUDE_DIRS)
ENDIF(PCRE_FOUND)
