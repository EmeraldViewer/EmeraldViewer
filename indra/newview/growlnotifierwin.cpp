/* Copyright (c) 2009
*
* Greg Hendrickson (LordGregGreg Back). All rights reserved.
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
* THIS SOFTWARE IS PROVIDED BY MODULAR SYSTEMS AND CONTRIBUTORS "AS IS"
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
#include "growlnotifierwin.h"
#include "llviewercontrol.h"

GrowlNotifierWin::GrowlNotifierWin():applicationName("")
{
	LL_INFOS("GrowlNotifierWin") << "Windows growl notifications initialised." << LL_ENDL;
	
}
void GrowlNotifierWin::registerAplication(const std::string& application, const std::string& csvtypes)
{
	applicationName=application;
	std::string theCMD(
		"cmd.exe /c START \"External Editor\" \""
		+ gSavedSettings.getString("EmeraldWindowsGrowlPath") + "\" " 
		+ "/t:\""+application +"\" "
		+ "/a:\""+application+"\" "
		+ "/s:"+"false"+" "
		+ "/r:"+csvtypes+" "//these need to be all the types (or names) in growl_notifications.xml
		+ "\""+application +" is ready to use Growl.\"");
	llinfos << "Issuing growl exe command of :"<<
		theCMD.c_str() << llendl;

	std::system(theCMD.c_str());
}
void GrowlNotifierWin::showNotification(const std::string& notification_title, const std::string& notification_message, 
										 const std::string& notification_type)
{
	std::string theCMD(
		"cmd.exe /c START \"External Editor\" \"" 
		+ gSavedSettings.getString("EmeraldWindowsGrowlPath") + "\" " 
		+ "/t:\""+notification_title +"\" "
		+ "/a:\""+applicationName+"\" "
		+ "/s:"+"false"+" "//sticky?
		+ "/id:\""+""+"\" "//i dont know what the id does
		//+ "/icon:\""+iconPath+"\" "  if we want a icon o.o
		+ "/n:\""+notification_type+"\" "
		+ "\""+notification_message+"\"");

	llinfos << "Issuing growl exe command of :"<<
		theCMD.c_str() << llendl;

	std::system(theCMD.c_str());
}

bool GrowlNotifierWin::isUsable()
{
	return false;
	//return gDirUtilp->fileExists(gSavedSettings.getString("EmeraldWindowsGrowlPath"));
}
