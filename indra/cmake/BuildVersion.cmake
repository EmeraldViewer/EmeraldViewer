# -*- cmake -*-
include(Python)

function (build_version _target)
  execute_process(
      COMMAND ${PYTHON_EXECUTABLE} ${SCRIPTS_DIR}/svnversion.py
      OUTPUT_VARIABLE SVN_VERSION
      OUTPUT_STRIP_TRAILING_WHITESPACE
      )
  message(STATUS "Svn Version ${SVN_VERSION}")
  # Read version components from the header file.
  file(STRINGS ${LIBS_OPEN_DIR}/llcommon/llversion${_target}.h lines
       REGEX " LL_VERSION_")
  foreach(line ${lines})
    string(REGEX REPLACE ".*LL_VERSION_([A-Z]+).*" "\\1" comp "${line}")
    string(REGEX REPLACE ".* = ([0-9]+);.*" "\\1" value "${line}")
    set(v${comp} "${value}")
  endforeach(line)

  # Compose the version.
  if (${SVN_VERSION} GREATER 0)
    message(STATUS "Using Svn Version ${SVN_VERSION}")
    set(${_target}_VERSION "${vMAJOR}.${vMINOR}.${vPATCH}.${SVN_VERSION}")
  else (${SVN_VERSION} GREATER 0)
    message(STATUS "Using header version ${SVN_VERSION}, ${vBUILD}")
    set(${_target}_VERSION "${vMAJOR}.${vMINOR}.${vPATCH}.${vBUILD}")
  endif (${SVN_VERSION} GREATER 0)
  if (${_target}_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(STATUS "Version of ${_target} is ${${_target}_VERSION}")
  else (${_target}_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "Could not determine ${_target} version (${${_target}_VERSION})")
  endif (${_target}_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+$")

  # Report version to caller.
  set(${_target}_VERSION "${${_target}_VERSION}" PARENT_SCOPE)
endfunction (build_version)
