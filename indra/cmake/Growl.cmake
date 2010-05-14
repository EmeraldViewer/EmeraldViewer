# -*- cmake -*-
include(Prebuilt)

use_prebuilt_binary(Growl)
if(DARWIN OR WINDOWS)
	set(GROWL_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/Growl)
endif(DARWIN OR WINDOWS)

if(WINDOWS)
    set(GROWL_LIBRARY libgrowl-shared++)
else(WINDOWS)
    set(GROWL_LIBRARY Growl)
endif(WINDOWS)
