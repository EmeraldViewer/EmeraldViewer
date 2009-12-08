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

#include "llviewerprecompiledheaders.h"
#include "lggHunSpell_Wrapper.h"

//#include "hunspelldll.h"
//#include "hunspell\hunspelldll.h"
//#include "HSpellEdit.h"

lggHunSpell_Wrapper *glggHunSpell = 0;
Hunspell* lggHunSpell_Wrapper::myHunspell = 0;

lggHunSpell_Wrapper::lggHunSpell_Wrapper()
{



}

lggHunSpell_Wrapper::~lggHunSpell_Wrapper()
{
    if (glggHunSpell)
    {  
		//delete_aspell_speller(chat_send_spell_checker);

		//delete_aspell_config(spell_config);
        //otrl_userstate_free(gOTR->userstate);
    }
}

void lggHunSpell_Wrapper::setNewDictionary(std::string newDict)
{
	if(myHunspell)delete myHunspell;

	std::string dicaffpath(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "Dictionaries", std::string(newDict+".aff")).c_str());
	std::string dicdicpath(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "Dictionaries", std::string(newDict+".dic")).c_str());


	myHunspell = new Hunspell(dicaffpath.c_str(),dicdicpath.c_str());

}
BOOL lggHunSpell_Wrapper::isSpelledRight(std::string wordToCheck)
{
	if(!myHunspell)return TRUE;
	if(wordToCheck.length()<3)return TRUE;
	return myHunspell->spell(wordToCheck.c_str());
}
std::vector<std::string> lggHunSpell_Wrapper::getSujestionList(std::string badWord)
{
	std::vector<std::string> toReturn;
	if(!myHunspell)return toReturn;
	char ** sujestionList;	
	int numberOfSujestions = myHunspell->suggest(&sujestionList, badWord.c_str());	
	if(numberOfSujestions <= 0)
		return toReturn;	
	for (int i = 0; i < numberOfSujestions; i++) 
	{
		std::string tempSuj(sujestionList[i]);
		toReturn.push_back(tempSuj);
	}
	myHunspell->free_list(&sujestionList,numberOfSujestions);	
	return toReturn;
}
void lggHunSpell_Wrapper::debugTest(std::string testWord)
{
	llinfos << "Testing to see if " << testWord.c_str() << " is spelled correct" << llendl;

	if( isSpelledRight(testWord))
	{
		llinfos << testWord.c_str() << " is spelled correctly" << llendl;
	}else
	{
		llinfos << testWord.c_str() << " is not spelled correctly, getting sujestions" << llendl;
		std::vector<std::string> sujList;
		sujList.clear();
		sujList = getSujestionList(testWord);
		llinfos << "Got sujestions.. " << llendl;

		for(int i = 0; i<(int)sujList.size();i++)
		{
			llinfos << "Sujestion for " << testWord.c_str() << ":" << sujList[i].c_str() << llendl;
		}

	}

}
void lggHunSpell_Wrapper::initSettings()
{
	setNewDictionary("en_US");
	debugTest("Hello");
	debugTest("Helllo");	

}
std::vector <std::string> lggHunSpell_Wrapper::getDicts()
{

	std::vector<std::string> toReturn;
	/*
	std::vector<std::string> toReturn;
	AspellDictInfoList * dlist;
	AspellDictInfoEnumeration * dels;
	const AspellDictInfo * entry;

	AspellConfig * spell_config = (AspellConfig *)malloc(32000);
	
	spell_config = new_aspell_config();

	
	/* the returned pointer should _not_ need to be deleted 
	dlist = get_aspell_dict_info_list(spell_config);

	delete_aspell_config(spell_config);

	dels = aspell_dict_info_list_elements(dlist);

	//printf("%-30s%-8s%-20s%-6s%-10s\n", "NAME", "CODE", "JARGON", "SIZE", "MODULE");
	while ( (entry = aspell_dict_info_enumeration_next(dels)) != 0) 
	{
		toReturn.push_back(llformat("%-30s%-8s%-20s%-6s%-10s\n",
			entry->name,
			entry->code, entry->jargon, 
			entry->size_str, entry->module->name));
	}

	delete_aspell_dict_info_enumeration(dels);
	*/
	return toReturn;
}