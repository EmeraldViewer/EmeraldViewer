# -*- cmake -*-
include(Prebuilt)

use_prebuilt_binary(otr)

#set(OTR_LIBRARY otr)
if (NOT LINUX)
	set(GCRYPT_LIBRARY libgcrypt)
	set(GPG-ERROR_LIBRARY libgpg-error)
	set(OTR_LIBRARY libotr)
endif (NOT LINUX)

set(LIBOTR_INCLUDE_DIRS
    )

set(LIBOTR_LIBRARY
    otr
    )
