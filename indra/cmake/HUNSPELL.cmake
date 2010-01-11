# -*- cmake -*-

IF(WINDOWS)
	include(Prebuilt)
	set(HUNSPELL_LIBRARY libhunspell)
ELSE(WINDOWS)
	FIND_PACKAGE(HunSpell REQUIRED)
ENDIF(WINDOWS)
