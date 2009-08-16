/* Copyright (c) 2009
 *
 * LordGregGreg Back. All rights reserved.
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
 *   3. Neither the name Modular Systems Ltd nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MODULAR SYSTEMS LTD AND CONTRIBUTORS “AS IS”
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

//#include "lggIrcThread.h"
////////////////////////////////////////////////////
//////////////DATA TYPE/////////////////////////////

#include "llviewerprecompiledheaders.h"
#include "lggIrcData.h"

lggIrcData lggIrcData::fromLLSD(LLSD inputData)
{
	llinfos << "lggIrcData::fromLLSD\n" << "\n"	<< inputData["ircserver"].asString() << "\n"
	<< inputData["ircname"].asString() << "\n"
	<< inputData["ircport"].asString() << "\n"
	<< inputData["ircnick"].asString() << "\n"
	<< inputData["ircchannel"].asString() << "\n"
	<< inputData["ircpassword"].asString() << "\n"
	<< LLUUID(inputData["ircid"].asString()) << llendl;
	return lggIrcData(
	inputData["ircserver"].asString(),
	inputData["ircname"].asString(),
	inputData["ircport"].asString(),
	inputData["ircnick"].asString(),
	inputData["ircchannel"].asString(),
	inputData["ircpassword"].asString(),
	LLUUID(inputData["ircid"].asString()));

}
LLSD lggIrcData::toLLSD()
{
	LLSD out;
	out["ircchannel"]=channel;
	out["ircport"]=port;
	out["ircid"]=id.asString();
	out["ircnick"]=nick;
	out["ircpassword"]=password;
	out["ircserver"]=server;
	out["ircname"]=name;
	return out;
}

std::string lggIrcData::toString()
{
	
	return llformat("Name is %s\nNick is %s\nChannel is %s\nUUID is %s\nServer is %s",
		name.c_str(),nick.c_str(),channel.c_str(),id.asString().c_str(),server.c_str());
}
void lggIrcData::become(lggIrcData input)
{
	name = input.name;
	server = input.server;
	port= input.port;
	nick=input.nick;
	channel=input.channel;
	password=input.password;
	id=input.id;
}
lggIrcData::lggIrcData(std::string iserver, std::string iname, std::string iport, std::string inick, std::string ichannel, std::string ipassword, LLUUID iid):
server(iserver),name(iname),port(iport),nick(inick),channel(ichannel),password(ipassword),id(iid)
{
}
lggIrcData::lggIrcData():
server("modularsystems.sl"),name("Emerald Chat"),port("8888"),
nick(std::string("Emerald-User"+LLUUID::generateNewID().asString().substr(28))),
channel("#emerald"),password(""),id(LLUUID::generateNewID())
{
}
lggIrcData::~lggIrcData()
{
	
}