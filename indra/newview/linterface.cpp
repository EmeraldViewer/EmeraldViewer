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

#include "linterface.h"
#include "floaterao.h"
#include "jc_lslviewerbridge.h"
#include "llviewerwindow.h"


bool linterface(std::string input)
{
	std::istringstream i(input);
	std::string command;
	i >> command;
	if(command != "")
	{
		if(command == "@emao")
		{
			std::string status;
			if(i >> status)
			{
				if (status == "on" )
				{
					gSavedSettings.setBOOL("EmeraldAOEnabled",TRUE);
					LLFloaterAO::run();
				}
				else if (status == "off" )
				{
					gSavedSettings.setBOOL("EmeraldAOEnabled",FALSE);
					LLFloaterAO::run();
				}
				else if (status == "state")
				{
					int chan;
					if(i >> chan)
					{
						std::string tmp="off";
						if(gSavedSettings.getBOOL("EmeraldAOEnabled"))tmp="on";
						JCLSLBridge::send_chat_to_object(tmp,chan,LLUUID(NULL));
					}
				}
			}
			return true;
		}
		else if(command == "@emdd")
		{
			int chan;
			if(i >> chan)
			{
				std::string tmp = llformat("%f",gSavedSettings.getF32("RenderFarClip"));
				JCLSLBridge::send_chat_to_object(tmp,chan,LLUUID(NULL));
			}
			return true;
		}
		else if(command == "#emxy")
		{
			int chan;
			if(i >> chan)
			{
				GLuint resX = gViewerWindow->getWindowDisplayWidth();
				GLuint resY = gViewerWindow->getWindowDisplayHeight();
				std::string tmp = llformat("%dx%d",resX,resY);
				JCLSLBridge::send_chat_to_object(tmp,chan,LLUUID(NULL));
			}
			return true;
		}
	}
	return false;
}
