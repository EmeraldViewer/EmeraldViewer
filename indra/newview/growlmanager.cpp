/* Copyright (c) 2010 Modular Systems All rights reserved.
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

#include "lldir.h"
#include "llfile.h"
#include "llnotify.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "llstartup.h"
#include "llviewercontrol.h"

#include "growlmanager.h"
#include "growlnotifier.h"

// Platform-specific includes
#ifdef LL_DARWIN
#include "growlnotifiermacosx.h"
#elif LL_WINDOWS
#include "growlnotifierwin.h"
#endif


GrowlManager *gGrowlManager = NULL;

GrowlManager::GrowlManager()
{
	// Create a notifier appropriate to the platform.
#ifdef LL_DARWIN
	this->mNotifier = new GrowlNotifierMacOSX();
	LL_INFOS("GrowlManagerInit") << "Created GrowlNotifierMacOSX." << LL_ENDL;
#elif LL_WINDOWS
	this->mNotifier = new GrowlNotifierWin();
	LL_INFOS("GrowlManagerInit") << "Created GrowlNotifierWin." << LL_ENDL;
#else
	this->mNotifier = new GrowlNotifier();
	LL_INFOS("GrowlManagerInit") << "Created generic GrowlNotifier." << LL_ENDL;
#endif
	
	// Don't do anything more if Growl isn't usable.
	if(!mNotifier->isUsable())
	{
		LL_WARNS("GrowlManagerInit") << "Growl is unusable; bailing out." << LL_ENDL;
		return;
	}

	// Hook into LLNotifications...
	LLNotificationChannel::buildChannel("GrowlNotifyTips", "Visible", LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, "notifytip"));
	LLNotificationChannel::buildChannel("GrowlNotify", "Visible", LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, "notify"));
	
	LLNotifications::instance().getChannel("GrowlNotifyTips")->connectChanged(&GrowlManager::onLLNotification);
	LLNotifications::instance().getChannel("GrowlNotify")->connectChanged(&GrowlManager::onLLNotification);
	this->loadConfig();
}

void GrowlManager::loadConfig()
{
	std::string config_file = gDirUtilp->getExpandedFilename(LL_PATH_APP_SETTINGS, "growl_notifications.xml");
	if(config_file == "")
	{
		LL_WARNS("GrowlConfig") << "Couldn't find growl_notifications.xml" << LL_ENDL;
		return;
	}
	LL_INFOS("GrowlConfig") << "Loading growl notification config from " << config_file << LL_ENDL;
	llifstream configs(config_file);
	LLSD notificationLLSD;
	std::set<std::string> notificationTypes;
	notificationTypes.insert("Keyword Alert");
	notificationTypes.insert("Instant Message received");
	if(configs.is_open())
	{
		LLSDSerialize::fromXML(notificationLLSD, configs);
		for(LLSD::map_iterator itr = notificationLLSD.beginMap(); itr != notificationLLSD.endMap(); ++itr)
		{
			GrowlNotification ntype;
			ntype.growlName = itr->second.get("GrowlName").asString();
			notificationTypes.insert(ntype.growlName);
			
			if(itr->second.has("GrowlTitle"))
				ntype.growlTitle = itr->second.get("GrowlTitle").asString();			
			if(itr->second.has("GrowlBody"))
				ntype.growlBody = itr->second.get("GrowlBody").asString();
			if(itr->second.has("UseDefaultTextForTitle"))
				ntype.useDefaultTextForTitle = itr->second.get("UseDefaultTextForTitle").asBoolean();
			else
				ntype.useDefaultTextForTitle = false;
			if(itr->second.has("UseDefaultTextForBody"))
				ntype.useDefaultTextForBody = itr->second.get("UseDefaultTextForBody").asBoolean();
			else
				ntype.useDefaultTextForBody = false;
			if(ntype.useDefaultTextForBody == false && ntype.useDefaultTextForTitle == false && 
			   ntype.growlBody == "" && ntype.growlTitle == "")
			{
				ntype.useDefaultTextForBody = true;
			}
			this->mNotifications[itr->first] = ntype;
		}
		configs.close();

		this->mNotifier->registerApplication("Emerald Viewer", notificationTypes);
	}
	else
	{
		LL_WARNS("GrowlConfig") << "Couldn't open growl config file." << LL_ENDL;
	}

}

void GrowlManager::notify(const std::string& notification_title, const std::string& notification_message, const std::string& notification_type)
{
	if(gSavedSettings.getBOOL("EmeraldEnableGrowl"))
		this->mNotifier->showNotification(notification_title, notification_message, notification_type);
}

bool GrowlManager::onLLNotification(const LLSD& notice)
{
	if(notice["sigtype"].asString() != "add")
		return false;
	if(!gSavedSettings.getBOOL("EmeraldEnableGrowl"))
		return false;
	LLNotificationPtr notification = LLNotifications::instance().find(notice["id"].asUUID());
	std::string name = notification->getName();
	LLSD substitutions = notification->getSubstitutions();
	if(LLStartUp::getStartupState() < STATE_STARTED)
	{
		LL_WARNS("GrowlLLNotification") << "GrowlManager discarded a notification (" << name << ") - too early." << LL_ENDL;
		return false;
	}
	if(gGrowlManager->mNotifications.find(name) != gGrowlManager->mNotifications.end())
	{
		GrowlNotification* growl_notification = &gGrowlManager->mNotifications[name];
		std::string body = "";
		std::string title = "";
		if(growl_notification->useDefaultTextForTitle)
			title = notification->getMessage();
		else if(growl_notification->growlTitle != "")
			title = LLNotification::format(growl_notification->growlTitle, substitutions);
		if(growl_notification->useDefaultTextForBody)
			body = notification->getMessage();
		else if(growl_notification->growlBody != "")
			body = LLNotification::format(growl_notification->growlBody, substitutions);
		LL_INFOS("GrowlLLNotification") << "Notice: " << title << ": " << body << LL_ENDL;
		gGrowlManager->notify(title, body, growl_notification->growlName);
	}
	return false;
}

void GrowlManager::InitiateManager()
{
	gGrowlManager = new GrowlManager();
}

