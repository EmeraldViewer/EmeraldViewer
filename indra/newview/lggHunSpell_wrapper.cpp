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
#include "lggHunSpell_wrapper.h"
#include <boost/regex.hpp>
#include "llweb.h"
#include "llviewercontrol.h"
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

lggHunSpell_Wrapper *glggHunSpell = 0;
Hunspell* lggHunSpell_Wrapper::myHunspell = 0;
//size is 498
static char * countryCodesraw[] = {"SL","SecondLife","AD","Andorra","AE","United Arab Emirates","AF","Afghanistan","AG","Antigua & Barbuda","AI","Anguilla","AL","Albania","AM","Armenia","AN","Netherlands Antilles","AO","Angola","AQ","Antarctica","AR","Argentina","AS","American Samoa","AT","Austria","AU","Australia","AW","Aruba","AZ","Azerbaijan","BA","Bosnia and Herzegovina","BB","Barbados","BD","Bangladesh","BE","Belgium","BF","Burkina Faso","BG","Bulgaria","BH","Bahrain","BI","Burundi","BJ","Benin","BM","Bermuda","BN","Brunei Darussalam","BO","Bolivia","BR","Brazil","BS","Bahama","BT","Bhutan","BU","Burma (no longer exists)","BV","Bouvet Island","BW","Botswana","BY","Belarus","BZ","Belize","CA","Canada","CC","Cocos (Keeling) Islands","CF","Central African Republic","CG","Congo","CH","Switzerland","CI","Côte D'ivoire (Ivory Coast)","CK","Cook Iislands","CL","Chile","CM","Cameroon","CN","China","CO","Colombia","CR","Costa Rica","CS","Czechoslovakia (no longer exists)","CU","Cuba","CV","Cape Verde","CX","Christmas Island","CY","Cyprus","CZ","Czech Republic","DD","German Democratic Republic","DE","Germany","DJ","Djibouti","DK","Denmark","DM","Dominica","DO","Dominican Republic","DZ","Algeria","EC","Ecuador","EE","Estonia","EG","Egypt","EH","Western Sahara","ER","Eritrea","ES","Spain","ET","Ethiopia","FI","Finland","FJ","Fiji","FK","Falkland Islands (Malvinas)","FM","Micronesia","FO","Faroe Islands","FR","France","FX","France"," Metropolitan","GA","Gabon","GB","United Kingdom (Great Britain)","GD","Grenada","GE","Georgia","GF","French Guiana","GH","Ghana","GI","Gibraltar","GL","Greenland","GM","Gambia","GN","Guinea","GP","Guadeloupe","GQ","Equatorial Guinea","GR","Greece","GS","South Georgia and the South Sandwich Islands","GT","Guatemala","GU","Guam","GW","Guinea-Bissau","GY","Guyana","HK","Hong Kong","HM","Heard & McDonald Islands","HN","Honduras","HR","Croatia","HT","Haiti","HU","Hungary","ID","Indonesia","IE","Ireland","IL","Israel","IN","India","IO","British Indian Ocean Territory","IQ","Iraq","IR","Islamic Republic of Iran","IS","Iceland","IT","Italy","JM","Jamaica","JO","Jordan","JP","Japan","KE","Kenya","KG","Kyrgyzstan","KH","Cambodia","KI","Kiribati","KM","Comoros","KN","St. Kitts and Nevis","KP","Korea"," Democratic People's Republic of","KR","Korea"," Republic of","KW","Kuwait","KY","Cayman Islands","KZ","Kazakhstan","LA","Lao People's Democratic Republic","LB","Lebanon","LC","Saint Lucia","LI","Liechtenstein","LK","Sri Lanka","LR","Liberia","LS","Lesotho","LT","Lithuania","LU","Luxembourg","LV","Latvia","LY","Libyan Arab Jamahiriya","MA","Morocco","MC","Monaco","MD","Moldova"," Republic of","MG","Madagascar","MH","Marshall Islands","ML","Mali","MN","Mongolia","MM","Myanmar","MO","Macau","MP","Northern Mariana Islands","MQ","Martinique","MR","Mauritania","MS","Monserrat","MT","Malta","MU","Mauritius","MV","Maldives","MW","Malawi","MX","Mexico","MY","Malaysia","MZ","Mozambique","NA","Namibia","NC","New Caledonia","NE","Niger","NF","Norfolk Island","NG","Nigeria","NI","Nicaragua","NL","Netherlands","NO","Norway","NP","Nepal","NR","Nauru","NT","Neutral Zone","NU","Niue","NZ","New Zealand","OM","Oman","PA","Panama","PE","Peru","PF","French Polynesia","PG","Papua New Guinea","PH","Philippines","PK","Pakistan","PL","Poland","PM","St. Pierre & Miquelon","PN","Pitcairn","PR","Puerto Rico","PT","Portugal","PW","Palau","PY","Paraguay","QA","Qatar","RE","Réunion","RO","Romania","RU","Russian Federation","RW","Rwanda","SA","Saudi Arabia","SB","Solomon Islands","SC","Seychelles","SD","Sudan","SE","Sweden","SG","Singapore","SH","St. Helena","SI","Slovenia","SJ","Svalbard & Jan Mayen Islands","SK","Slovakia","SL","Sierra Leone","SM","San Marino","SN","Senegal","SO","Somalia","SR","Suriname","ST","Sao Tome & Principe","SU","Union of Soviet Socialist Republics (no longer exists)","SV","El Salvador","SY","Syrian Arab Republic","SZ","Swaziland","TC","Turks & Caicos Islands","TD","Chad","TF","French Southern Territories","TG","Togo","TH","Thailand","TJ","Tajikistan","TK","Tokelau","TM","Turkmenistan","TN","Tunisia","TO","Tonga","TP","East Timor","TR","Turkey","TT","Trinidad & Tobago","TV","Tuvalu","TW","Taiwan"," Province of China","TZ","Tanzania"," United Republic of","UA","Ukraine","UG","Uganda","UM","United States Minor Outlying Islands","US","United States of America","UY","Uruguay","UZ","Uzbekistan","VA","Vatican City State (Holy See)","VC","St. Vincent & the Grenadines","VE","Venezuela","VG","British Virgin Islands","VI","United States Virgin Islands","VN","Viet Nam","VU","Vanuatu","WF","Wallis & Futuna Islands","WS","Samoa","YD","Democratic Yemen (no longer exists)","YE","Yemen","YT","Mayotte","YU","Yugoslavia","ZA","South Africa","ZM","Zambia","ZR","Zaire","ZW","Zimbabwe","ZZ","Unknown or unspecified country"};
//size is 371
static char * languageCodesraw[]={"aa","Afar","ab","Abkhazian","ae","Avestan","af","Afrikaans","ak","Akan","am","Amharic","an","Aragonese","ar","Arabic","as","Assamese","av","Avaric","ay","Aymara","az","Azerbaijani","ba","Bashkir","be","Belarusian","bg","Bulgarian","bh","Bihari","bi","Bislama","bm","Bambara","bn","Bengali","bo","Tibetan","br","Breton","bs","Bosnian","ca","Catalan","ce","Chechen","ch","Chamorro","co","Corsican","cr","Cree","cs","Czech","cu","ChurchSlavic","cv","Chuvash","cy","Welsh","da","Danish","de","German","dv","Divehi","dz","Dzongkha","ee","Ewe","el","ModernGreek","en","English","eo","Esperanto","es","Spanish","et","Estonian","eu","Basque","fa","Persian","ff","Fulah","fi","Finnish","fj","Fijian","fo","Faroese","fr","French","fy","WesternFrisian","ga","Irish","gd","Gaelic","gl","Galician","gn","Guaraní","gu","Gujarati","gv","Manx","ha","Hausa","he","ModernHebrew","hi","Hindi","ho","HiriMotu","hr","Croatian","ht","Haitian","hu","Hungarian","hy","Armenian","hz","Herero","ia","Interlingua","id","Indonesian","ie","Interlingue","ig","Igbo","ii","SichuanYi","ik","Inupiaq","io","Ido","is","Icelandic","it","Italian","iu","Inuktitut","ja","Japanese","jv","Javanese","ka","Georgian","kg","Kongo","ki","Kikuyu","kj","Kwanyama","kk","Kazakh","kl","Kalaallisut","km","CentralKhmer","kn","Kannada","ko","Korean","kr","Kanuri","ks","Kashmiri","ku","Kurdish","kv","Komi","kw","Cornish","ky","Kirghiz","la","Latin","lb","Luxembourgish","lg","Ganda","li","Limburgish","ln","Lingala","lo","Lao","lt","Lithuanian","lu","Luba-Katanga","lv","Latvian","mg","Malagasy","mh","Marshallese","mi","Maori","mk","Macedonian","ml","Malayalam","mn","Mongolian","mr","Marathi","ms","Malay","mt","Maltese","my","Burmese","na","Nauru","nb","NorwegianBokmal","nd","NorthNdebele","ne","Nepali","ng","Ndonga","nl","Dutch","Flemish","nn","NorwegianNynorsk","no","Norwegian","nr","SouthNdebele","nv","Navajo","Navaho","ny","Nyanja","oc","Occitan","oj","Ojibwa","om","Oromo","or","Oriya","os","Ossetian","pa","Panjabi","pi","Pali","pl","Polish","ps","Pashto","Pushto","pt","Portuguese","qu","Quechua","rm","Romansh","rn","Rundi","ro","Romanian","ru","Russian","rw","Kinyarwanda","sa","Sanskrit","sc","Sardinian","sd","Sindhi","se","NorthernSami","sg","Sango","si","Sinhala","sk","Slovak","sl","Slovene","sm","Samoan","sn","Shona","so","Somali","sq","Albanian","sr","Serbian","ss","Swati","st","SouthernSotho","su","Sundanese","sv","Swedish","sw","Swahili","ta","Tamil","te","Telugu","tg","Tajik","th","Thai","ti","Tigrinya","tk","Turkmen","tl","Tagalog","tn","Tswana","to","Tonga","tr","Turkish","ts","Tsonga","tt","Tatar","tw","Twi","ty","Tahitian","ug","Uighur","uk","Ukrainian","ur","Urdu","uz","Uzbek","ve","Venda","vi","Vietnamese","vo","Volapük","wa","Walloon","wo","Wolof","xh","Xhosa","yi","Yiddish","yo","Yoruba","za","Zhuang","zh","Chinese","zu","Zulu"};

lggHunSpell_Wrapper::lggHunSpell_Wrapper()
{
	highlightInRed=false;
	//languageCodes(begin(languageCodesraw), end(languageCodesraw));    
}
lggHunSpell_Wrapper::~lggHunSpell_Wrapper(){}

void lggHunSpell_Wrapper::setNewDictionary(std::string newDict)
{
	currentBaseDic=newDict;
	//expecting a full name comming in
	newDict = fullName2DictName(newDict);

	if(myHunspell)delete myHunspell;

	std::string dicaffpath(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "Dictionaries", std::string(newDict+".Aff")).c_str());
	std::string dicdicpath(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "Dictionaries", std::string("Emerald_Custom.Dic")).c_str());
	
	llinfos << "Setting new base dictionary -> " << dicaffpath.c_str() << llendl;

	myHunspell = new Hunspell(dicaffpath.c_str(),dicdicpath.c_str());

	addDictionary(currentBaseDic);


}
void lggHunSpell_Wrapper::addWordToCustomDictionary(std::string wordToAdd)
{
	if(!myHunspell)return;
	myHunspell->add(wordToAdd.c_str());
}
BOOL lggHunSpell_Wrapper::isSpelledRight(std::string wordToCheck)
{
	if(!myHunspell)return TRUE;
	if(wordToCheck.length()<3)return TRUE;
	return myHunspell->spell(wordToCheck.c_str());
}
std::vector<std::string> lggHunSpell_Wrapper::getSuggestionList(std::string badWord)
{
	std::vector<std::string> toReturn;
	if(!myHunspell)return toReturn;
	char ** suggestionList;	
	int numberOfSuggestions = myHunspell->suggest(&suggestionList, badWord.c_str());	
	if(numberOfSuggestions <= 0)
		return toReturn;	
	for (int i = 0; i < numberOfSuggestions; i++) 
	{
		std::string tempSugg(suggestionList[i]);
		toReturn.push_back(tempSugg);
	}
	myHunspell->free_list(&suggestionList,numberOfSuggestions);	
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
		llinfos << testWord.c_str() << " is not spelled correctly, getting suggestions" << llendl;
		std::vector<std::string> suggList;
		suggList.clear();
		suggList = getSuggestionList(testWord);
		llinfos << "Got suggestions.. " << llendl;

		for(int i = 0; i<(int)suggList.size();i++)
		{
			llinfos << "Suggestion for " << testWord.c_str() << ":" << suggList[i].c_str() << llendl;
		}

	}

}
void lggHunSpell_Wrapper::initSettings()
{
	processSettings();
	//debugTest("Hello");
	//debugTest("Helllo");	

}
void lggHunSpell_Wrapper::processSettings()
{
	//expects everything to already be in saved settings
	setNewDictionary(gSavedSettings.getString("EmeraldSpellBase"));
	highlightInRed= gSavedSettings.getBOOL("EmeraldSpellDisplay");
	std::vector<std::string> toInstall = getInstalledDicts();
	for(int i =0;i<(int)toInstall.size();i++)
		addDictionary(toInstall[i]);
}
void lggHunSpell_Wrapper::addDictionary(std::string additionalDictionary)
{
	if(!myHunspell)return;
	//expecting a full name here
	std::string dicpath(
		gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "Dictionaries",
		std::string(fullName2DictName(additionalDictionary)+".Dic")
		).c_str());
	llinfos << "Adding additional dictionary -> " << dicpath.c_str() << llendl;
	myHunspell->add_dic(dicpath.c_str());
}
std::string lggHunSpell_Wrapper::dictName2FullName(std::string dictName)
{
	std::string countryCode="";
	std::string languageCode="";
	//remove extension
	dictName = dictName.substr(0,dictName.find(".")-1);
	//break it up by - or _
	S32 breakPoint = dictName.find("-");
	if(breakPoint==std::string::npos)
		breakPoint = dictName.find("_");
	if(breakPoint==std::string::npos)
	{
		//no country code given
		languageCode=dictName;
	}else
	{
		languageCode=dictName.substr(0,breakPoint);
		countryCode=dictName.substr(breakPoint+1);
	}
	//get long language code
	for(int i =0;i<498;i++)
	{		
		if(LLStringUtil::compareInsensitive(languageCode,languageCodesraw[i]))
		{
			languageCode=languageCodesraw[i+1];
			break;
		}
	}
	//get long country code
	if(countryCode!="")
		for(int i =0;i<371;i++)
		{		
			if(LLStringUtil::compareInsensitive(countryCode,countryCodesraw[i]))
			{
				countryCode=countryCodesraw[i+1];
				break;
			}
		}
	
	return std::string(languageCode+" ("+countryCode+")");
}
std::string lggHunSpell_Wrapper::fullName2DictName(std::string fullName)
{
	std::string countryCode="";
	std::string languageCode="";
	//break it up by - or _
	S32 breakPoint = fullName.find(" (");
	languageCode=fullName.substr(0,breakPoint);
	countryCode=fullName.substr(breakPoint+2,fullName.length()-1-breakPoint);
	
	//get long language code
	for(int i =0;i<498;i++)
	{		
		if(LLStringUtil::compareInsensitive(languageCode,languageCodesraw[i]))
		{
			languageCode=languageCodesraw[i-1];
			break;
		}
	}
	//get long country code
	if(countryCode!="")
		for(int i =0;i<371;i++)
		{		
			if(LLStringUtil::compareInsensitive(countryCode,countryCodesraw[i]))
			{
				countryCode=countryCodesraw[i-1];
				break;
			}
		}
		std::string toReturn = languageCode;
		if(countryCode!="")
		{
			languageCode+="_"+countryCode;
		}
		LLStringUtil::toUpper(toReturn);
		return toReturn;
}
std::vector <std::string> lggHunSpell_Wrapper::getDicts()
{
	std::vector<std::string> names;	
	std::string path_name(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "Dictionaries", ""));
	bool found = true;			
	while(found) 
	{
		std::string name;
		found = gDirUtilp->getNextFileInDir(path_name, "*.Aff", name, false);
		if(found)
		{
			names.push_back(dictName2FullName(name));
		}
	}
	return names;
}
std::vector<std::string> lggHunSpell_Wrapper::getInstalledDicts()
{
	std::vector<std::string> toReturn;
	//expecting short names to be stored...
	std::vector<std::string> shortNames =  CSV2VEC(gSavedSettings.getString("EmeraldSpellInstalled"));
	for(int i =0;i<(int)shortNames.size();i++)
		toReturn.push_back(dictName2FullName(shortNames[i]));
	return toReturn;
}
std::vector<std::string> lggHunSpell_Wrapper::getAvailDicts()
{
	std::vector<std::string> toReturn;
	std::vector<std::string> dics = lggHunSpell_Wrapper::getDicts();
	std::vector<std::string> installedDics = lggHunSpell_Wrapper::getInstalledDicts();
	for(int i =0;i<(int)dics.size();i++)
	{
		bool found = false;
		for(int j=0;j<(int)installedDics.size();j++)
		{
			if(LLStringUtil::compareInsensitive(dics[i],installedDics[j]))
				found=true;//this dic is already installed
			if((LLStringUtil::compareInsensitive(dics[i],currentBaseDic))
				found=true;
		}
		if(!found)toReturn.push_back(dics[i]);
	}
	return toReturn;
}
std::vector<std::string> lggHunSpell_Wrapper::CSV2VEC(std::string csv)
{
	std::vector<std::string> toReturn;
	boost::regex re(",");
	boost::sregex_token_iterator i(csv.begin(), csv.end(), re, -1);
	boost::sregex_token_iterator j;
	while(i != j)
		toReturn.push_back(*i++);
	return toReturn;
}
std::string lggHunSpell_Wrapper::VEC2CSV(std::vector<std::string> vec)
{
	std::string toReturn="";
	for(int i = 0;i<(int)vec.size();i++)
		toReturn+=vec[i]+",";
	return toReturn.erase(1);
}
void lggHunSpell_Wrapper::addButton(std::string selection)
{
	addDictionary(selection);
	gSavedSettings.setString("EmeraldSpellInstalled",
		std::string(gSavedSettings.getString("EmeraldSpellInstalled")+
		","+fullName2DictName(selection)));
}
void lggHunSpell_Wrapper::removeButton(std::string selection)
{
	std::vector<std::string> newInstalledDics;
	std::vector<std::string> currentlyInstalled = getInstalledDicts();
	for(int i =0;i<(int)currentlyInstalled.size();i++)
	{
		if(!LLStringUtil::compareInsensitive(selection,currentlyInstalled[i]))
			newInstalledDics.push_back(currentlyInstalled[i]);
	}
	gSavedSettings.setString("EmeraldSpellInstalled",VEC2CSV(newInstalledDics));
	processSettings();
}
void lggHunSpell_Wrapper::newDictSelection(std::string selection)
{
	gSavedSettings.setString("EmeraldSpellBase",selection);
	processSettings();
}
void lggHunSpell_Wrapper::getMoreButton()
{
	LLWeb::loadURL("http://www.cypherpunks.ca/otr/");
}
void lggHunSpell_Wrapper::editCustomButton()
{
	std::string dicdicpath(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "Dictionaries", std::string("Emerald_Custom.Dic")).c_str());
	gViewerWindow->getWindow()->ShellEx(dicdicpath);
}