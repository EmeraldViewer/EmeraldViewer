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
#include "llnotify.h"
#include "llstartup.h"
#include "growlmanager.h"
#include "growlnotifier.h"

// Platform-specific includes
#ifdef LL_DARWIN
#include "growlnotifiermacosx.h"
#endif

GrowlManager *gGrowlManager = NULL;

GrowlManager::GrowlManager()
{
	// Create a notifier appropriate to the platform.
#ifdef LL_DARWIN
	this->mNotifier = new GrowlNotifierMacOSX();
	LL_INFOS("GrowlManagerInit") << "Created GrowlNotifierMacOSX." << LL_ENDL;
#else
	this->mNotifier = new GrowlNotifier();
	LL_WARNS("GrowlManagerInit") << "Created generic GrowlNotifier." << LL_ENDL;
#endif
	
	// Hook into LLNotifications...
	LLNotificationChannel::buildChannel("GrowlNotifyTips", "Visible", LLNotificationFilters::filterBy<std::string>(&LLNotification::getType, "notifytip"));
	LLNotifications::instance().getChannel("GrowlNotifyTips")->connectChanged(&GrowlManager::onLLNotification);
	LL_INFOS("GrowlManagerInit") << "Connected to notifytips." << llendl;
}

void GrowlManager::notify(const std::string& notification_title, const std::string& notification_message, const std::string& notification_type)
{
	this->mNotifier->showNotification(notification_title, notification_message, notification_type);
}

bool GrowlManager::onLLNotification(const LLSD& notice)
{
	LL_INFOS("GrowlLLNotification") << "GrowlManager recieved a notification." << LL_ENDL;
	LLNotificationPtr notification = LLNotifications::instance().find(notice["id"].asUUID());
	std::string name = notification->getName();
	if(LLStartUp::getStartupState() < STATE_STARTED)
	{
		LL_WARNS("GrowlLLNotification") << "GrowlManager discarded a notification (" << name << ") - too early." << LL_ENDL;
		return false;
	}
	//TODO: Make this better - move everything into a file instead of hardcoding like this?
	if(name == "FriendOnline" || name == "FriendOffline")
	{
		gGrowlManager->notify(notification->getMessage(), "", "Friend logged on/off");
	}
	return false;
}

void GrowlManager::InitiateManager()
{
	gGrowlManager = new GrowlManager();
}

