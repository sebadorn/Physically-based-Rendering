# Source: http://www.openengine.dk/code/extensions/Assimp/FindAssimp.cmake

# - Try to find Assimp
# Once done this will define
#
#  ASSIMP_FOUND - system has Assimp
#  ASSIMP_INCLUDE_DIR - the Assimp include directory
#  ASSIMP_LIBRARIES - Link these to use Assimp
#

STRING(COMPARE EQUAL ${CMAKE_BUILD_TYPE} "debug" ISDEBUGENABLED)

SET(ASSIMP "assimp")

FIND_PATH(ASSIMP_INCLUDE_DIR NAMES assimp.h
  PATHS
  ${ASSIMP_DEPS_INCLUDE_DIR}
  ${PROJECT_BINARY_DIR}/include
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/libraries/assimp/include
  ENV CPATH
  /usr/include
  /usr/local/include
  /opt/local/include
  NO_DEFAULT_PATH
)


FIND_LIBRARY(LIBASSIMP
  NAMES
  ${ASSIMP}
  PATHS
  ${ASSIMP_DEPS_LIB_DIR}
  ${PROJECT_SOURCE_DIR}/libraries/assimp/lib
  ENV LD_LIBRARY_PATH
  ENV LIBRARY_PATH
  /usr/lib
  /usr/local/lib
  /opt/local/lib
  NO_DEFAULT_PATH
)

SET (ASSIMP_LIBRARIES
  ${LIBASSIMP}
)

IF(ASSIMP_INCLUDE_DIR AND ASSIMP_LIBRARIES)
   SET(ASSIMP_FOUND TRUE)
ENDIF(ASSIMP_INCLUDE_DIR AND ASSIMP_LIBRARIES)

# show the COLLADA_DOM_INCLUDE_DIR and COLLADA_DOM_LIBRARIES variables only in the advanced view
IF(ASSIMP_FOUND)
  MARK_AS_ADVANCED(ASSIMP_INCLUDE_DIR ASSIMP_LIBRARIES )
ENDIF(ASSIMP_FOUND)


