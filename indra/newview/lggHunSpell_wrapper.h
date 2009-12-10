/* Copyright (C) 2009 LordGregGreg Back

   This is free software; you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.
 
   This is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.
 
   You should have received a copy of the GNU Lesser General Public
   License along with the viewer; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

#ifndef ASPELL_WRAPPER
#define ASPELL_WRAPPER 1


#include "hunspell\hunspelldll.h"

class lggHunSpell_Wrapper
{

public:
	static Hunspell * myHunspell;

	void initSettings();
	std::vector<std::string> getDicts();
	void setNewDictionary(std::string newDict);
	BOOL isSpelledRight(std::string wordToCheck);
	std::vector<std::string> getSujestionList(std::string badWord);
	S32 findNextError(std::string haystack, int startAt);

private:
	lggHunSpell_Wrapper();
	~lggHunSpell_Wrapper();
	void debugTest(std::string testWord);//prints out debug about testing the word
};

extern lggHunSpell_Wrapper *glggHunSpell; // the singleton hunspell wrapper

#endif 
