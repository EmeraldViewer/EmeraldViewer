/* Copyright (c) 2009
 *
 * Modular Systems All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *   3. Neither the name Modular Systems nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MODULAR SYSTEMS AND CONTRIBUTORS “AS IS”
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MODULAR SYSTEMS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "llviewerprecompiledheaders.h"

#include "jclslpreproc.h"

#include "llagent.h"

#include "emerald.h"

#include "llcurl.h"

#include "llscrolllistctrl.h"

#include "llviewertexteditor.h"

#include "llinventorymodel.h"

#include "llversionviewer.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>


//apparently LL #defined this function which happens to precisely match
//a boost::wave function name, destroying the internet, silly grey furries
#undef equivalent

#include <boost/assert.hpp>
#include <boost/program_options.hpp>

#include <boost/wave.hpp>


#include <boost/wave/cpplexer/cpp_lex_token.hpp>    // token class
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp> // lexer class

#include <boost/wave/preprocessing_hooks.hpp>

///////////////////////////////////////////////////////////////////////////////
//  include lexer specifics, import lexer names
#if BOOST_WAVE_SEPARATE_LEXER_INSTANTIATION == 0
#include <boost/wave/cpplexer/re2clex/cpp_re2c_lexer.hpp>
#endif 

#include <boost/regex.hpp>

#include <boost/filesystem.hpp>


using namespace boost::spirit::classic;
namespace po = boost::program_options;


using namespace boost::regex_constants;

void cmdline_printchat(std::string message);

std::map<std::string,LLUUID> JCLSLPreprocessor::cached_assetids;


#define FAILDEBUG llinfos << "line:" << __LINE__ << llendl;


#define encode_start std::string("//start_unprocessed_text\n/*")
#define encode_end std::string("*/\n//end_unprocessed_text")

BOOL JCLSLPreprocessor::mono_directive(std::string& text, bool agent_inv)
{
	//FAILDEBUG
	BOOL domono = agent_inv;
	//FAILDEBUG
	if(text.find("//mono\n") != -1)
	{
		//FAILDEBUG
		domono = TRUE;
	}else if(text.find("//lsl2\n") != -1)
	{
		//FAILDEBUG
		domono = FALSE;
	}
	//FAILDEBUG
	return domono;
}

std::string JCLSLPreprocessor::encode(std::string script)
{
	//FAILDEBUG
	std::string otext = JCLSLPreprocessor::decode(script);
	//FAILDEBUG
	BOOL mono = mono_directive(script);
	//FAILDEBUG
	otext = boost::regex_replace(otext, boost::regex("([/*])(?=[/*|])",boost::regex::perl), "$1|");
	//FAILDEBUG
	//otext = curl_escape(otext.c_str(), otext.size());
	//FAILDEBUG
	otext = encode_start+otext+encode_end;
	//FAILDEBUG
	otext += "\n//nfo_preprocessor_version 0";
	//FAILDEBUG
	otext += "\n//^ = determine what featureset is supported";
	//FAILDEBUG
	otext += llformat("\n//program_version %s", LLAppViewer::instance()->getSecondLifeTitle().c_str());
	//FAILDEBUG
	otext += "\n";
	//FAILDEBUG
	if(mono)otext += "//mono\n";
	else otext += "//lsl2\n";
	//FAILDEBUG

	return otext;
}

std::string JCLSLPreprocessor::decode(std::string script)
{
	//FAILDEBUG
	static S32 startpoint = encode_start.length();
	//FAILDEBUG
	std::string tip = script.substr(0,startpoint);
	//FAILDEBUG
	if(tip != encode_start)
	{
		//FAILDEBUG
		//cmdline_printchat("no start");
		//if(sp != -1)trigger warningg/error?
		return script;
	}
	//FAILDEBUG
	S32 end = script.find(encode_end);
	//FAILDEBUG
	if(end == -1)
	{
		//FAILDEBUG
		//cmdline_printchat("no end");
		return script;
	}
	//FAILDEBUG

	std::string data = script.substr(startpoint,end-startpoint);
	//cmdline_printchat("data="+data);
	//FAILDEBUG

	std::string otext = data;

	//FAILDEBUG

	otext = boost::regex_replace(otext, boost::regex("([/*])\\|",boost::regex::perl), "$1");

	//FAILDEBUG
	//otext = curl_unescape(otext.c_str(),otext.length());

	return otext;
}

inline S32 const_iterator2pos(std::string::const_iterator it, std::string::const_iterator begin)
{
	//FAILDEBUG
	S32 pos = 1;
	//FAILDEBUG
	while(it != begin)
	{
		//FAILDEBUG
		pos += 1;
		//FAILDEBUG
		it -= 1;
	}
	//FAILDEBUG
	return pos;
}
/*
//I know this is one ugly ass function but it does the job eh
std::string scoperip(std::string& top, S32 fstart)
{
	S32 up = top.find("{",fstart);
	//cmdline_printchat(llformat("up=%d",up));
	S32 down = top.find("}",fstart);
	S32 ldown = 0;
	//cmdline_printchat(llformat("down=%d",down));
	S32 count = 0;
	do
	{
		while(up < down && up != -1)
		{
			up = top.find("{",up+1);
			//cmdline_printchat(llformat("up=%d",up));
			count += 1;
		}
		while((down < up || up == -1) && count > 0 && down != -1)
		{
			ldown = down;
			down = top.find("}",down+1);
			//cmdline_printchat(llformat("down=%d",down));
			count -= 1;
		}
		//cmdline_printchat(llformat("count=%d",count));
	}while(count > 0 && up != -1 && down != -1);
	
	std::string function = top.substr(fstart,(ldown-fstart)+1);
	
	//cmdline_printchat("func=["+function+"]");
	return function;
}*/
//letstry that again, last function was fundamentally flawed and didn't take string x = "oolol{olo\"lol}"; into consideration
std::string scopeript2(std::string& top, S32 fstart, char left = '{', char right = '}')
{
	//FAILDEBUG
	if(fstart >= int(top.length()))return "begin out of bounds";
	//FAILDEBUG
	int cursor = fstart;
	bool noscoped = true;
	bool in_literal = false;
	int count = 0;
	char ltoken = ' ';
	//FAILDEBUG
	do
	{
		char token = top.at(cursor);
		if(token == '"' && ltoken != '\\')in_literal = !in_literal;
		else if(token == '\\' && ltoken == '\\')token = ' ';
		else if(!in_literal)
		{
			if(token == left)
			{
				count += 1;
				noscoped = false;
			}else if(token == right)
			{
				count -= 1;
				noscoped = false;
			}
		}
		ltoken = token;
		cursor += 1;
	}while((count > 0 || noscoped) && cursor < int(top.length()));
	int end = (cursor-fstart);
	if(end > int(top.length()))return "end out of bounds";//end = int(top.length()) - 1;
	//FAILDEBUG
	return top.substr(fstart,(cursor-fstart));
}
/*
int getscope(std::string& file, int cursor, char left = '{', char right = '}')
{
	int filelen = file.length();
	if(cursor < filelen)
	{
		int pos = 0;
		bool in_literal = false;
		int scope = 0;
		char ltoken = ' ';
		do
		{
			char token = file.at(pos);
			if(token == '"' && ltoken != '\\')in_literal = !in_literal;
			else if(token == '\\' && ltoken == '\\')token = ' ';
			else if(!in_literal)
			{
				if(token == left)
				{
					scope += 1;
				}else if(token == right)
				{
					scope -= 1;
				}
			}
			ltoken = token;
			pos += 1;
		}while(pos <= cursor);

		return scope;
	}
	return -1;
}*/
/*
//checks if the item at "it" is accessible at "me"
bool in_my_scope(std::string& file, int it, int me, char left = '{', char right = '}')
{
	int scope = getscope(file, me);
	if(me < it)return false;//the future isnt behind us
	bool accessible = true;
	bool in_literal = false;
	int relative_scope = scope;
	int out_of_scope = 0;
	char ltoken = ' ';
	do
	{
		char token = file.at(me);
		if(ltoken == '"' && token != '\\')in_literal = !in_literal;
		else if(ltoken == '\\' && token == '\\')token = ' ';
		else if(!in_literal)
		{
			if(token == left)
			{
				relative_scope -= 1;
				out_of_scope -= 1;
				if(out_of_scope < 0)out_of_scope = 0;
			}else if(token == right)
			{
				relative_scope += 1;
				out_of_scope += 1;
			}
		}
		ltoken = token;
		me -= 1;
	}while(it < me);
	if(out_of_scope > 0)accessible = false;
	return accessible;
}

static std::map<std::string, std::string> llFuncs;
void init_funcs()
{
	llFuncs.clear();
	llFuncs[std::string("llSin")] = std::string("float");
	llFuncs[std::string("llCos")] = std::string("float");
	llFuncs[std::string("llTan")] = std::string("float");
	llFuncs[std::string("llAtan2")] = std::string("float");
	llFuncs[std::string("llSqrt")] = std::string("float");
	llFuncs[std::string("llPow")] = std::string("float");
	llFuncs[std::string("llAbs")] = std::string("integer");
	llFuncs[std::string("llFabs")] = std::string("float");
	llFuncs[std::string("llFrand")] = std::string("float");
	llFuncs[std::string("llFloor")] = std::string("integer");
	llFuncs[std::string("llCeil")] = std::string("integer");
	llFuncs[std::string("llRound")] = std::string("integer");
	llFuncs[std::string("llVecMag")] = std::string("float");
	llFuncs[std::string("llVecNorm")] = std::string("vector");
	llFuncs[std::string("llVecDist")] = std::string("float");
	llFuncs[std::string("llRot2Euler")] = std::string("vector");
	llFuncs[std::string("llEuler2Rot")] = std::string("rotation");
	llFuncs[std::string("llAxes2Rot")] = std::string("rotation");
	llFuncs[std::string("llRot2Fwd")] = std::string("vector");
	llFuncs[std::string("llRot2Left")] = std::string("vector");
	llFuncs[std::string("llRot2Up")] = std::string("vector");
	llFuncs[std::string("llRotBetween")] = std::string("rotation");
	llFuncs[std::string("llWhisper")] = std::string("void");
	llFuncs[std::string("llSay")] = std::string("void");
	llFuncs[std::string("llShout")] = std::string("void");
	llFuncs[std::string("llListen")] = std::string("integer");
	llFuncs[std::string("llListenControl")] = std::string("void");
	llFuncs[std::string("llListenRemove")] = std::string("void");
	llFuncs[std::string("llSensor")] = std::string("void");
	llFuncs[std::string("llSensorRepeat")] = std::string("void");
	llFuncs[std::string("llSensorRemove")] = std::string("void");
	llFuncs[std::string("llDetectedName")] = std::string("string");
	llFuncs[std::string("llDetectedKey")] = std::string("key");
	llFuncs[std::string("llDetectedOwner")] = std::string("key");
	llFuncs[std::string("llDetectedType")] = std::string("integer");
	llFuncs[std::string("llDetectedPos")] = std::string("vector");
	llFuncs[std::string("llDetectedVel")] = std::string("vector");
	llFuncs[std::string("llDetectedGrab")] = std::string("vector");
	llFuncs[std::string("llDetectedRot")] = std::string("rotation");
	llFuncs[std::string("llDetectedGroup")] = std::string("integer");
	llFuncs[std::string("llDetectedLinkNumber")] = std::string("integer");
	llFuncs[std::string("llDie")] = std::string("void");
	llFuncs[std::string("llGround")] = std::string("float");
	llFuncs[std::string("llCloud")] = std::string("float");
	llFuncs[std::string("llWind")] = std::string("vector");
	llFuncs[std::string("llSetStatus")] = std::string("void");
	llFuncs[std::string("llGetStatus")] = std::string("integer");
	llFuncs[std::string("llSetScale")] = std::string("void");
	llFuncs[std::string("llGetScale")] = std::string("vector");
	llFuncs[std::string("llSetColor")] = std::string("void");
	llFuncs[std::string("llGetAlpha")] = std::string("float");
	llFuncs[std::string("llSetAlpha")] = std::string("void");
	llFuncs[std::string("llGetColor")] = std::string("vector");
	llFuncs[std::string("llSetTexture")] = std::string("void");
	llFuncs[std::string("llScaleTexture")] = std::string("void");
	llFuncs[std::string("llOffsetTexture")] = std::string("void");
	llFuncs[std::string("llRotateTexture")] = std::string("void");
	llFuncs[std::string("llGetTexture")] = std::string("string");
	llFuncs[std::string("llSetPos")] = std::string("void");
	llFuncs[std::string("llGetPos")] = std::string("vector");
	llFuncs[std::string("llGetLocalPos")] = std::string("vector");
	llFuncs[std::string("llSetRot")] = std::string("void");
	llFuncs[std::string("llGetRot")] = std::string("rotation");
	llFuncs[std::string("llGetLocalRot")] = std::string("rotation");
	llFuncs[std::string("llSetForce")] = std::string("void");
	llFuncs[std::string("llGetForce")] = std::string("vector");
	llFuncs[std::string("llTarget")] = std::string("integer");
	llFuncs[std::string("llTargetRemove")] = std::string("void");
	llFuncs[std::string("llRotTarget")] = std::string("integer");
	llFuncs[std::string("llRotTargetRemove")] = std::string("void");
	llFuncs[std::string("llMoveToTarget")] = std::string("void");
	llFuncs[std::string("llStopMoveToTarget")] = std::string("void");
	llFuncs[std::string("llApplyImpulse")] = std::string("void");
	llFuncs[std::string("llApplyRotationalImpulse")] = std::string("void");
	llFuncs[std::string("llSetTorque")] = std::string("void");
	llFuncs[std::string("llGetTorque")] = std::string("vector");
	llFuncs[std::string("llSetForceAndTorque")] = std::string("void");
	llFuncs[std::string("llGetVel")] = std::string("vector");
	llFuncs[std::string("llGetAccel")] = std::string("vector");
	llFuncs[std::string("llGetOmega")] = std::string("vector");
	llFuncs[std::string("llGetTimeOfDay")] = std::string("float");
	llFuncs[std::string("llGetWallclock")] = std::string("float");
	llFuncs[std::string("llGetTime")] = std::string("float");
	llFuncs[std::string("llResetTime")] = std::string("void");
	llFuncs[std::string("llGetAndResetTime")] = std::string("float");
	llFuncs[std::string("llSound")] = std::string("void");
	llFuncs[std::string("llPlaySound")] = std::string("void");
	llFuncs[std::string("llLoopSound")] = std::string("void");
	llFuncs[std::string("llLoopSoundMaster")] = std::string("void");
	llFuncs[std::string("llLoopSoundSlave")] = std::string("void");
	llFuncs[std::string("llPlaySoundSlave")] = std::string("void");
	llFuncs[std::string("llTriggerSound")] = std::string("void");
	llFuncs[std::string("llStopSound")] = std::string("void");
	llFuncs[std::string("llPreloadSound")] = std::string("void");
	llFuncs[std::string("llGetSubString")] = std::string("string");
	llFuncs[std::string("llDeleteSubString")] = std::string("string");
	llFuncs[std::string("llInsertString")] = std::string("string");
	llFuncs[std::string("llToUpper")] = std::string("string");
	llFuncs[std::string("llToLower")] = std::string("string");
	llFuncs[std::string("llGiveMoney")] = std::string("void");
	llFuncs[std::string("llMakeExplosion")] = std::string("void");
	llFuncs[std::string("llMakeFountain")] = std::string("void");
	llFuncs[std::string("llMakeSmoke")] = std::string("void");
	llFuncs[std::string("llMakeFire")] = std::string("void");
	llFuncs[std::string("llRezObject")] = std::string("void");
	llFuncs[std::string("llLookAt")] = std::string("void");
	llFuncs[std::string("llStopLookAt")] = std::string("void");
	llFuncs[std::string("llSetTimerEvent")] = std::string("void");
	llFuncs[std::string("llSleep")] = std::string("void");
	llFuncs[std::string("llGetMass")] = std::string("float");
	llFuncs[std::string("llCollisionFilter")] = std::string("void");
	llFuncs[std::string("llTakeControls")] = std::string("void");
	llFuncs[std::string("llReleaseControls")] = std::string("void");
	llFuncs[std::string("llAttachToAvatar")] = std::string("void");
	llFuncs[std::string("llDetachFromAvatar")] = std::string("void");
	llFuncs[std::string("llTakeCamera")] = std::string("void");
	llFuncs[std::string("llReleaseCamera")] = std::string("void");
	llFuncs[std::string("llGetOwner")] = std::string("key");
	llFuncs[std::string("llInstantMessage")] = std::string("void");
	llFuncs[std::string("llEmail")] = std::string("void");
	llFuncs[std::string("llGetNextEmail")] = std::string("void");
	llFuncs[std::string("llGetKey")] = std::string("key");
	llFuncs[std::string("llSetBuoyancy")] = std::string("void");
	llFuncs[std::string("llSetHoverHeight")] = std::string("void");
	llFuncs[std::string("llStopHover")] = std::string("void");
	llFuncs[std::string("llMinEventDelay")] = std::string("void");
	llFuncs[std::string("llSoundPreload")] = std::string("void");
	llFuncs[std::string("llRotLookAt")] = std::string("void");
	llFuncs[std::string("llStringLength")] = std::string("integer");
	llFuncs[std::string("llStartAnimation")] = std::string("void");
	llFuncs[std::string("llStopAnimation")] = std::string("void");
	llFuncs[std::string("llPointAt")] = std::string("void");
	llFuncs[std::string("llStopPointAt")] = std::string("void");
	llFuncs[std::string("llTargetOmega")] = std::string("void");
	llFuncs[std::string("llGetStartParameter")] = std::string("integer");
	llFuncs[std::string("llGodLikeRezObject")] = std::string("void");
	llFuncs[std::string("llRequestPermissions")] = std::string("void");
	llFuncs[std::string("llGetPermissionsKey")] = std::string("key");
	llFuncs[std::string("llGetPermissions")] = std::string("integer");
	llFuncs[std::string("llGetLinkNumber")] = std::string("integer");
	llFuncs[std::string("llSetLinkColor")] = std::string("void");
	llFuncs[std::string("llCreateLink")] = std::string("void");
	llFuncs[std::string("llBreakLink")] = std::string("void");
	llFuncs[std::string("llBreakAllLinks")] = std::string("void");
	llFuncs[std::string("llGetLinkKey")] = std::string("key");
	llFuncs[std::string("llGetLinkName")] = std::string("string");
	llFuncs[std::string("llGetInventoryNumber")] = std::string("integer");
	llFuncs[std::string("llGetInventoryName")] = std::string("string");
	llFuncs[std::string("llSetScriptState")] = std::string("void");
	llFuncs[std::string("llGetEnergy")] = std::string("float");
	llFuncs[std::string("llGiveInventory")] = std::string("void");
	llFuncs[std::string("llRemoveInventory")] = std::string("void");
	llFuncs[std::string("llSetText")] = std::string("void");
	llFuncs[std::string("llWater")] = std::string("float");
	llFuncs[std::string("llPassTouches")] = std::string("void");
	llFuncs[std::string("llRequestAgentData")] = std::string("key");
	llFuncs[std::string("llRequestInventoryData")] = std::string("key");
	llFuncs[std::string("llSetDamage")] = std::string("void");
	llFuncs[std::string("llTeleportAgentHome")] = std::string("void");
	llFuncs[std::string("llModifyLand")] = std::string("void");
	llFuncs[std::string("llCollisionSound")] = std::string("void");
	llFuncs[std::string("llCollisionSprite")] = std::string("void");
	llFuncs[std::string("llGetAnimation")] = std::string("string");
	llFuncs[std::string("llResetScript")] = std::string("void");
	llFuncs[std::string("llMessageLinked")] = std::string("void");
	llFuncs[std::string("llPushObject")] = std::string("void");
	llFuncs[std::string("llPassCollisions")] = std::string("void");
	llFuncs[std::string("llGetScriptName")] = std::string("void");
	llFuncs[std::string("llGetNumberOfSides")] = std::string("integer");
	llFuncs[std::string("llAxisAngle2Rot")] = std::string("rotation");
	llFuncs[std::string("llRot2Axis")] = std::string("vector");
	llFuncs[std::string("llRot2Angle")] = std::string("float");
	llFuncs[std::string("llAcos")] = std::string("float");
	llFuncs[std::string("llAsin")] = std::string("float");
	llFuncs[std::string("llAngleBetween")] = std::string("float");
	llFuncs[std::string("llGetInventoryKey")] = std::string("key");
	llFuncs[std::string("llAllowInventoryDrop")] = std::string("void");
	llFuncs[std::string("llGetSunDirection")] = std::string("vector");
	llFuncs[std::string("llGetTextureOffset")] = std::string("vector");
	llFuncs[std::string("llGetTextureScale")] = std::string("vector");
	llFuncs[std::string("llGetTextureRot")] = std::string("float");
	llFuncs[std::string("llSubStringIndex")] = std::string("integer");
	llFuncs[std::string("llGetOwnerKey")] = std::string("key");
	llFuncs[std::string("llGetCenterOfMass")] = std::string("vector");
	llFuncs[std::string("llListSort")] = std::string("list");
	llFuncs[std::string("llGetListLength")] = std::string("integer");
	llFuncs[std::string("llList2Integer")] = std::string("integer");
	llFuncs[std::string("llList2Float")] = std::string("float");
	llFuncs[std::string("llList2String")] = std::string("string");
	llFuncs[std::string("llList2Key")] = std::string("key");
	llFuncs[std::string("llList2Vector")] = std::string("vector");
	llFuncs[std::string("llList2Rot")] = std::string("rotation");
	llFuncs[std::string("llList2List")] = std::string("list");
	llFuncs[std::string("llDeleteSubList")] = std::string("list");
	llFuncs[std::string("llGetListEntryType")] = std::string("integer");
	llFuncs[std::string("llList2CSV")] = std::string("string");
	llFuncs[std::string("llCSV2List")] = std::string("list");
	llFuncs[std::string("llListRandomize")] = std::string("list");
	llFuncs[std::string("llList2ListStrided")] = std::string("list");
	llFuncs[std::string("llGetRegionCorner")] = std::string("vector");
	llFuncs[std::string("llListInsertList")] = std::string("list");
	llFuncs[std::string("llListFindList")] = std::string("integer");
	llFuncs[std::string("llGetObjectName")] = std::string("string");
	llFuncs[std::string("llSetObjectName")] = std::string("void");
	llFuncs[std::string("llGetDate")] = std::string("string");
	llFuncs[std::string("llEdgeOfWorld")] = std::string("integer");
	llFuncs[std::string("llGetAgentInfo")] = std::string("integer");
	llFuncs[std::string("llAdjustSoundVolume")] = std::string("void");
	llFuncs[std::string("llSetSoundQueueing")] = std::string("void");
	llFuncs[std::string("llSetSoundRadius")] = std::string("void");
	llFuncs[std::string("llKey2Name")] = std::string("string");
	llFuncs[std::string("llSetTextureAnim")] = std::string("void");
	llFuncs[std::string("llTriggerSoundLimited")] = std::string("void");
	llFuncs[std::string("llEjectFromLand")] = std::string("void");
	llFuncs[std::string("llParseString2List")] = std::string("list");
	llFuncs[std::string("llOverMyLand")] = std::string("integer");
	llFuncs[std::string("llGetLandOwnerAt")] = std::string("key");
	llFuncs[std::string("llGetNotecardLine")] = std::string("key");
	llFuncs[std::string("llGetAgentSize")] = std::string("vector");
	llFuncs[std::string("llSameGroup")] = std::string("integer");
	llFuncs[std::string("llUnSit")] = std::string("key");
	llFuncs[std::string("llGroundSlope")] = std::string("vector");
	llFuncs[std::string("llGroundNormal")] = std::string("vector");
	llFuncs[std::string("llGroundCountour")] = std::string("vector");
	llFuncs[std::string("llGetAttached")] = std::string("integer");
	llFuncs[std::string("llGetFreeMemory")] = std::string("integer");
	llFuncs[std::string("llGetRegionName")] = std::string("string");
	llFuncs[std::string("llGetRegionTimeDilation")] = std::string("float");
	llFuncs[std::string("llGetRegionFPS")] = std::string("float");
	llFuncs[std::string("llParticleSystem")] = std::string("void");
	llFuncs[std::string("llGroundRepel")] = std::string("void");
	llFuncs[std::string("llGiveInventoryList")] = std::string("void");
	llFuncs[std::string("llSetVehicleType")] = std::string("void");
	llFuncs[std::string("llSetVehicleFloatParam")] = std::string("void");
	llFuncs[std::string("llSetVehicleVectorParam")] = std::string("void");
	llFuncs[std::string("llSetVehicleVectorParam")] = std::string("void");
	llFuncs[std::string("llSetVehicleFlags")] = std::string("void");
	llFuncs[std::string("llRemoveVehicleFlags")] = std::string("void");
	llFuncs[std::string("llSitTarget")] = std::string("void");
	llFuncs[std::string("llAvatarOnSitTarget")] = std::string("key");
	llFuncs[std::string("llAddToLandPassList")] = std::string("void");
	llFuncs[std::string("llSetTouchText")] = std::string("void");
	llFuncs[std::string("llSetSitText")] = std::string("void");
	llFuncs[std::string("llSetCameraEyeOffset")] = std::string("void");
	llFuncs[std::string("llSetCameraAtOffset")] = std::string("void");
	llFuncs[std::string("llDumpList2String")] = std::string("string");
	llFuncs[std::string("llScriptDanger")] = std::string("integer");
	llFuncs[std::string("llDialog")] = std::string("void");
	llFuncs[std::string("llVolumeDetect")] = std::string("void");
	llFuncs[std::string("llResetOtherScript")] = std::string("void");
	llFuncs[std::string("llGetScriptState")] = std::string("integer");
	llFuncs[std::string("llScriptLibraryFunction")] = std::string("void");
	llFuncs[std::string("llSetRemoteScriptAccessPin")] = std::string("void");
	llFuncs[std::string("llRemoteLoadScriptPin")] = std::string("void");
	llFuncs[std::string("llOpenRemoteDataChannel")] = std::string("void");
	llFuncs[std::string("llSendRemoteData")] = std::string("void");
	llFuncs[std::string("llRemoteDataReply")] = std::string("void");
	llFuncs[std::string("llCloseRemoteDataChannel")] = std::string("void");
	llFuncs[std::string("llMD5String")] = std::string("string");
	llFuncs[std::string("llSetPrimitiveParams")] = std::string("void");
	llFuncs[std::string("llStringToBase64")] = std::string("string");
	llFuncs[std::string("llBase64ToString")] = std::string("string");
	llFuncs[std::string("llXorBase64Strings")] = std::string("void");
	llFuncs[std::string("llRemoteDataSetRegion")] = std::string("void");
	llFuncs[std::string("llLog10")] = std::string("float");
	llFuncs[std::string("llLog")] = std::string("float");
	llFuncs[std::string("llGetAnimationList")] = std::string("list");
	llFuncs[std::string("llSetParcelMusicURL")] = std::string("void");
	llFuncs[std::string("llGetRootPosition")] = std::string("vector");
	llFuncs[std::string("llGetRootRotation")] = std::string("rotation");
	llFuncs[std::string("llGetObjectDesc")] = std::string("string");
	llFuncs[std::string("llSetObjectDesc")] = std::string("void");
	llFuncs[std::string("llGetCreator")] = std::string("key");
	llFuncs[std::string("llGetTimestamp")] = std::string("string");
	llFuncs[std::string("llSetLinkAlpha")] = std::string("void");
	llFuncs[std::string("llGetNumberOfPrims")] = std::string("integer");
	llFuncs[std::string("llGetNumberOfNotecardLines")] = std::string("key");
	llFuncs[std::string("llGetBoundingBox")] = std::string("list");
	llFuncs[std::string("llGetGeometricCenter")] = std::string("vector");
	llFuncs[std::string("llGetPrimitiveParams")] = std::string("list");
	llFuncs[std::string("llIntegerToBase64")] = std::string("void");
	llFuncs[std::string("llBase64ToInteger")] = std::string("void");
	llFuncs[std::string("llGetGMTclock")] = std::string("float");
	llFuncs[std::string("llGetSimulatorHostname")] = std::string("void");
	llFuncs[std::string("llSetLocalRot")] = std::string("void");
	llFuncs[std::string("llParseStringKeeps")] = std::string("list");
	llFuncs[std::string("llRezAtRoot")] = std::string("void");
	llFuncs[std::string("llGetObjectPermMask")] = std::string("integer");
	llFuncs[std::string("llSetObjectPermMask")] = std::string("void");
	llFuncs[std::string("llGetInventoryPermMask")] = std::string("integer");
	llFuncs[std::string("llSetInventoryPermMask")] = std::string("void");
	llFuncs[std::string("llGetInventoryCreator")] = std::string("key");
	llFuncs[std::string("llOwnerSay")] = std::string("void");
	llFuncs[std::string("llRequestSimulatorData")] = std::string("key");
	llFuncs[std::string("llForceMouselook")] = std::string("void");
	llFuncs[std::string("llGetObjectMass")] = std::string("float");
	llFuncs[std::string("llListReplaceList")] = std::string("list");
	llFuncs[std::string("lloadURL")] = std::string("void");
	llFuncs[std::string("llParcelMediaCommandList")] = std::string("void");
	llFuncs[std::string("llParcelMediaQuery")] = std::string("void");
	llFuncs[std::string("llModPow")] = std::string("integer");
	llFuncs[std::string("llGetInventoryType")] = std::string("integer");
	llFuncs[std::string("llSetPayPrice")] = std::string("void");
	llFuncs[std::string("llGetCameraPos")] = std::string("vector");
	llFuncs[std::string("llGetCameraRot")] = std::string("rotation");
	llFuncs[std::string("llSetPrimURL")] = std::string("void");
	llFuncs[std::string("llRefreshPrimURL")] = std::string("void");
	llFuncs[std::string("llEscapeURL")] = std::string("string");
	llFuncs[std::string("llUnescapeURL")] = std::string("string");
	llFuncs[std::string("llMapDestination")] = std::string("void");
	llFuncs[std::string("llAddToLandBanList")] = std::string("void");
	llFuncs[std::string("llRemoveFromLandPassList")] = std::string("void");
	llFuncs[std::string("llRemoveFromLandBanList")] = std::string("void");
	llFuncs[std::string("llSetCameraParams")] = std::string("");
	llFuncs[std::string("llClearCameraParams")] = std::string("");
	llFuncs[std::string("llListStatistics")] = std::string("float");
	llFuncs[std::string("llGetUnixTime")] = std::string("integer");
	llFuncs[std::string("llGetParcelFlags")] = std::string("integer");
	llFuncs[std::string("llGetRegionFlags")] = std::string("integer");
	llFuncs[std::string("llXorBase64StringsCorrect")] = std::string("string");
	llFuncs[std::string("llHTTPRequest")] = std::string("void");
	llFuncs[std::string("llResetLandBanList")] = std::string("void");
	llFuncs[std::string("llResetLandPassList")] = std::string("void");
	llFuncs[std::string("llGetObjectPrimCount")] = std::string("integer");
	llFuncs[std::string("llGetParcelPrimOwners")] = std::string("void");
	llFuncs[std::string("llGetParcelPrimCount")] = std::string("integer");
	llFuncs[std::string("llGetParcelMaxPrims")] = std::string("integer");
	llFuncs[std::string("llGetParcelDetails")] = std::string("list");
	llFuncs[std::string("llSetLinkPrimitiveParams")] = std::string("void");
	llFuncs[std::string("llSetLinkTexture")] = std::string("void");
	llFuncs[std::string("llStringTrim")] = std::string("string");
	llFuncs[std::string("llRegionSay")] = std::string("void");
	llFuncs[std::string("llGetObjectDetails")] = std::string("list");
	llFuncs[std::string("llSetClickAction")] = std::string("void");
	llFuncs[std::string("llGetRegionAgentCount")] = std::string("integer");
	llFuncs[std::string("llTextBox")] = std::string("void");
	llFuncs[std::string("llGetAgentLanguage")] = std::string("void");
	llFuncs[std::string("llDetectedTouchUV")] = std::string("vector");
	llFuncs[std::string("llDetectedTouchFace")] = std::string("integer");
	llFuncs[std::string("llDetectedTouchPos")] = std::string("vector");
	llFuncs[std::string("llDetectedTouchNormal")] = std::string("vector");
	llFuncs[std::string("llDetectedTouchBinormal")] = std::string("vector");
	llFuncs[std::string("llDetectedTouchST")] = std::string("vector");
	llFuncs[std::string("llSHA1String")] = std::string("string");
	llFuncs[std::string("llGetFreeURLs")] = std::string("integer");
	llFuncs[std::string("llRequestURL")] = std::string("key");
	llFuncs[std::string("llRequestSecureURL")] = std::string("key");
	llFuncs[std::string("llReleaseURL")] = std::string("void");
	llFuncs[std::string("llHTTPResponse")] = std::string("void");
	llFuncs[std::string("llGetHTTPHeader")] = std::string("string");
	llFuncs[std::string("llGetHTTPHeader")] = std::string("string");
	llFuncs[std::string("integer")] = std::string("integer");
	llFuncs[std::string("float")] = std::string("float");
	llFuncs[std::string("string")] = std::string("string");
	llFuncs[std::string("key")] = std::string("key");
	llFuncs[std::string("vector")] = std::string("vector");
	llFuncs[std::string("rotation")] = std::string("rotation");
	llFuncs[std::string("list")] = std::string("list");
}

std::string gettype(std::string& file, std::string var, int pos)
{
	static bool init_func_types = true;
	if(init_func_types)
	{
		init_func_types = false;
		init_funcs();
	}
	//in_my_scope(std::string& file, int it, int me, char left = '{', char right = '}')
	//int scope = getscope(file, pos);
//todo//(\S*?)\s*
	{
		boost::smatch matches;
		std::string::const_iterator start = var.begin();
		var += " ";
		if(boost::regex_search(start, std::string::const_iterator(var.end()), matches, boost::regex("[^a-zA-Z0-9_]*?([a-zA-Z0-9_]*?)\\s+?"), boost::match_default))
		{
			var = matches[1];
		}
	}
	if(llFuncs.find(var) != llFuncs.end())return llFuncs[var];
	else
	{
		boost::smatch matches;
		std::string::const_iterator start = file.begin();
		while(boost::regex_search(start, std::string::const_iterator(file.end()), matches, boost::regex("(integer|float|string|key|vector|rotation|list)\\s+?"+var+"\\s*?[;=]"), boost::match_default))
		{
			std::string type = matches[1];
			if(in_my_scope(file, matches.position(0), pos))
			{
				return type;
			}
			start = matches[0].second;
		}
	}
	return std::string("-1");
}*/

inline int const_iterator_to_pos(std::string::const_iterator begin, std::string::const_iterator cursor)
{
	//FAILDEBUG
	return std::distance(begin, cursor);
}


std::string JCLSLPreprocessor::lslopt(std::string script)
{
	//FAILDEBUG
	script = " \n"+script;//HACK//this should prevent regex fail for functions starting on line 0, column 0
	//added more to prevent split fail on scripts with no global data

	//this should be fun
	//FAILDEBUG
	try
	{
		boost::smatch result;
		//FAILDEBUG
		if (boost::regex_search(script, result, boost::regex("([\\S\\s]*?)(\\s*default\\s*\\{)([\\S\\s]*)")))
		{
			//FAILDEBUG
			std::string top = result[1];
			std::string bottom = result[2];
			bottom += result[3];

			boost::regex findfuncts("(integer|float|string|key|vector|rotation|list){0,1}[\\}\\s]+([a-zA-Z0-9_]+)\\(");
			//there is a minor problem with this regex, it will 
			//grab extra wnhitespace/newlines in front of functions that do not return a value
			//however this seems unimportant as it is only a couple chars and 
			//never grabs code that would actually break compilation
			//FAILDEBUG
			boost::smatch TOPfmatch;

			std::set<std::string> kept_functions;

			std::map<std::string, std::string> functions;
			//FAILDEBUG
			while(boost::regex_search(std::string::const_iterator(top.begin()), std::string::const_iterator(top.end()), TOPfmatch, findfuncts, boost::match_default))
			{
				//FAILDEBUG
				//std::string type = TOPfmatch[1];
				std::string funcname = TOPfmatch[2];

				int pos = TOPfmatch.position(boost::match_results<std::string::const_iterator>::size_type(0));
				std::string funcb = scopeript2(top, pos);
				functions[funcname] = funcb;
				//cmdline_printchat("func "+funcname+" added to list["+funcb+"]");
				top.erase(pos,funcb.size());
			}
			//FAILDEBUG
			bool repass = false;
			do
			{
				//FAILDEBUG
				repass = false;
				std::map<std::string, std::string>::iterator func_it;
				for(func_it = functions.begin(); func_it != functions.end(); func_it++)
				{
					//FAILDEBUG
					std::string funcname = func_it->first;
					
					if(kept_functions.find(funcname) == kept_functions.end())
					{
						//FAILDEBUG
						boost::smatch calls;
												//funcname has to be [a-zA-Z0-9_]+, so we know its safe
						boost::regex findcalls(std::string("[^a-zA-Z0-9_]")+funcname+std::string("\\("));
						
						std::string::const_iterator bstart = bottom.begin();
						std::string::const_iterator bend = bottom.end();

						if(boost::regex_search(bstart, bend, calls, findcalls, boost::match_default))
						{
							//FAILDEBUG
							std::string function = func_it->second;
							kept_functions.insert(funcname);
							bottom = function+"\n"+bottom;
							repass = true;
						}
					}
				}
			}while(repass);
			//FAILDEBUG
			//global var time

			std::map<std::string, std::string> gvars;
			//FAILDEBUG
			boost::regex findvars("(integer|float|string|key|vector|rotation|list)\\s+([a-zA-Z0-9_]+)([^\\(\\);]*;)");

			boost::smatch TOPvmatch;
			while(boost::regex_search(std::string::const_iterator(top.begin()), std::string::const_iterator(top.end()), TOPvmatch, findvars, boost::match_default))
			{
				//FAILDEBUG
				std::string varname = TOPvmatch[2];
				std::string fullref = TOPvmatch[1] + " " + varname+TOPvmatch[3];

				gvars[varname] = fullref;
				//cmdline_printchat("var "+varname+" added to list as ["+fullref+"]");
				int start = const_iterator_to_pos(std::string::const_iterator(top.begin()), TOPvmatch[1].first);
				top.erase(start,fullref.length());
				//cmdline_printchat("top=["+top+"]");
			}
			//FAILDEBUG
			std::map<std::string, std::string>::iterator var_it;
			for(var_it = gvars.begin(); var_it != gvars.end(); var_it++)
			{
				//FAILDEBUG
				std::string varname = var_it->first;
				boost::regex findvcalls(std::string("[^a-zA-Z0-9_]+")+varname+std::string("[^a-zA-Z0-9_]+"));
				boost::smatch vcalls;
				std::string::const_iterator bstart = bottom.begin();
				std::string::const_iterator bend = bottom.end();
				//FAILDEBUG
				if(boost::regex_search(bstart, bend, vcalls, findvcalls, boost::match_default))
				{
					//FAILDEBUG
					//boost::regex findvcalls(std::string("[^a-zA-Z0-9_]+")+varname+std::string("[^a-zA-Z0-9_]+="));
					//do we want to opt out unset global strings into local literals hrm
					bottom = var_it->second + "\n" + bottom;
				}
				//FAILDEBUG
			}
			//FAILDEBUG
			script = bottom;
		}
	}
	catch (boost::regex_error& e)
	{
		//FAILDEBUG
		std::string err = "not a valid regular expression: \"";
		err += e.what();
		err += "\"; optimization skipped";
		cmdline_printchat(err);
	}
	catch (...)
	{
		//FAILDEBUG
		cmdline_printchat("unexpected exception caught; optimization skipped");
	}
	return script;
}

struct ProcCacheInfo
{
	LLViewerInventoryItem* item;
	JCLSLPreprocessor* self;
};


struct trace_include_files : public boost::wave::context_policies::default_preprocessing_hooks
{
	trace_include_files(JCLSLPreprocessor* proc) 
    :   mProc(proc) 
    {
		//FAILDEBUG
		mAssetStack.push(LLUUID::null.asString());
	}


template <typename ContextT>
    bool found_include_directive(ContextT const& ctx, 
        std::string const &filename, bool include_next)
	{
		std::string cfilename = filename.substr(1,filename.length()-2);
		//cmdline_printchat(cfilename+":found_include_directive");
		LLUUID item_id = JCLSLPreprocessor::findInventoryByName(cfilename);
		if(item_id.notNull())
		{
			LLViewerInventoryItem* item = gInventory.getItem(item_id);
			if(item)
			{
				std::map<std::string,LLUUID>::iterator it = mProc->cached_assetids.find(cfilename);
				bool not_cached = (it == mProc->cached_assetids.end());
				bool changed = true;
				if(!not_cached)
				{
					changed = (mProc->cached_assetids[cfilename] != item->getAssetUUID());
				}
				if (not_cached || changed)
				{
					std::set<std::string>::iterator it = mProc->caching_files.find(cfilename);
					if (it == mProc->caching_files.end())
					{
						if(not_cached)mProc->display_error(std::string("Caching ")+cfilename);
						else /*if(changed)*/mProc->display_error(cfilename+std::string(" has changed, recaching..."));
						//one is always true
						mProc->caching_files.insert(cfilename);
						ProcCacheInfo* info = new ProcCacheInfo;
						info->item = item;
						info->self = mProc;
						LLPermissions perm(((LLInventoryItem*)item)->getPermissions());
						gAssetStorage->getInvItemAsset(LLHost::invalid,
						gAgent.getID(),
						gAgent.getSessionID(),
						perm.getOwner(),
						LLUUID::null,
						item->getUUID(),
						LLUUID::null,
						item->getType(),
						&JCLSLPreprocessor::JCProcCacheCallback,
						info,
						TRUE);
						return true;
					}
				}
			}
        }else
		{
			//todo check on HDD in user defined dir for file in question
		}
        //++include_depth;
		return false;
    }

template <typename ContextT>
	void opened_include_file(ContextT const& ctx, 
		std::string const &relname, std::string const& absname,
		bool is_system_include)
	{
		//FAILDEBUG
		ContextT& usefulctx = const_cast<ContextT&>(ctx);
		std::string id;
		std::string filename = boost::filesystem::path(std::string(relname)).filename();
		std::map<std::string,LLUUID>::iterator it = mProc->cached_assetids.find(filename);
		if(it != mProc->cached_assetids.end())
		{
			id = mProc->cached_assetids[filename].asString();
		}else id = "NOT_IN_WORLD";//I guess, still need to add external includes atm
		mAssetStack.push(id);
		std::string macro = "__ASSETID__";
		usefulctx.remove_macro_definition(macro, true);
		std::string def = llformat("%s=\"%s\"",macro.c_str(),id.c_str());
		usefulctx.add_macro_definition(def,false);
	}


template <typename ContextT>
	void returning_from_include_file(ContextT const& ctx)
	{
		//FAILDEBUG
		ContextT& usefulctx = const_cast<ContextT&>(ctx);
		if(mAssetStack.size() > 1)
		{
			mAssetStack.pop();
			std::string id = mAssetStack.top();
			std::string macro = "__ASSETID__";
			usefulctx.remove_macro_definition(macro, true);
			std::string def = llformat("%s=\"%s\"",macro.c_str(),id.c_str());
			usefulctx.add_macro_definition(def,false);
		}//else wave did something really fucked up
	}

    /*template <typename ContextT>
    void returning_from_include_file(ContextT const& ctx) 
    {
        --include_depth;
    }*/
    JCLSLPreprocessor* mProc;

	std::stack<std::string> mAssetStack;

	//ContextT *usefulctx;
    //std::size_t include_depth;
	//stroutputfunc lolcall;
};

//typedef void (*stroutputfunc)(std::string message);

std::string cachepath(std::string name)
{
	//FAILDEBUG
	return gDirUtilp->getExpandedFilename(LL_PATH_CACHE,"lslpreproc",name);
}

void cache_script(std::string name, std::string content)
{
	//FAILDEBUG
	content = "\n" + content + "\n";/*hack!*/
	//cmdline_printchat("writing "+name+" to cache");
	std::string path = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,"lslpreproc",name);
	LLAPRFile infile;
	infile.open(path.c_str(), LL_APR_WB);
	apr_file_t *fp = infile.getFileHandle();
	if(fp)infile.write(content.c_str(), content.length());
	infile.close();
}

void JCLSLPreprocessor::JCProcCacheCallback(LLVFS *vfs, const LLUUID& iuuid, LLAssetType::EType type, void *userdata, S32 result, LLExtStat extstat)
{
	//FAILDEBUG
	LLUUID uuid = iuuid;
	//cmdline_printchat("cachecallback called");
	ProcCacheInfo* info =(ProcCacheInfo*)userdata;
	LLViewerInventoryItem* item = info->item;
	JCLSLPreprocessor* self = info->self;
	if(item && self)
	{
		std::string name = item->getName();
		if(result == LL_ERR_NOERR)
		{
			LLVFile file(vfs, uuid, type);
			S32 file_length = file.getSize();
			char* buffer = new char[file_length+1];
			file.read((U8*)buffer, file_length);
			// put a EOS at the end
			buffer[file_length] = 0;

			std::string content(buffer);
			content = utf8str_removeCRLF(content);
			content = self->decode(content);
			/*content += llformat("\n#define __UP_ITEMID__ __ITEMID__\n#define __ITEMID__ %s\n",uuid.asString().c_str())+content;
			content += "\n#define __ITEMID__ __UP_ITEMID__\n";*/
			//prolly wont work and ill have to be not lazy, but worth a try
			delete buffer;
			if(boost::filesystem::native(name))
			{
				//cmdline_printchat("native name of "+name);
				self->mCore->mErrorList->addCommentText(std::string("Cached ")+name);
				cache_script(name, content);
				std::set<std::string>::iterator loc = self->caching_files.find(name);
				if(loc != self->caching_files.end())
				{
					//cmdline_printchat("finalizing cache");
					self->caching_files.erase(loc);
					//self->cached_files.insert(name);
					if(uuid.isNull())uuid.generate();
					item->setAssetUUID(uuid);
					self->cached_assetids[name] = uuid;//.insert(uuid.asString());
					self->start_process();
				}else
				{
					cmdline_printchat("something fucked");
				}
			}else self->mCore->mErrorList->addCommentText(std::string("Error: script named '")+name+"' isn't safe to copy to the filesystem. This include will fail.");
		}else
		{
			self->mCore->mErrorList->addCommentText(std::string("Error caching "+name));
		}
	}
	if(info)delete info;
}

class ScriptMatches : public LLInventoryCollectFunctor
{
public:
	ScriptMatches(std::string name)
	{
		sName = name;
	}
	virtual ~ScriptMatches() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item)
	{
		if(item)
		{
			//LLViewerInventoryCategory* folderp = gInventory.getCategory((item->getParentUUID());
			if(item->getName() == sName)
			{
				if(item->getType() == LLAssetType::AT_LSL_TEXT)return true;
			}
			//return (item->getName() == sName);// && cat->getName() == "#v");
		}
		return false;
	}
private:
	std::string sName;
};

LLUUID JCLSLPreprocessor::findInventoryByName(std::string name)
{
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	ScriptMatches namematches(name);
	gInventory.collectDescendentsIf(gAgent.getInventoryRootID(),cats,items,FALSE,namematches);

	if (items.count())
	{
		return items[0]->getUUID();
	}
	return LLUUID::null;
}

void JCLSLPreprocessor::preprocess_script(BOOL close, BOOL defcache)
{
	FAILDEBUG
	mClose = close;
	mDefinitionCaching = defcache;
	caching_files.clear();
	//cached_assetids.clear();
	mCore->mErrorList->addCommentText(std::string("Starting..."));
	FAILDEBUG
	LLFile::mkdir(gDirUtilp->getExpandedFilename(LL_PATH_CACHE,"")+gDirUtilp->getDirDelimiter()+"lslpreproc");
	FAILDEBUG
	std::string script = mCore->mEditor->getText();
	FAILDEBUG
	//this script is special
	LLViewerInventoryItem* item = mCore->mItem;
	FAILDEBUG
	std::string name = item->getName();
	FAILDEBUG
	//cached_files.insert(name);
	cached_assetids[name] = LLUUID::null;
	FAILDEBUG
	cache_script(name, script);
	FAILDEBUG
	//start the party
	start_process();
	//FAILDEBUG
}

const std::string lazy_list_set_func("\
list lazy_list_set(list target, integer pos, list newval)\n\
{\n\
    integer end = llGetListLength(target);\n\
    if(end > pos)\n\
    {\n\
        target = llListReplaceList(target,newval,pos,pos);\n\
    }else if(end == pos)\n\
    {\n\
        target += newval;\n\
    }else\n\
    {\n\
        do\n\
        {\n\
            target += [0];\n\
            end += 1;\n\
        }while(end < pos);\n\
        target += newval;\n\
    }\n\
    return target;\n\
}\n\
");

std::string reformat_lazy_lists(std::string script)
{
	BOOL add_set = FALSE;
	std::string nscript = script;
	nscript = boost::regex_replace(nscript, boost::regex("([a-zA-Z0-9_]+)\\[([a-zA-Z0-9_()\"]+)]\\s*=\\s*([a-zA-Z0-9_()\"\\+\\-\\*/]+)([;)])",boost::regex::perl), "$1=lazy_list_set($1,$2,[$3])$4");
	if(nscript != script)add_set = TRUE;
	
	//...

	//boost::regex_search(
	

	if(add_set == TRUE)
	{
		//add lazy_list_set function to top of script, as it is used
		nscript = utf8str_removeCRLF(lazy_list_set_func) + "\n" + nscript;
	}


	return nscript;
}


inline std::string randstr(int len, std::string chars)
{
	int clen = int(chars.length());
	int built = 0;
	std::string ret;
	while(built < len)
	{
		int r = std::rand() / ( RAND_MAX / clen );
		r = r % clen;//sanity
		ret += chars.at(r);
		built += 1;
	}
	return ret;
}

inline std::string quicklabel()
{
	return std::string("c")+randstr(5,"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
}

//#define cmdline_printchat(x) cmdline_printchat(x); llinfos << x << llendl

std::string minimalize_whitespace(std::string in)
{
	return boost::regex_replace(in, boost::regex("\\s*",boost::regex::perl), "\n");		
}

std::string reformat_switch_statements(std::string script)
{
	//"switch\\(([^\\)]*)\\)\\s{"

	std::string buffer = script;
	{
		try
		{
			boost::regex findswitches("\\sswitch\\(");//nasty

			boost::smatch matches;

			static std::string switchstr = "switch(";

			int escape = 100;

			while(boost::regex_search(std::string::const_iterator(buffer.begin()), std::string::const_iterator(buffer.end()), matches, findswitches, boost::match_default) && escape > 1)
			{
				int res = matches.position(boost::match_results<std::string::const_iterator>::size_type(0))+1;
				
				static int slen = switchstr.length();

				std::string arg = scopeript2(buffer, res+slen-1,'(',')');

				//arg *will have* () around it
				if(arg == "begin out of bounds" || arg == "end out of bounds")
				{
					cmdline_printchat(arg);
					break;
				}
				//cmdline_printchat("arg=["+arg+"]");
				std::string rstate = scopeript2(buffer, res+slen+arg.length()-1);

				int cutlen = slen;
				cutlen -= 1;
				cutlen += arg.length();
				cutlen += rstate.length();
				//slen is for switch( and arg has () so we need to - 1 ( to get the right length
				//then add arg len and state len to get section to excise

				//rip off the scope edges
				int slicestart = rstate.find("{")+1;
				rstate = rstate.substr(slicestart,(rstate.rfind("}")-slicestart)-1);
				//cmdline_printchat("rstate=["+rstate+"]");



				boost::regex findcases("\\scase\\s");

				boost::smatch statematches;

				std::map<std::string,std::string> ifs;

				while(boost::regex_search(std::string::const_iterator(rstate.begin()), std::string::const_iterator(rstate.end()), statematches, findcases, boost::match_default) && escape > 1)
				{
					//if(statematches[0].matched)
					{
						int case_start = statematches.position(boost::match_results<std::string::const_iterator>::size_type(0))+1;//const_iterator2pos(statematches[0].first+1, std::string::const_iterator(rstate.begin()))-1;
						int next_curl = rstate.find("{",case_start+1);
						int next_semi = rstate.find(":",case_start+1);
						int case_end = (next_curl < next_semi && next_curl != -1) ? next_curl : next_semi;
						static int caselen = std::string("case").length();
						if(case_end != -1)
						{
							std::string casearg = rstate.substr(case_start+caselen,case_end-(case_start+caselen));
							//cmdline_printchat("casearg=["+casearg+"]");
							std::string label = quicklabel();
							ifs[casearg] = label;
							//cmdline_printchat("BEFORE["+rstate+"]");
							bool addcurl = (case_end == next_curl ? 1 : 0);
							label = "@"+label+";\n";
							if(addcurl)label += "{";
							rstate.erase(case_start,(case_end-case_start) + 1);
							rstate.insert(case_start,label);
							//cmdline_printchat("AFTER["+rstate+"]");
						}else
						{
							cmdline_printchat("error in regex case_end != -1");
							rstate.erase(case_start,caselen);
							rstate.insert(case_start,"error; cannot find { or :");
						}
					}
					escape -= 1;
				}

				std::string deflt = quicklabel();
				bool isdflt = false;
				std::string defstate;
				defstate = boost::regex_replace(rstate, boost::regex("(\\s)(default\\s*?):",boost::regex::perl), "$1\\@"+deflt+";");
				defstate = boost::regex_replace(defstate, boost::regex("(\\s)(default\\s*?)\\{",boost::regex::perl), "$1\\@"+deflt+"; \\{");
				if(defstate != rstate)
				{
					isdflt = true;
					rstate = defstate;
				}
				std::string argl;
				std::string jumptable = "{";
				/*std::string type = gettype(buffer, arg, res);
				if(type != "void" && type != "-1")
				{
					std::string argl = quicklabel();
					jumptable += type+" "+argl+" = "+arg+";\n";
					arg = argl;
				}else
				{
					cmdline_printchat("type="+type);
				}*/

				std::map<std::string, std::string>::iterator ifs_it;
				for(ifs_it = ifs.begin(); ifs_it != ifs.end(); ifs_it++)
				{
					jumptable += "if("+arg+" == ("+ifs_it->first+"))jump "+ifs_it->second+";\n";
				}
				if(isdflt)jumptable += "jump "+deflt+";\n";

				rstate = jumptable + rstate + "\n";
			
				std::string brk = quicklabel();
				defstate = boost::regex_replace(rstate, boost::regex("(\\s)break\\s*;",boost::regex::perl), "$1jump "+brk+";");
				if(defstate != rstate)
				{
					rstate = defstate;
					rstate += "\n@"+brk+";\n";
				}
				rstate = rstate + "}";

				//cmdline_printchat("replacing["+buffer.substr(res,cutlen)+"] with ["+rstate+"]");
				buffer.erase(res,cutlen);
				buffer.insert(res,rstate);

				//start = buffer.begin();
				//end = buffer.end();

				escape -= 1;

				//res = buffer.find(switchstr);
			}


			/*std::string::const_iterator start = buffer.begin();
			while(boost::regex_search(start, std::string::const_iterator(buffer.end()), matches, findswitches, boost::match_default))
			{
				std::string argument = matches[1];

				int pos = const_iterator2pos(matches[0].first, start)-1;
				std::string statement = scopeript2(buffer, pos);

				std::string newstatement = "{\n";
				newstatement += "integer switch_res = ("+argument+");\n";
				std::map<std::string case_,std::string label> ifs;

				//functions[funcname] = funcb;
				cmdline_printchat("detected switch ["+statement+"]");
				buffer.erase(pos,statement.size());
			}*/
			script = buffer;
		}
		catch (boost::regex_error& e)
		{
			std::string err = "not a valid regular expression: \"";
			err += e.what();
			err += "\"; switch statements skipped";
			cmdline_printchat(err);
		}
		catch (...)
		{
			cmdline_printchat("unexpected exception caught; buffer=["+buffer+"]");
		}
	}


	return script;
}
/*
std::string convert_continue_break_statements(std::string script)
{
	std::string buffer = script;
	try
	{
		static boost::regex find_breaks("\\s(break)\\s*?;");
		static boost::regex find_continues("\\s(continue)\\s*?;");

		static boost::regex reverse_find_while("\\{[^\\{\\};]*?\\(\\s*?(elihw)\\s");
		static boost::regex reverse_find_do("\\{\\s*?(od)\\s");
		static boost::regex reverse_find_for("\\{\\s*?(rof)\\s");

		boost::smatch matches;



		int escape = 100;

		while(boost::regex_search(std::string::const_iterator(buffer.begin()), std::string::const_iterator(buffer.end()), matches, find_breaks, boost::match_default) && escape > 1)
		{
			int mpos = matches.position(boost::match_results<std::string::const_iterator>::size_type(1));

			boost::smatch r_matches;

			int last_for = -1;
			int last_do = -1;
			int last_while = -1;


			std::string slice = buffer.substr(0,mpos-1);
			if(boost::regex_search(std::string::const_reverse_iterator(slice.rbegin()), std::string::const_reverse_iterator(slice.rend()), r_matches, reverse_find_while, boost::match_default))
			{
				int rpos = r_matches.position(boost::match_results<std::string::const_reverse_iterator>::size_type(1));
				//reversed pos?
				rpos = (slice.length()-1) - rpos;
				//fixed

				last_while = rpos;
			}
			if(boost::regex_search(std::string::const_reverse_iterator(slice.rbegin()), std::string::const_reverse_iterator(slice.rend()), r_matches, reverse_find_do, boost::match_default))
			{
				int rpos = r_matches.position(boost::match_results<std::string::const_reverse_iterator>::size_type(1));
				//reversed pos?
				rpos = (slice.length()-1) - rpos;
				//fixed

				last_do = rpos;
			}
			if(boost::regex_search(std::string::const_reverse_iterator(slice.rbegin()), std::string::const_reverse_iterator(slice.rend()), r_matches, reverse_find_for, boost::match_default))
			{
				int rpos = r_matches.position(boost::match_results<std::string::const_reverse_iterator>::size_type(1));
				//reversed pos?
				rpos = (slice.length()-1) - rpos;
				//fixed

				last_for = rpos;
			}
			int lop = last_while;
			if(last_do > lop)lop = last_do;
			if(last_for > lop)lop = last_for;
			if(lop < mpos)
			{
				std::string scope = scopeript2(buffer, lop);
				int blen = scope.length();
				
			}else
			{
				cmdline_printchat("couldnt find a loop");
				escape = 0;
				//throw NULL;
			}

			escape -= 1;
		}
	}
	catch (boost::regex_error& e)
	{
		std::string err = "not a valid regular expression: \"";
		err += e.what();
		err += "\"; break/continue statements skipped";
		cmdline_printchat(err);
	}
	catch (...)
	{
		cmdline_printchat("unexpected exception caught; buffer=["+buffer+"]");
	}
			
}*/


void JCLSLPreprocessor::display_error(std::string err)
{
	//FAILDEBUG
	mCore->mErrorList->addCommentText(err);
}


void JCLSLPreprocessor::start_process()
{
	FAILDEBUG
	if(waving)
	{
		//FAILDEBUG
		cmdline_printchat("already waving?");
		return;
	}
	waving = TRUE;
	//cmdline_printchat("entering waving");
	//mCore->mErrorList->addCommentText(std::string("Completed caching."));

	boost::wave::util::file_position_type current_position;

	//static const std::string predefined_stuff('
	FAILDEBUG
	std::string input = mCore->mEditor->getText();
	std::string rinput = input;
	input = "\n"+input+"\n";
	std::string output;
	std::string name = mCore->mItem->getName();
	//BOOL use_switches;
	BOOL lazy_lists = gSavedSettings.getBOOL("EmeraldLSLLazyLists");
	BOOL use_switch = gSavedSettings.getBOOL("EmeraldLSLSwitch");

	BOOL errored = FALSE;
	std::string err;
	try
	{
		FAILDEBUG
		trace_include_files tracer(this);

		//  This token type is one of the central types used throughout the library, 
		//  because it is a template parameter to some of the public classes and  
		//  instances of this type are returned from the iterators.
		typedef boost::wave::cpplexer::lex_token<> token_type;
	    
		//  The template boost::wave::cpplexer::lex_iterator<> is the lexer type to
		//  to use as the token source for the preprocessing engine. It is 
		//  parametrized with the token type.
		typedef boost::wave::cpplexer::lex_iterator<token_type> lex_iterator_type;
	        
		//  This is the resulting context type to use. The first template parameter
		//  should match the iterator type to be used during construction of the
		//  corresponding context object (see below).
		typedef boost::wave::context<std::string::iterator, lex_iterator_type, boost::wave::iteration_context_policies::load_file_to_string, trace_include_files >
				context_type;
		FAILDEBUG
		context_type ctx(input.begin(), input.end(), name.c_str(), tracer);
		//tracer.usefulctx = &ctx;
		FAILDEBUG
		ctx.set_language(boost::wave::enable_long_long(ctx.get_language()));
		//ctx.set_language(boost::wave::enable_preserve_comments(ctx.get_language()));
		ctx.set_language(boost::wave::enable_prefer_pp_numbers(ctx.get_language()));
		ctx.set_language(boost::wave::enable_variadics(ctx.get_language()));
		FAILDEBUG
		std::string path = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,"")+gDirUtilp->getDirDelimiter()+"lslpreproc"+gDirUtilp->getDirDelimiter();
		ctx.add_include_path(path.c_str());
		if(gSavedSettings.getBOOL("EmeraldEnableHDDInclude"))
		{
			FAILDEBUG
			std::string hddpath = gSavedSettings.getString("EmeraldHDDIncludeLocation");
			if(hddpath != "")
			{
				FAILDEBUG
				ctx.add_include_path(hddpath.c_str());
				ctx.add_sysinclude_path(hddpath.c_str());
				//allow "file" and <file>
			}
		}
		FAILDEBUG
		std::string def = llformat("__AGENTKEY__=\"%s\"",gAgent.getID().asString().c_str());//legacy because I used it earlier
		ctx.add_macro_definition(def,false);
		def = llformat("__AGENTID__=\"%s\"",gAgent.getID().asString().c_str());
		ctx.add_macro_definition(def,false);
		def = llformat("__AGENTIDRAW__=%s",gAgent.getID().asString().c_str());
		ctx.add_macro_definition(def,false);
		FAILDEBUG
		std::string aname;
		gAgent.getName(aname);
		def = llformat("__AGENTNAME__=\"%s\"",aname.c_str());
		ctx.add_macro_definition(def,false);
		def = llformat("__ASSETID__=%s",LLUUID::null.asString().c_str());
		ctx.add_macro_definition(def,false);
		//  Get the preprocessor iterators and use them to generate 
		//  the token sequence.
		context_type::iterator_type first = ctx.begin();
		context_type::iterator_type last = ctx.end();
	        
		FAILDEBUG
		//  The input stream is preprocessed for you while iterating over the range
		//  [first, last)
        while (first != last)
		{
			//FAILDEBUG
			if(caching_files.size() != 0)
			{
				//FAILDEBUG
				//cmdline_printchat("caching somethin, exiting waving");
				//mCore->mErrorList->addCommentText("Caching something...");
				waving = FALSE;
				//first = last;
				return;
			}
			//FAILDEBUG
            current_position = (*first).get_position();
			//FAILDEBUG
			std::string token = std::string((*first).get_value().c_str());//stupid boost bitching even though we know its a std::string
			//FAILDEBUG
			if(token == "#line")token = "//#line";
			//FAILDEBUG
			//line directives are emitted in case the frontend of the future can find use for them
			output += token;
			//FAILDEBUG
			if(lazy_lists == FALSE)lazy_lists = ctx.is_defined_macro(std::string("USE_LAZY_LISTS"));
			//FAILDEBUG
			if(use_switch == FALSE)use_switch = ctx.is_defined_macro(std::string("USE_SWITCHES"));
			/*this needs to be checked for because if it is *ever* enabled it indicates that the transform is a dependency*/
            ++first;
        }
	}
	catch(boost::wave::cpp_exception const& e)
	{
		FAILDEBUG
		errored = TRUE;
		// some preprocessing error
		err = name + "(" + llformat("%d",e.line_no()) + "): " + e.description();
		cmdline_printchat(err);
		mCore->mErrorList->addCommentText(err);
	}
	catch(std::exception const& e)
	{
		FAILDEBUG
		errored = TRUE;
		err = std::string(current_position.get_file().c_str()) + "(" + llformat("%d",current_position.get_line()) + "): ";
		err += std::string("exception caught: ") + e.what();
		cmdline_printchat(err);
		mCore->mErrorList->addCommentText(err);
	}
	catch (...)
	{
		FAILDEBUG
		errored = TRUE;
		err = std::string(current_position.get_file().c_str()) + llformat("%d",current_position.get_line());
		err += std::string("): unexpected exception caught.");
		cmdline_printchat(err);
		mCore->mErrorList->addCommentText(err);
	}

	if(!errored)
	{
		FAILDEBUG
		if(lazy_lists == TRUE)
		{
			try
			{
				mCore->mErrorList->addCommentText("Applying lazy list set transform");
				output = reformat_lazy_lists(output);
			}catch(...)
			{	
				errored = TRUE;
				err = "unexpected exception in lazy list converter.";
				mCore->mErrorList->addCommentText(err);
			}

		}
		if(use_switch == TRUE)
		{
			try
			{
				mCore->mErrorList->addCommentText("Applying switch statement transform");
				output = reformat_switch_statements(output);
			}catch(...)
			{	
				errored = TRUE;
				err = "unexpected exception in switch statement converter.";
				mCore->mErrorList->addCommentText(err);
			}
		}
	}

	if(!mDefinitionCaching)
	{
		if(!errored)
		{
			if(gSavedSettings.getBOOL("EmeraldLSLOptimizer"))
			{
				mCore->mErrorList->addCommentText("Optimizing out unreferenced user-defined functions and global variables");
				try
				{
					output = lslopt(output);
				}catch(...)
				{	
					errored = TRUE;
					err = "unexpected exception in lsl optimizer";
					mCore->mErrorList->addCommentText(err);
				}
			}
		}
		//output = implement_switches(output);
		/*if(errored)
		{
			output += "\nPreprocessor exception:\n"+err;
			mCore->mErrorList->addCommentText("!! Preprocessor exception:");
			mCore->mErrorList->addCommentText(err);
		}*/
		output = encode(rinput)+"\n\n"+output;


		LLTextEditor* outfield = mCore->mPostEditor;//getChild<LLViewerTextEditor>("post_process");
		if(outfield)
		{
			outfield->setText(LLStringExplicit(output));
		}
		mCore->doSaveComplete((void*)mCore,mClose);
	}else
	{
		
	}
	waving = FALSE;
}

/*void JCLSLPreprocessor::getfuncmatches(std::string token)
{//not sure if this is needed...
	if(!LSLHunspell)
	{
		std::string dicaffpath(gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "dictionaries", std::string(newDict+".aff")).c_str());
		LSLHunspell = new Hunspell(
	}
}*/