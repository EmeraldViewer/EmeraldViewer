

#include "llviewerprecompiledheaders.h"

#include "llavatarconstants.h"
#include "llappviewer.h"
#include "floateravatarlist.h"

#include "lluictrlfactory.h"
#include "llviewerwindow.h"
#include "llscrolllistctrl.h"
#include "llviewercontrol.h"

#include "llvoavatar.h"
#include "llimview.h"
#include "llfloateravatarinfo.h"
#include "llregionflags.h"
#include "llfloaterreporter.h"
#include "llagent.h"
#include "llselectmgr.h"
#include "llviewerregion.h"
#include "lltracker.h"
#include "llviewercontrol.h"
#include "llviewerstats.h"
#include "llerror.h"
#include "llchat.h"
#include "llviewermessage.h"
#include "llweb.h"
#include "llviewerobjectlist.h"
#include "llmutelist.h"
#include "llchat.h"
#include "llfloaterchat.h"
#include "llcallbacklist.h"
#include "llviewerparcelmgr.h"

#include <time.h>
#include <string.h>

#include <map>


#include "llworld.h"

#include "llsdutil.h"
#include "jc_lslviewerbridge.h"
#include "v3dmath.h"

#include "scriptcounter.h"

#include "llfloaterregioninfo.h"
#include "llfocusmgr.h"

void cmdline_printchat(std::string message);

FloaterAvatarList* FloaterAvatarList::sInstance = NULL;

FloaterAvatarList::FloaterAvatarList() :  LLFloater(), LLEventTimer( 0.12f )
{
	if(sInstance)delete sInstance;
	sInstance = this;
	LLUICtrlFactory::getInstance()->buildFloater(sInstance, "floater_avatar_scanner.xml");

	mTracking = FALSE;
	mTrackByLocation = FALSE;
	last_av_req.start();
	
}

FloaterAvatarList::~FloaterAvatarList()
{
	//if(sInstance)delete sInstance;
	sInstance = NULL;
}

void FloaterAvatarList::onOpen()
{
	sInstance->setVisible(TRUE);
	gSavedSettings.setBOOL("ShowAvatarList", TRUE);
}

void FloaterAvatarList::onClose(bool app_quitting)
{
	if(app_quitting)
	{
		LLFloater::onClose(app_quitting);
	}else
	{
		gSavedSettings.setBOOL("ShowAvatarList", FALSE);
		if(gSavedSettings.getBOOL("EmeraldAvatarListKeepOpen"))
		{
			sInstance->setVisible(FALSE);
		}else
		{
			LLFloater::onClose(app_quitting);
			delete sInstance;
		}
	}
}

//static
void FloaterAvatarList::toggle()
{
	if (sInstance)
	{
		if(sInstance->getVisible())
		{
			sInstance->close();
		}else
		{
			sInstance->open();
		}
	}
	else
	{
		sInstance = new FloaterAvatarList();
	}
}

BOOL FloaterAvatarList::postBuild()
{
	childSetAction("profile_btn", onClickProfile, this);
	childSetAction("im_btn", onClickIM, this);
	childSetAction("offer_btn", onClickTeleportOffer, this);
	childSetAction("track_btn", onClickTrack, this);
	/*childSetAction("mark_btn", onClickMark, this);

	childSetAction("prev_in_list_btn", onClickPrevInList, this);
	childSetAction("next_in_list_btn", onClickNextInList, this);
	childSetAction("prev_marked_btn", onClickPrevMarked, this);
	childSetAction("next_marked_btn", onClickNextMarked, this);*/
	
	childSetAction("get_key_btn", onClickGetKey, this);

	childSetAction("freeze_btn", onClickFreeze, this);
	childSetAction("eject_btn", onClickEject, this);
	childSetAction("mute_btn", onClickMute, this);
	childSetAction("unmute_btn", onClickUnmute, this);
	childSetAction("ar_btn", onClickAR, this);
	childSetAction("teleport_btn", onClickTeleport, this);
	childSetAction("estate_kick_btn", onClickKickFromEstate, this);
	childSetAction("estate_ban_btn", onClickBanFromEstate, this);
	childSetAction("estate_tph_btn", onClickTPHFromEstate, this);
	childSetAction("estate_gtfo_btn", onClickGTFOFromEstate, this);
	childSetAction("script_count_btn", onClickScriptCount, this);

	//childSetCommitCallback("agealert", onClickAgeAlert,this);
	//childSetValue("agealert",*sEmeraldAvatarAgeAlert);

	//childSetCommitCallback("AgeAlertDays",onClickAgeAlertDays,this);
	//childSetValue("AgeAlertDays",*sEmeraldAvatarAgeAlertDays);*/

	mAvatarList = getChild<LLScrollListCtrl>("avatar_list");

	mAvatarList->setCallbackUserData(this);
	mAvatarList->setDoubleClickCallback(onDoubleClick);
	mAvatarList->sortByColumn("distance", TRUE);

	return TRUE;
}


extern U32 gFrameCount;

#define CLEANUP_TIMEOUT 3600.0f

#define MIN_REQUEST_INTERVAL 1.0f

#define FIRST_REQUEST_TIMEOUT 16.0f

typedef std::vector<std::string> strings_t;
static void sendEstateOwnerMessage(
	LLMessageSystem* msg,
	const std::string& request,
	const LLUUID& invoice,
	const strings_t& strings)
{
	llinfos << "Sending estate request '" << request << "'" << llendl;
	msg->newMessage("EstateOwnerMessage");
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null); //not used
	msg->nextBlock("MethodData");
	msg->addString("Method", request);
	msg->addUUID("Invoice", invoice);
	if(strings.empty())
	{
		msg->nextBlock("ParamList");
		msg->addString("Parameter", NULL);
	}
	else
	{
		strings_t::const_iterator it = strings.begin();
		strings_t::const_iterator end = strings.end();
		for(; it != end; ++it)
		{
			msg->nextBlock("ParamList");
			msg->addString("Parameter", *it);
		}
	}
	msg->sendReliable(gAgent.getRegion()->getHost());
}

std::string keyasname(LLUUID id)
{
	std::string ret;
	gCacheName->getFullName(id,ret);
	return ret;
}
void execEstateKick(LLUUID& target)
{
	cmdline_printchat("Moderation: Kicking "+ keyasname(target)+" from estate.");
	strings_t strings;
	strings.push_back(target.asString());
	sendEstateOwnerMessage(gMessageSystem, "kickestate", LLFloaterRegionInfo::getLastInvoice(), strings);
}


void FloaterAvatarList::chat_alerts(avatar_entry *entry, bool same_region, bool in_draw, bool in_chat)
{
	static BOOL *sEmeraldRadarChatAlerts = rebind_llcontrol<BOOL>("EmeraldRadarChatAlerts",&gSavedSettings,true);
	static BOOL *sEmeraldRadarAlertSim = rebind_llcontrol<BOOL>("EmeraldRadarAlertSim",&gSavedSettings,true);
	static BOOL *sEmeraldRadarAlertDraw = rebind_llcontrol<BOOL>("EmeraldRadarAlertDraw",&gSavedSettings,true);
	static BOOL *sEmeraldRadarAlertChatRange = rebind_llcontrol<BOOL>("EmeraldRadarAlertChatRange",&gSavedSettings,true);

	static F32 *sEmeraldAvatarAgeAlertDays = rebind_llcontrol<F32>("EmeraldAvatarAgeAlertDays",&gSavedSettings,true);
					
	if(*sEmeraldRadarChatAlerts)
	{
		if(entry->last_in_sim != same_region && *sEmeraldRadarAlertSim)
		{
			LLChat chat;
			chat.mFromName = entry->name;
			chat.mURL = llformat("secondlife:///app/agent/%s/about",entry->id.asString().c_str());
			chat.mText = entry->name+" has "+(same_region ? "entered" : "left")+" the sim.";
			chat.mSourceType = CHAT_SOURCE_SYSTEM;
			LLFloaterChat::addChat(chat);
		}

		if(entry->last_in_draw != in_draw && *sEmeraldRadarAlertDraw)
		{
			LLChat chat;
			chat.mFromName = entry->name;
			chat.mURL = llformat("secondlife:///app/agent/%s/about",entry->id.asString().c_str());
			chat.mText = entry->name+" has "+(in_draw ? "entered" : "left")+" draw distance.";
			chat.mSourceType = CHAT_SOURCE_SYSTEM;
			LLFloaterChat::addChat(chat);
		}

		if(entry->last_in_chat != in_chat && *sEmeraldRadarAlertChatRange)
		{
			LLChat chat;
			chat.mFromName = entry->name;
			chat.mURL = llformat("secondlife:///app/agent/%s/about",entry->id.asString().c_str());
			chat.mText = entry->name+" has "+(in_chat ? "entered" : "left")+" chat range.";
			chat.mSourceType = CHAT_SOURCE_SYSTEM;
			LLFloaterChat::addChat(chat);
		}

		if(!entry->age_alerted && entry->has_info)
		{
			entry->age_alerted = true;
			if(entry->account_age < *sEmeraldAvatarAgeAlertDays)
			{
				LLChat chat;
				chat.mFromName = entry->name;
				chat.mURL = llformat("secondlife:///app/agent/%s/about",entry->id.asString().c_str());

				make_ui_sound("EmeraldAvatarAgeAlertSoundUUID");
				chat.mText = entry->name+" has triggered your avatar age alert.";
				chat.mSourceType = CHAT_SOURCE_SYSTEM;
				LLFloaterChat::addChat(chat);
			}
		}
	}
}

BOOL FloaterAvatarList::tick()
{
	if(!gDisconnected)
	{

		LLCheckboxCtrl* check = getChild<LLCheckboxCtrl>("update_enabled_cb");

		if(check && check->getValue())
		{
			//UPDATES//
			LLVector3d agent_pos = gAgent.getPositionGlobal();
			std::vector<LLUUID> avatar_ids;
			std::vector<LLUUID> sorted_avatar_ids;
			std::vector<LLVector3d> positions;

			LLWorld::instance().getAvatars(&avatar_ids, &positions,agent_pos);

			sorted_avatar_ids = avatar_ids;
			std::sort(sorted_avatar_ids.begin(), sorted_avatar_ids.end());

			for(std::vector<LLCharacter*>::const_iterator iter = LLCharacter::sInstances.begin(); iter != LLCharacter::sInstances.end(); ++iter)
			{
				LLUUID avid = (*iter)->getID();

				if(!std::binary_search(sorted_avatar_ids.begin(), sorted_avatar_ids.end(), avid))
				{
					avatar_ids.push_back(avid);
				}
			}

			int i = 0;
			int len = avatar_ids.size();
			for(i = 0; i < len; i++)
			{
				LLUUID &avid = avatar_ids[i];
				if(avid != gAgent.getID())
				{

					std::string name;
					std::string first;
					std::string last;
					LLVector3d position;
					LLViewerObject *obj = gObjectList.findObject(avid);

					bool same_region = false;

					bool in_draw = false;

					if(obj)
					{
						LLVOAvatar* avatarp = dynamic_cast<LLVOAvatar*>(obj);

						if (avatarp == NULL)
						{
							continue;
						}
						if (avatarp->isDead())
						{
							continue;
						}
						position = gAgent.getPosGlobalFromAgent(avatarp->getCharacterPosition());

						in_draw = true;

						same_region = avatarp->getRegion() == gAgent.getRegion();

					}else
					{
						if( i < (int)positions.size())
						{
							position = positions[i];
						}
						else
						{
							continue;
						}

						same_region = gAgent.getRegion()->pointInRegionGlobal(position);
					}
					if(gCacheName->getName(avid, first, last))
					{
						name = first + " " + last;
					}
					else
					{
						continue;//name = LLCacheName::getDefaultName();
					}

					
					if (mAvatars.find(avid) == mAvatars.end())
					{
						avatar_entry new_entry;
						new_entry.id = avid;
						new_entry.is_linden = (strcmp(last.c_str(),"Linden") == 0 || strcmp(last.c_str(),"Tester") == 0) ? true : false;

						mAvatars[avid] = new_entry;

					}
					avatar_entry* entry = &(mAvatars[avid]);
					entry->name = name;
					entry->position = position;
					entry->same_region = same_region;
					entry->last_update_frame = gFrameCount;
					entry->last_update_sec.reset();

					bool in_chat = (position - agent_pos).magVec() < 20.0f;

					
					chat_alerts(entry,same_region, in_draw, in_chat);

					if(entry->last_in_sim != same_region)
					{
						LLViewerRegion* regionp = gAgent.getRegion();
						if(sInstance->mEstateMemoryBans.find(regionp) != sInstance->mEstateMemoryBans.end())
						{
							std::set<LLUUID> *mem = &mEstateMemoryBans[regionp];
							if(mem->find(avid) != mem->end())
							{
								cmdline_printchat("Enforcing memory ban.");
								execEstateKick(avid);
							}
						}
						if(gSavedSettings.getBOOL("EmeraldRadarChatKeys"))
						{
							gMessageSystem->newMessage("ScriptDialogReply");
							gMessageSystem->nextBlock("AgentData");
							gMessageSystem->addUUID("AgentID", gAgent.getID());
							gMessageSystem->addUUID("SessionID", gAgent.getSessionID());
							gMessageSystem->nextBlock("Data");
							gMessageSystem->addUUID("ObjectID", gAgent.getID());
							gMessageSystem->addS32("ChatChannel", gSavedSettings.getS32("EmeraldRadarChatKeysChannel"));
							gMessageSystem->addS32("ButtonIndex", 1);
							gMessageSystem->addString("ButtonLabel",llformat("%d,%d,", gFrameCount, same_region) + avid.asString());
							gAgent.sendReliableMessage();
						}
					}
					entry->last_in_sim = same_region;
					entry->last_in_draw = in_draw;
					entry->last_in_chat = in_chat;
				}
			}
			//END UPDATES//


			//EXPIRE//
			std::map<LLUUID, avatar_entry>::iterator iter;
			std::queue<LLUUID> delete_queue;

			
			for(iter = mAvatars.begin(); iter != mAvatars.end(); iter++)
			{
				//avatar_entry *entry = &iter->second;
				
				F32 aged = iter->second.last_update_sec.getElapsedTimeF32();
				if ( aged > CLEANUP_TIMEOUT )
				{
					LLUUID av_id = iter->first;
					delete_queue.push(av_id);
				}else if(iter->second.last_update_frame != gFrameCount)
				{
					chat_alerts(&iter->second,false, false, false);
					iter->second.last_in_sim = false;
					iter->second.last_in_draw = false;
					iter->second.last_in_chat = false;
				}
			}

			while(!delete_queue.empty())
			{
				mAvatars.erase(delete_queue.front());
				delete_queue.pop();
			}
			//END EXPIRE//


			//DRAW//
			if (sInstance->getVisible())
			{
				LLCheckboxCtrl* fetch_data = getChild<LLCheckboxCtrl>("fetch_avdata_enabled_cb");
				BOOL fetching = fetch_data->getValue();

				LLDynamicArray<LLUUID> selected = mAvatarList->getSelectedIDs();
				S32 scrollpos = mAvatarList->getScrollPos();

				mAvatarList->deleteAllItems();

				std::map<LLUUID, avatar_entry>::iterator iter;

				for(iter = mAvatars.begin(); iter != mAvatars.end(); iter++)
				{

					
					
					LLUUID av_id;

					av_id = iter->first;
					avatar_entry *entry = &iter->second;

					//llinfos << entry->last_update_sec.getElapsedSeconds() << llendl;
					if(entry->last_update_sec.getElapsedTimeF32() < 5.0f)
					{
						LLSD element;

						if(entry->has_info == false && fetching && last_av_req.getElapsedTimeF32() > MIN_REQUEST_INTERVAL)
						{
							F32 now = entry->lifetime.getElapsedTimeF32();
							F32 diff = now - entry->last_info_req;
							bool requested = entry->info_requested();
							if(diff > entry->request_timeout || !requested)
							{
								
								if(requested && entry->request_timeout < 256.0f)entry->request_timeout *= 2.0f;

								last_av_req.reset();

								entry->last_info_req = entry->lifetime.getElapsedTimeF32();


								LLMessageSystem *msg = gMessageSystem;
								msg->newMessageFast(_PREHASH_AvatarPropertiesRequest);
								msg->nextBlockFast(_PREHASH_AgentData);
								msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
								msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
								msg->addUUIDFast(_PREHASH_AvatarID, av_id);
								gAgent.sendReliableMessage();

							}
						}


						LLVector3d delta = entry->position - agent_pos;
						F32 distance = (F32)delta.magVec();
						delta.mdV[VZ] = 0.0f;
						//F32 side_distance = (F32)delta.magVec();

						if(av_id.isNull())continue;

						element["id"] = av_id;

						std::string icon = "";

						if(entry->is_linden)
						{
							element["columns"][LIST_AVATAR_NAME]["font-style"] = "BOLD";
						}

						element["columns"][LIST_AVATAR_NAME]["column"] = "avatar_name";
						element["columns"][LIST_AVATAR_NAME]["type"] = "text";
						element["columns"][LIST_AVATAR_NAME]["value"] = entry->name;

						element["columns"][LIST_DISTANCE]["column"] = "distance";
						element["columns"][LIST_DISTANCE]["type"] = "text";
						
						if(entry->position.mdV[VZ] == 0.0)
						{
							static F32 *sRenderFarClip = rebind_llcontrol<F32>("RenderFarClip",&gSavedSettings,true);
							element["columns"][LIST_DISTANCE]["value"] = llformat("> %d", S32(*sRenderFarClip) );
						}else
						{
							element["columns"][LIST_DISTANCE]["value"] = llformat("%.2f", distance);
						}

						LLColor4 dist_color;

						static LLColor4U *sAvatarListTextDistNormalRange = rebind_llcontrol<LLColor4U>("AvatarListTextDistNormalRange",&gColors,true);
						static LLColor4U *sAvatarListTextDistShoutRange = rebind_llcontrol<LLColor4U>("AvatarListTextDistShoutRange",&gColors,true);
						static LLColor4U *sAvatarListTextDistOver = rebind_llcontrol<LLColor4U>("AvatarListTextDistOver",&gColors,true);

						if(distance <= 20.0f)dist_color = LLColor4(*sAvatarListTextDistNormalRange);
						else if(distance > 20.0f && distance <= 96.0f)dist_color = LLColor4(*sAvatarListTextDistShoutRange);
						else dist_color = LLColor4(*sAvatarListTextDistOver);

						element["columns"][LIST_DISTANCE]["color"] = dist_color.getValue();


						if(entry->has_info)
						{
							static F32 *sEmeraldAvatarAgeAlertDays = rebind_llcontrol<F32>("EmeraldAvatarAgeAlertDays",&gSavedSettings,true);
							if(entry->account_age < *sEmeraldAvatarAgeAlertDays && entry->age_alerted == false)
							{
								entry->age_alerted = true;
							}
							element["columns"][LIST_AGE]["column"] = "age";
							element["columns"][LIST_AGE]["type"] = "text";
							element["columns"][LIST_AGE]["value"] = llformat("%d",entry->account_age);

							LLColor4 age_color;

							static LLColor4U *sAvatarListTextAgeYoung = rebind_llcontrol<LLColor4U>("AvatarListTextAgeYoung",&gColors,true);
							static LLColor4U *sAvatarListTextAgeNormal = rebind_llcontrol<LLColor4U>("AvatarListTextAgeNormal",&gColors,true);

							if ( entry->account_age <= 7 )age_color = LLColor4(*sAvatarListTextAgeYoung);
							else age_color = LLColor4(*sAvatarListTextAgeNormal);

							element["columns"][LIST_AGE]["color"] = age_color.getValue();
						}

						element["columns"][LIST_SIM]["column"] = "samesim";
						element["columns"][LIST_SIM]["type"] = "text";

						icon = "";
						if(entry->last_update_sec.getElapsedTimeF32() < 0.5)
						{
							if(entry->same_region)
							{
								icon = "account_id_green.tga";
							}
						}else
						{
							icon = "avatar_gone.tga";
						}
							
						if (!icon.empty() )
						{	
							element["columns"][LIST_SIM].erase("color");
							element["columns"][LIST_SIM]["type"] = "icon";
							element["columns"][LIST_SIM]["value"] = icon;
						}

						icon = "";

						if(!entry->has_info)
						{
							if(entry->info_requested())
							{
								if(entry->request_timeout < 256.0f) icon = "info_fetching.tga";
								else icon = "info_error.tga";
							}else icon = "info_unknown.tga";
						}else
						{
							switch(entry->agent_info.payment)
							{
							case PAYMENT_NONE:
								break;
							case PAYMENT_ON_FILE:
								icon = "payment_info_filled.tga";
								break;
							case PAYMENT_USED:
								icon = "payment_info_used.tga";
								break;
							case PAYMENT_LINDEN:
								// confusingly named icon, maybe use something else
								icon = "icon_top_pick.tga";
								break;
							}
						}
						element["columns"][LIST_PAYMENT]["column"] = "payment_data";
						element["columns"][LIST_PAYMENT]["type"] = "text";
						if ( !icon.empty() )
						{
							element["columns"][LIST_PAYMENT]["type"] = "icon";
							element["columns"][LIST_PAYMENT]["value"] =  icon;
						}

						S32 seentime = (S32)entry->lifetime.getElapsedTimeF32();
						S32 hours = (S32)(seentime / (60*60));
						S32 mins = (S32)((seentime - hours*(60*60)) / 60);
						S32 secs = (S32)((seentime - hours*(60*60) - mins*60));

						element["columns"][LIST_TIME]["column"] = "time";
						element["columns"][LIST_TIME]["type"] = "text";
						//element["columns"][LIST_TIME]["color"] = sDefaultListText->getValue();
						element["columns"][LIST_TIME]["value"] = llformat("%d:%02d:%02d", hours,mins,secs);




						element["columns"][LIST_CLIENT]["column"] = "client";
						element["columns"][LIST_CLIENT]["type"] = "text";

						static LLColor4U *sAvatarNameColor = rebind_llcontrol<LLColor4U>("AvatarNameColor",&gColors,true);
						static LLColor4U *sScrollUnselectedColor = rebind_llcontrol<LLColor4U>("ScrollUnselectedColor",&gColors,true);

						LLColor4 avatar_name_color = LLColor4(*sAvatarNameColor);
						std::string client;
						LLVOAvatar *av = (LLVOAvatar*)gObjectList.findObject(av_id);
						if(av)
						{
							LLVOAvatar::resolveClient(avatar_name_color, client, av);
							if(client == "")
							{
								avatar_name_color = LLColor4(*sScrollUnselectedColor);
								client = "?";
							}
							element["columns"][LIST_CLIENT]["value"] = client.c_str();
						}
						else
						{
							element["columns"][LIST_CLIENT]["value"] = "Out Of Range";
							avatar_name_color = LLColor4(*sScrollUnselectedColor);
						}

						avatar_name_color = (avatar_name_color * 0.5) + (LLColor4(*sScrollUnselectedColor) * 0.5);

						element["columns"][LIST_CLIENT]["color"] = avatar_name_color.getValue();

						mAvatarList->addElement(element, ADD_BOTTOM);
					}
				}
				mAvatarList->sortItems();
				mAvatarList->selectMultiple(selected);
				mAvatarList->setScrollPos(scrollpos);
			}
			//END DRAW//
		}
	}

	if( mTracking && LLTracker::getTrackedPositionGlobal().isExactlyZero() )
	{
		// trying to track an avatar, but tracker stopped tracking		
		if ( mAvatars.find(mTrackedAvatar) != mAvatars.end() && !mTrackByLocation)
		{
			mTrackByLocation = TRUE;
		}
		else
		{
			LLTracker::stopTracking(NULL);
			mTracking = FALSE;
		}
	}

	if ( mTracking && mTrackByLocation)
	{
		std::string name = mAvatars[mTrackedAvatar].name;
		std::string tooltip = "Tracking last known position";
		name += " (near)";
		LLTracker::trackLocation(mAvatars[mTrackedAvatar].position,name, tooltip);
	}

	return FALSE;
}

void FloaterAvatarList::processAvatarPropertiesReply(LLMessageSystem *msg, void**)
{
	if(!sInstance)return;

	BOOL	identified = FALSE;
	BOOL	transacted = FALSE;

	LLUUID	agent_id;	// your id
	LLUUID	avatar_id;	// target of this panel
	U32	flags = 0x0;
	char	born_on[DB_BORN_BUF_SIZE];
	S32	charter_member_size = 0;

	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AvatarID, avatar_id );

	if(sInstance->mAvatars.find(avatar_id) == sInstance->mAvatars.end())return;

	avatar_entry *entry = &(sInstance->mAvatars[avatar_id]);

	msg->getStringFast(_PREHASH_PropertiesData, _PREHASH_BornOn, DB_BORN_BUF_SIZE, born_on);
	msg->getU32Fast(_PREHASH_PropertiesData, _PREHASH_Flags, flags);

	identified = (flags & AVATAR_IDENTIFIED);
	transacted = (flags & AVATAR_TRANSACTED);

	U8 caption_index = 0;
	std::string caption_text;
	charter_member_size = msg->getSize("PropertiesData", "CharterMember");

	if(1 == charter_member_size)
	{
		msg->getBinaryData("PropertiesData", "CharterMember", &caption_index, 1);
	}
	else if(1 < charter_member_size)
	{
		char caption[MAX_STRING];
		msg->getString("PropertiesData", "CharterMember", MAX_STRING, caption);

		caption_text = caption;
		//entry->setAccountCustomTitle(caption_text);
	}

	const char* ACCT_TYPE[] = {
				"Resident",
				"Trial",
				"Charter Member",
				"Linden Lab Employee"
			};
	if(caption_text.empty())
	{
		caption_index = llclamp(caption_index, (U8)0, (U8)(LL_ARRAY_SIZE(ACCT_TYPE)-1));
		caption_text = ACCT_TYPE[caption_index];
	}else
	{
		caption_index = ACCOUNT_CUSTOM;
	}
	entry->has_info = true;
	//entry->account_title = caption_text;
	entry->agent_info.caption = caption_text;
	entry->agent_info.caption_index = caption_index;
	entry->birthdate = born_on;

	U8 payment;
	if (caption_index != ACCOUNT_EMPLOYEE )
	{
		if ( transacted )
		{
			payment = PAYMENT_USED;
		}
		else if ( identified )
		{
			payment = PAYMENT_ON_FILE;
		}
		else
		{
			payment = PAYMENT_NONE;
		}
	}
	else
	{
		payment = PAYMENT_LINDEN;
		entry->is_linden = true;
	}
	entry->agent_info.payment = payment;
	

	tm birthdate;
	memset(&birthdate, 0, sizeof(birthdate));

	int num_read = sscanf(born_on, "%d/%d/%d", &birthdate.tm_mon,
	                                           &birthdate.tm_mday,
	                                           &birthdate.tm_year);

	if ( num_read == 3 && birthdate.tm_mon <= 12 )
	{
		birthdate.tm_year -= 1900;
		birthdate.tm_mon--;
	}
	else
	{
		// Zero again to remove any partially read data
		memset(&birthdate, 0, sizeof(birthdate));
		llwarns << "Error parsing birth date: " << born_on << llendl;
	}

	time_t birth = mktime(&birthdate);
	time_t now = time(NULL);
	U32 age = (U32)(difftime(now,birth) / (60*60*24));

	entry->account_age = age;

}

void FloaterAvatarList::processSoundTrigger(LLMessageSystem* msg,void**)
{
        LLUUID  sound_id,owner_id;
        msg->getUUIDFast(_PREHASH_SoundData, _PREHASH_SoundID, sound_id);
        msg->getUUIDFast(_PREHASH_SoundData, _PREHASH_OwnerID, owner_id);
        if(owner_id == gAgent.getID() && sound_id == LLUUID("76c78607-93f9-f55a-5238-e19b1a181389"))
        {
                //lgg we need to auto turn on settings for ppl now that we know they has the thingy
                if(gSavedSettings.getBOOL("EmeraldRadarChatKeys"))
                {
			int num_ids = 0;
			int first=1;
			std::ostringstream ids;
			std::map<LLUUID, avatar_entry>::iterator iter;
			FloaterAvatarList* self = FloaterAvatarList::getInstance();
			for(iter = self->mAvatars.begin(); iter != self->mAvatars.end(); iter++)
			{
				const LLUUID *avid = &iter->first;
				if(*avid != gAgent.getID())
				{
					++num_ids;
					if(first==1)
					{
						first = 0;
						ids << avid->asString();
					}
					else
						ids << "," << avid->asString();
					if(ids.tellp() > 200)
					{				
						gMessageSystem->newMessage("ScriptDialogReply");
						gMessageSystem->nextBlock("AgentData");
						gMessageSystem->addUUID("AgentID", gAgent.getID());
						gMessageSystem->addUUID("SessionID", gAgent.getSessionID());
						gMessageSystem->nextBlock("Data");
						gMessageSystem->addUUID("ObjectID", gAgent.getID());
						gMessageSystem->addS32("ChatChannel", gSavedSettings.getS32("EmeraldRadarChatKeysChannel"));
						gMessageSystem->addS32("ButtonIndex", 1);
						gMessageSystem->addString("ButtonLabel",llformat("%d,%d,", gFrameCount, num_ids) + ids.str());
						gAgent.sendReliableMessage();
						num_ids = 0;
						ids.seekp(0);
						ids.str("");
						first=1;
					}
				}
			}
			if(num_ids)
			{
				gMessageSystem->newMessage("ScriptDialogReply");
				gMessageSystem->nextBlock("AgentData");
				gMessageSystem->addUUID("AgentID", gAgent.getID());
				gMessageSystem->addUUID("SessionID", gAgent.getSessionID());
				gMessageSystem->nextBlock("Data");
				gMessageSystem->addUUID("ObjectID", gAgent.getID());
				gMessageSystem->addS32("ChatChannel", gSavedSettings.getS32("EmeraldRadarChatKeysChannel"));
				gMessageSystem->addS32("ButtonIndex", 1);
				gMessageSystem->addString("ButtonLabel",llformat("%d,%d,", gFrameCount, num_ids) + ids.str());
				gAgent.sendReliableMessage();
				num_ids = 0;
				ids.seekp(0);
				ids.str("");
			}
                }
        }
}

std::string FloaterAvatarList::getSelectedNames(const std::string& separator)
{
	std::string ret = "";
	
	LLDynamicArray<LLUUID> ids = mAvatarList->getSelectedIDs();
	for(LLDynamicArray<LLUUID>::iterator itr = ids.begin(); itr != ids.end(); ++itr)
	{
		LLUUID avid = *itr;
		if (!ret.empty()) ret += separator;
		ret += keyasname(avid);
	}

	return ret;
}

std::string FloaterAvatarList::getSelectedName()
{
	LLUUID id = getSelectedID();
	return keyasname(id);
}

LLUUID FloaterAvatarList::getSelectedID()
{
	LLScrollListItem *item = mAvatarList->getFirstSelected();
	if(item) return item->getUUID();
	return LLUUID::null;
}

void FloaterAvatarList::doCommand(avlist_command_t cmd)
{
	LLDynamicArray<LLUUID> ids = sInstance->mAvatarList->getSelectedIDs();

	for(LLDynamicArray<LLUUID>::iterator itr = ids.begin(); itr != ids.end(); ++itr)
	{
		LLUUID avid = *itr;
		cmd(avid);
	}
}



void execProfile(LLUUID& target)
{
	LLFloaterAvatarInfo::showFromDirectory(target);
}
void FloaterAvatarList::onClickProfile(void *userdata)
{
	doCommand(&execProfile);
}

void FloaterAvatarList::onClickIM(void *userdata)
{
	//llinfos << "LLFloaterFriends::onClickIM()" << llendl;
	FloaterAvatarList *avlist = (FloaterAvatarList*)userdata;

	LLDynamicArray<LLUUID> ids = avlist->mAvatarList->getSelectedIDs();
	if(ids.size() > 0)
	{
		if(ids.size() == 1)
		{
			// Single avatar
			LLUUID agent_id = ids[0];

			//char buffer[MAX_STRING];
			//snprintf(buffer, MAX_STRING, "%s", avlist->mAvatars[agent_id].getName().c_str());
			//std::string name;
			//gCacheName->getFullName(agent_id,&name);
			gIMMgr->setFloaterOpen(TRUE);
			gIMMgr->addSession(
				keyasname(agent_id),
				IM_NOTHING_SPECIAL,
				agent_id);
		}
		else
		{
			// Group IM
			LLUUID session_id;
			session_id.generate();
			gIMMgr->setFloaterOpen(TRUE);
			gIMMgr->addSession("Avatars Conference", IM_SESSION_CONFERENCE_START, ids[0], ids);
		}
	}
}
/*
bool handle_lure_callback(const LLSD& notification, const LLSD& response);
// Prompt for a message to the invited user.
void handle_lure(LLDynamicArray<LLUUID>& ids) 
{
	LLSD edit_args;
	edit_args["REGION"] = gAgent.getRegion()->getName();

	LLSD payload;
	for (LLDynamicArray<LLUUID>::iterator it = ids.begin();
		it != ids.end();
		++it)
	{
		cmdline_printchat("Offering teleport to "+ keyasname(*it)+".");
		payload["ids"].append(*it);
	}
	LLNotifications::instance().add("OfferTeleport", edit_args, payload, handle_lure_callback);
}*/
void FloaterAvatarList::onClickTeleportOffer(void *userdata)
{
	FloaterAvatarList *avlist = (FloaterAvatarList*)userdata;

	LLDynamicArray<LLUUID> ids = avlist->mAvatarList->getSelectedIDs();
	if(ids.size() > 0)
	{
		handle_lure(ids);
	}
}

void FloaterAvatarList::onClickTrack(void *userdata)
{
	FloaterAvatarList *avlist = (FloaterAvatarList*)userdata;
	
 	LLScrollListItem *item =   avlist->mAvatarList->getFirstSelected();
	if (!item) return;

	LLUUID agent_id = item->getUUID();

	if ( avlist->mTracking && avlist->mTrackedAvatar == agent_id ) {
		LLTracker::stopTracking(NULL);
		avlist->mTracking = FALSE;
	}
	else
	{
		avlist->mTracking = TRUE;
		avlist->mTrackByLocation = FALSE;
		avlist->mTrackedAvatar = agent_id;
		//std::string name;
		//gCacheName->getFullName(agent_id,&name);
		LLTracker::trackAvatar(agent_id, keyasname(agent_id));
	}
}

void execGetKey(LLUUID& target)
{
	//std::string name;
	//gCacheName->getFullName(agent_id,&name);

	gViewerWindow->mWindow->copyTextToClipboard(utf8str_to_wstring(target.asString()));
	cmdline_printchat(keyasname(target)+" UUID: "+target.asString());
}
void FloaterAvatarList::onClickGetKey(void *userdata)
{
	doCommand(&execGetKey);
}

void FloaterAvatarList::lookAtAvatar(LLUUID &uuid)
{ // twisted laws
    LLViewerObject* voavatar = gObjectList.findObject(uuid);
    if(voavatar && voavatar->isAvatar())
    {
        gAgent.setFocusOnAvatar(FALSE, FALSE);
        gAgent.changeCameraToThirdPerson();
        gAgent.setFocusGlobal(voavatar->getPositionGlobal(),uuid);
        gAgent.setCameraPosAndFocusGlobal(voavatar->getPositionGlobal() 
                + LLVector3d(3.5,1.35,0.75) * voavatar->getRotation(), 
                                                voavatar->getPositionGlobal(), 
                                                uuid );
    }
}

void FloaterAvatarList::onDoubleClick(void *userdata)
{
	FloaterAvatarList *self = (FloaterAvatarList*)userdata;
 	LLScrollListItem *item =   self->mAvatarList->getFirstSelected();
	LLUUID agent_id = item->getUUID();
	lookAtAvatar(agent_id);
}

void send_freeze(const LLUUID& avatar_id, bool freeze)
{
	U32 flags = 0x0;
	if (!freeze)
	{
		// unfreeze
		flags |= 0x1;
	}

	LLMessageSystem* msg = gMessageSystem;
	LLViewerObject* avatar = gObjectList.findObject(avatar_id);

	if (avatar)
	{
		msg->newMessage("FreezeUser");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgent.getID());
		msg->addUUID("SessionID", gAgent.getSessionID());
		msg->nextBlock("Data");
		msg->addUUID("TargetID", avatar_id );
		msg->addU32("Flags", flags );
		msg->sendReliable( avatar->getRegion()->getHost() );
	}
}

void execFreeze(LLUUID& target)
{
	cmdline_printchat("Moderation: Freezing "+ keyasname(target)+".");
	send_freeze(target, true);
}
void execUnfreeze(LLUUID& target)
{
	cmdline_printchat("Moderation: Unfreezing "+ keyasname(target)+".");
	send_freeze(target, false);
}
void FloaterAvatarList::callbackFreeze(const LLSD& notification, const LLSD& response)
{
	S32 option = LLNotification::getSelectedOption(notification, response);

	if ( option == 0 )
	{
		sInstance->doCommand(&execFreeze);
	}
	else if ( option == 1 )
	{
		sInstance->doCommand(&execUnfreeze);
	}
}
void FloaterAvatarList::onClickFreeze(void *userdata)
{
	//doCommand(&exec);
	LLSD args;
	args["AVATAR_NAME"] = sInstance->getSelectedNames();
	LLNotifications::instance().add("FreezeAvatarFullname", args, LLSD(), callbackFreeze);
}

void send_eject(const LLUUID& avatar_id, bool ban)
{	
	LLMessageSystem* msg = gMessageSystem;
	LLViewerObject* avatar = gObjectList.findObject(avatar_id);

	if (avatar)
	{
		U32 flags = 0x0;
		if ( ban )
		{
			// eject and add to ban list
			flags |= 0x1;
		}

		msg->newMessage("EjectUser");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgent.getID() );
		msg->addUUID("SessionID", gAgent.getSessionID() );
		msg->nextBlock("Data");
		msg->addUUID("TargetID", avatar_id );
		msg->addU32("Flags", flags );
		msg->sendReliable( avatar->getRegion()->getHost() );
	}
}

void execEject(LLUUID& target)
{
	send_eject(target, false);
}
void FloaterAvatarList::onClickEject(void *userdata)
{
	doCommand(&execEject);
}

void execBan(LLUUID& target)
{
	send_eject(target, true);
}
void FloaterAvatarList::onClickBan(void *userdata)
{
	doCommand(&execBan);
}

/*void execUnban(LLUUID& target)
{
	LLViewerParcelMgr::getInstance()->getAgentParcel()
}
void FloaterAvatarList::onClickUnban(void *userdata)
{
	doCommand(&execUnban);
}*/

void execMute(LLUUID& target)
{
	cmdline_printchat("Muting "+ keyasname(target)+".");
	
	if (!LLMuteList::getInstance()->isMuted(target))
	{
		LLMute mute(target, keyasname(target), LLMute::AGENT);
		LLMuteList::getInstance()->add(mute);
	}
}
void FloaterAvatarList::onClickMute(void *userdata)
{
	doCommand(&execMute);
}

void execUnmute(LLUUID& target)
{
	cmdline_printchat("Unmuting "+ keyasname(target)+".");
	if (LLMuteList::getInstance()->isMuted(target))
	{
		LLMute mute(target, keyasname(target), LLMute::AGENT);
		LLMuteList::getInstance()->remove(mute);	
	}
}
void FloaterAvatarList::onClickUnmute(void *userdata)
{
	doCommand(&execUnmute);
}

void FloaterAvatarList::onClickAR(void *userdata)
{
	FloaterAvatarList *avlist = (FloaterAvatarList*)userdata;
	
 	LLScrollListItem *item =   avlist->mAvatarList->getFirstSelected();
	if (!item) return;

	LLUUID agent_id = item->getUUID();

	cmdline_printchat("Opening AR window for "+ keyasname(agent_id)+".");
	LLFloaterReporter::showFromObject(agent_id);
}

void FloaterAvatarList::onClickTeleport(void *userdata)
{
	//doCommand(&exec);
	FloaterAvatarList *avlist = (FloaterAvatarList*)userdata;

	LLScrollListItem *item =   avlist->mAvatarList->getFirstSelected();
	if (!item) return;

	LLUUID agent_id = item->getUUID();

	if(avlist->mAvatars.find(agent_id) != avlist->mAvatars.end())
	{
		cmdline_printchat("Teleporting to "+ keyasname(agent_id)+".");
		gAgent.teleportViaLocation( avlist->mAvatars[agent_id].position );
	}
}



void FloaterAvatarList::onClickKickFromEstate(void *userdata)
{
	doCommand(&execEstateKick);
}

void execEstateBan(LLUUID& target)
{
	cmdline_printchat("Moderation: Banning "+ keyasname(target)+" from estate.");
	//send_estate_message("teleporthomeuser", avatar); // Kick first, just to be sure
	LLPanelEstateInfo::sendEstateAccessDelta(ESTATE_ACCESS_BANNED_AGENT_ADD | ESTATE_ACCESS_ALLOWED_AGENT_REMOVE | ESTATE_ACCESS_NO_REPLY, target);
}
void FloaterAvatarList::onClickBanFromEstate(void *userdata)
{
	doCommand(&execEstateBan);
}

void execTPH(LLUUID& target)
{
	cmdline_printchat("Moderation: Teleporting "+ keyasname(target)+" home.");
	strings_t strings;
	std::string buffer;
	gAgent.getID().toString(buffer);
	strings.push_back(buffer);
	target.toString(buffer);
	strings.push_back(strings_t::value_type(buffer));
	LLUUID invoice(LLFloaterRegionInfo::getLastInvoice());
	sendEstateOwnerMessage(gMessageSystem, "teleporthomeuser", invoice, strings);
}
void FloaterAvatarList::onClickTPHFromEstate(void *userdata)
{
	doCommand(&execTPH);
}

void execMemBan(LLUUID& target)
{
	LLViewerRegion* regionp = gAgent.getRegion();
	FloaterAvatarList* self = FloaterAvatarList::getInstance();
	if(self->mEstateMemoryBans.find(regionp) == self->mEstateMemoryBans.end())
	{
		self->mEstateMemoryBans[regionp] = std::set<LLUUID>();
	}
	std::set<LLUUID> *mem = &(self->mEstateMemoryBans[regionp]);
	if(mem->find(target) == mem->end())mem->insert(target);
}
void FloaterAvatarList::onClickGTFOFromEstate(void *userdata)
{
	doCommand(&execEstateBan);
	doCommand(&execTPH);
	doCommand(&execEstateKick);
	doCommand(&execMemBan);
	cmdline_printchat("Agent UUIDs recorded for automatic estate kick on re-entry.");
}

void execScriptCount(LLUUID& target)
{

}
void FloaterAvatarList::onClickScriptCount(void *userdata)
{
	doCommand(&execScriptCount);
}















