/* Copyright (c) 2010
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

#include "a_modularsystemslink.h"

#include "llbufferstream.h"

#include "llappviewer.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include "llsdserialize.h"

#include "llversionviewer.h"

#include "llagent.h"
#include "llnotifications.h"
#include "llimview.h"
#include "llfloaterabout.h"

ModularSystemsLink* ModularSystemsLink::sInstance;

ModularSystemsLink::ModularSystemsLink()
{
	sInstance = this;
}

ModularSystemsLink::~ModularSystemsLink()
{
	sInstance = NULL;
}

ModularSystemsLink* ModularSystemsLink::getInstance()
{
	if(sInstance)return sInstance;
	else
	{
		sInstance = new ModularSystemsLink();
		return sInstance;
	}
}

static const std::string versionid = llformat(" %s %d.%d.%d (%d)", LL_CHANNEL, LL_VERSION_MAJOR, LL_VERSION_MINOR, LL_VERSION_PATCH, LL_VERSION_BUILD);

//void cmdline_printchat(std::string message);

void ModularSystemsLink::start_download()
{
	//cmdline_printchat("requesting msdata");
	std::string url = "http://modularsystems.sl/app/msdata/";
	LLSD headers;
	headers.insert("Accept", "*/*");
	headers.insert("User-Agent", LLAppViewer::instance()->getWindowTitle());
	headers.insert("viewer-version", versionid);

	LLHTTPClient::get(url,new ModularSystemsDownloader( ModularSystemsLink::msdata ),headers);
}

void ModularSystemsLink::msdata(U32 status, std::string body)
{
	ModularSystemsLink* self = getInstance();
	//cmdline_printchat("msdata downloaded");

	LLSD data;
	std::istringstream istr(body);
	LLSDSerialize::fromXML(data, istr);
	if(data.isDefined())
	{
		LLSD& support_agents = data["agents"];

		self->personnel.clear();
		for(LLSD::map_iterator itr = support_agents.beginMap(); itr != support_agents.endMap(); ++itr)
		{
			std::string key = (*itr).first;
			LLSD& content = (*itr).second;
			U8 val = 0;
			if(content.has("support"))val = val | EM_SUPPORT;
			if(content.has("developer"))val = val | EM_DEVELOPER;
			self->personnel[LLUUID(key)] = val;
		}

		LLSD& blocked = data["blocked"];

		self->blocked_versions.clear();
		for (LLSD::array_iterator itr = blocked.beginArray(); itr != blocked.endArray(); ++itr)
		{
			std::string vers = (*itr).asString();
			self->blocked_versions.insert(vers);
		}

		if(data.has("MOTD"))
		{
			self->ms_motd = data["MOTD"].asString();
			gAgent.mMOTD = self->ms_motd;
		}else
		{
			self->ms_motd = "";
		}
	}

	//LLSD& dev_agents = data["dev_agents"];
	//LLSD& client_ids = data["client_ids"];
}

BOOL ModularSystemsLink::is_support(LLUUID id)
{
	ModularSystemsLink* self = getInstance();
	if(self->personnel.find(id) != self->personnel.end())
	{
		return ((self->personnel[id] & EM_SUPPORT) != 0) ? TRUE : FALSE;
	}
	return FALSE;
}

BOOL ModularSystemsLink::is_developer(LLUUID id)
{
	ModularSystemsLink* self = getInstance();
	if(self->personnel.find(id) != self->personnel.end())
	{
		return ((self->personnel[id] & EM_DEVELOPER) != 0) ? TRUE : FALSE;
	}
	return FALSE;
}

BOOL ModularSystemsLink::allowed_login()
{
	ModularSystemsLink* self = getInstance();
	return (self->blocked_versions.find(versionid) == self->blocked_versions.end());
}


std::string ModularSystemsLink::processRequestForInfo(LLUUID requester, std::string message, std::string name)
{
	std::string detectstring = "/sysinfo";
	if(!message.find("/sysinfo")==0)
	{
		//llinfos << "sysinfo was not found in this message, it was at " << message.find("/sysinfo") << " pos." << llendl;
		return message;
	}
	//llinfos << "sysinfo was found in this message, it was at " << message.find("/sysinfo") << " pos." << llendl;
	std::string outmessage("I am requesting information about your system setup.");
	std::string reason("");
	if(message.length()>detectstring.length())
	{
		reason = std::string(message.substr(detectstring.length()));
		//there is more to it!
		outmessage = std::string("I am requesting information about your system setup for this reason : "+reason);
		reason = "The reason provided was : "+reason;
	}
	LLSD args;
	args["REASON"] =reason;
	args["NAME"] = name;
	args["FROMUUID"]=requester;
	LLNotifications::instance().add("EmeraldReqInfo",args,LLSD(), callbackEmeraldReqInfo);

	return outmessage;
}
void ModularSystemsLink::callbackEmeraldReqInfo(const LLSD &notification, const LLSD &response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	std::string my_name;
	LLSD subs = LLNotification(notification).getSubstitutions();
	LLUUID uid = subs["FROMUUID"].asUUID();
	gAgent.buildFullname(my_name);
	if ( option == 0 )//yes
	{
		std::string myInfo1 = getMyInfo(1);
		std::string myInfo2 = getMyInfo(2);		
		
		pack_instant_message(
			gMessageSystem,
			gAgent.getID(),
			FALSE,
			gAgent.getSessionID(),
			uid,
			my_name,
			myInfo1);
		gAgent.sendReliableMessage();
		pack_instant_message(
			gMessageSystem,
			gAgent.getID(),
			FALSE,
			gAgent.getSessionID(),
			uid,
			my_name,
			myInfo2);
		gAgent.sendReliableMessage();
		gIMMgr->addMessage(gIMMgr->computeSessionID(IM_NOTHING_SPECIAL,uid),uid,my_name,"Information Sent: "+
			myInfo1+"\n"+myInfo2);
	}
	else
	{

	
		pack_instant_message(
			gMessageSystem,
			gAgent.getID(),
			FALSE,
			gAgent.getSessionID(),
			uid,
			my_name,
			"Request Denied.");
		gAgent.sendReliableMessage();
		gIMMgr->addMessage(gIMMgr->computeSessionID(IM_NOTHING_SPECIAL,uid),uid,my_name,"Request Denied");
	}
}
//part , 0 for all, 1 for 1st half, 2 for 2nd
std::string ModularSystemsLink::getMyInfo(int part)
{
	std::string info("");
	if(part!=2)
	{

		info.append(LLFloaterAbout::get_viewer_version());
		info.append("\n");
		info.append(LLFloaterAbout::get_viewer_build_version());
		info.append("\n");
		info.append(LLFloaterAbout::get_viewer_region_info("I am "));
		info.append("\n");
		if(part==1)return info;
	}
	info.append(LLFloaterAbout::get_viewer_misc_info());
	return info;
}

ModularSystemsDownloader::ModularSystemsDownloader(void (*callback)(U32, std::string)) : mCallback(callback) {}
ModularSystemsDownloader::~ModularSystemsDownloader(){}
void ModularSystemsDownloader::error(U32 status, const std::string& reason){}
void ModularSystemsDownloader::completedRaw(
			U32 status,
			const std::string& reason,
			const LLChannelDescriptors& channels,
			const LLIOPipe::buffer_ptr_t& buffer)
{
	LLBufferStream istr(channels, buffer.get());
	std::stringstream strstrm;
	strstrm << istr.rdbuf();
	std::string result = std::string(strstrm.str());
	mCallback(status, result);
}


