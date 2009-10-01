/** 
 * @file llfloaterao.cpp
 * @brief clientside animation overrider
 * by Skills Hak
 */

#include "llviewerprecompiledheaders.h"

#include "llfloaterao.h"

#include "llagent.h"
#include "llvoavatar.h"
#include "llanimationstates.h"
#include "llframetimer.h"
#include "lluictrlfactory.h"
#include "llinventoryview.h"
#include "llstartup.h"
#include "llpreviewnotecard.h"
#include "llviewertexteditor.h"
#include "llcheckboxctrl.h"
#include "chatbar_as_cmdline.h"

#include "llinventory.h"
#include "llinventoryview.h"
#include "roles_constants.h"
#include "llviewerregion.h"

#include "llpanelinventory.h"
#include "llinventorybridge.h"

#include <boost/regex.hpp>

void cmdline_printchat(std::string message);

// -------------------------------------------------------

AOStandTimer::AOStandTimer() : LLEventTimer( gSavedSettings.getF32("EmeraldAOStandInterval") )
{
	LLFloaterAO::ChangeStand();
}
AOStandTimer::~AOStandTimer()
{
}
BOOL AOStandTimer::tick()
{
	LLFloaterAO::stand_iterator++;
	return LLFloaterAO::ChangeStand();
}

// -------------------------------------------------------

AOInvTimer::AOInvTimer() : LLEventTimer( (F32)1.0 )
{
}
AOInvTimer::~AOInvTimer()
{
}
BOOL AOInvTimer::tick()
{
	if (!(gSavedSettings.getBOOL("EmeraldAOEnabled"))) return TRUE;
//	cmdline_printchat("fetching Inventory..");
	if(LLStartUp::getStartupState() >= STATE_INVENTORY_SEND)
	{
		if(gInventory.isEverythingFetched())
		{
			cmdline_printchat("Inventory fetched, loading AO.");
			LLFloaterAO::init();
			return TRUE;
		}
	}
	return FALSE;
}
// NC DROP -------------------------------------------------------

class AONoteCardDropTarget : public LLView
{
public:
	AONoteCardDropTarget(const std::string& name, const LLRect& rect, void (*callback)(LLViewerInventoryItem*));
	~AONoteCardDropTarget();

	void doDrop(EDragAndDropType cargo_type, void* cargo_data);

	//
	// LLView functionality
	virtual BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   std::string& tooltip_msg);
protected:
	void	(*mDownCallback)(LLViewerInventoryItem*);
};


AONoteCardDropTarget::AONoteCardDropTarget(const std::string& name, const LLRect& rect,
						  void (*callback)(LLViewerInventoryItem*)) :
	LLView(name, rect, NOT_MOUSE_OPAQUE, FOLLOWS_ALL),
	mDownCallback(callback)
{
}

AONoteCardDropTarget::~AONoteCardDropTarget()
{
}

void AONoteCardDropTarget::doDrop(EDragAndDropType cargo_type, void* cargo_data)
{
//	llinfos << "AONoteCardDropTarget::doDrop()" << llendl;
}

BOOL AONoteCardDropTarget::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data,
									 EAcceptance* accept,
									 std::string& tooltip_msg)
{
	BOOL handled = FALSE;
	if(getParent())
	{
		handled = TRUE;
		LLViewerInventoryItem* inv_item = (LLViewerInventoryItem*)cargo_data;
		if(gInventory.getItem(inv_item->getUUID()))
		{
			*accept = ACCEPT_YES_COPY_SINGLE;
			if(drop)
			{
				mDownCallback(inv_item);
			}
		}
		else
		{
			*accept = ACCEPT_NO;
		}
	}
	return handled;
}

AONoteCardDropTarget * LLFloaterAO::mAOItemDropTarget;


// STUFF -------------------------------------------------------

int LLFloaterAO::mAnimationState = 0;
int LLFloaterAO::stand_iterator = 0;

LLUUID LLFloaterAO::invfolderid = LLUUID::null;
LLUUID LLFloaterAO::mCurrentStandId = LLUUID::null;

struct struct_overrides
{
	LLUUID orig_id;
	LLUUID ao_id;
	int state;
};
std::vector<struct_overrides> mAOOverrides;


struct struct_stands
{
	LLUUID ao_id;
	std::string anim_name;
};
std::vector<struct_stands> mAOStands;

struct struct_tokens
{
	std::string token;
	int state;
};
std::vector<struct_tokens> mAOTokens;

LLFloaterAO* LLFloaterAO::sInstance = NULL;

LLFloaterAO::LLFloaterAO()
:LLFloater(std::string("floater_ao"))
{
//	init();
    LLUICtrlFactory::getInstance()->buildFloater(this, "floater_ao.xml");
	childSetAction("reloadcard",onClickReloadCard,this);
	childSetAction("prevstand",onClickPrevStand,this);
	childSetAction("nextstand",onClickNextStand,this);
	sInstance = this;
}

LLFloaterAO::~LLFloaterAO()
{
    sInstance=NULL;
	delete mAOItemDropTarget;
	mAOItemDropTarget = NULL;
}

BOOL LLFloaterAO::postBuild()
{
	LLView *target_view = getChild<LLView>("ao_notecard");
	if(target_view)
	{
		if (mAOItemDropTarget)
		{
			delete mAOItemDropTarget;
		}
		mAOItemDropTarget = new AONoteCardDropTarget("drop target", target_view->getRect(), AOItemDrop);//, mAvatarID);
		addChild(mAOItemDropTarget);
	}
	if(LLStartUp::getStartupState() == STATE_STARTED)
	{
		LLUUID itemidimport = (LLUUID)gSavedPerAccountSettings.getString("EmeraldAOConfigNotecardID");
		LLViewerInventoryItem* itemimport = gInventory.getItem(itemidimport);
		if(itemimport)
		{
			childSetValue("ao_nc_text","Currently set to: "+itemimport->getName());
		}
		else if(itemidimport.isNull())
		{
			childSetValue("ao_nc_text","Currently not set");
		}
		else
		{
			childSetValue("ao_nc_text","Currently set to a item not on this account");
		}
	}
	else
	{
		childSetValue("ao_nc_text","Not logged in");
	}
	childSetCommitCallback("EmeraldAOEnabled",onClickRun,this);
//	getChild<LLCheckBoxCtrl>("")->setClickedCallback(run, this);
//	childSetValue("EmeraldAOEnabled", gSavedPerAccountSettings.getBOOL("EmeraldAOEnabled"));
//	childSetValue("EmeraldAOSitsEnabled", gSavedPerAccountSettings.getBOOL("EmeraldAOSitsEnabled"));
//	childSetValue("standtime", gSavedPerAccountSettings.getBOOL("EmeraldAOStandInterval"));
	return TRUE;
}

void LLFloaterAO::show(void*)
{
    if (!sInstance)
	sInstance = new LLFloaterAO();
    sInstance->open();
}

void LLFloaterAO::init()
{
	mAOTokens.clear();
	struct_tokens tokenloader;
	tokenloader.token = 
	tokenloader.token = "[ Sitting On Ground ]";	tokenloader.state = STATE_AGENT_GROUNDSIT; mAOTokens.push_back(tokenloader);    // 0
	tokenloader.token = "[ Sitting ]";				tokenloader.state = STATE_AGENT_SIT; mAOTokens.push_back(tokenloader);              // 1
	tokenloader.token = "[ Crouching ]";			tokenloader.state = STATE_AGENT_CROUCH; mAOTokens.push_back(tokenloader);            // 3
	tokenloader.token = "[ Crouch Walking ]";		tokenloader.state = STATE_AGENT_CROUCHWALK; mAOTokens.push_back(tokenloader);       // 4
	tokenloader.token = "[ Standing Up ]";			tokenloader.state = STATE_AGENT_STANDUP; mAOTokens.push_back(tokenloader);          // 6
	tokenloader.token = "[ Falling ]";				tokenloader.state = STATE_AGENT_FALLDOWN; mAOTokens.push_back(tokenloader);              // 7
	tokenloader.token = "[ Flying Down ]";			tokenloader.state = STATE_AGENT_HOVER_DOWN; mAOTokens.push_back(tokenloader);          // 8
	tokenloader.token = "[ Flying Up ]";			tokenloader.state = STATE_AGENT_HOVER_UP; mAOTokens.push_back(tokenloader);            // 9
	tokenloader.token = "[ Flying Slow ]";			tokenloader.state = STATE_AGENT_FLYSLOW; mAOTokens.push_back(tokenloader);          // 10
	tokenloader.token = "[ Flying ]";				tokenloader.state = STATE_AGENT_FLY; mAOTokens.push_back(tokenloader);               // 11
	tokenloader.token = "[ Hovering ]";				tokenloader.state = STATE_AGENT_HOVER; mAOTokens.push_back(tokenloader);             // 12
	tokenloader.token = "[ Jumping ]";				tokenloader.state = STATE_AGENT_JUMP; mAOTokens.push_back(tokenloader);              // 13
	tokenloader.token = "[ Pre Jumping ]";			tokenloader.state = STATE_AGENT_PRE_JUMP; mAOTokens.push_back(tokenloader);          // 14
	tokenloader.token = "[ Running ]";				tokenloader.state = STATE_AGENT_RUN; mAOTokens.push_back(tokenloader);              // 15
	tokenloader.token = "[ Turning Right ]";		tokenloader.state = STATE_AGENT_TURNRIGHT; mAOTokens.push_back(tokenloader);        // 16
	tokenloader.token = "[ Turning Left ]";			tokenloader.state = STATE_AGENT_TURNLEFT; mAOTokens.push_back(tokenloader);         // 17
	tokenloader.token = "[ Walking ]";				tokenloader.state = STATE_AGENT_WALK; mAOTokens.push_back(tokenloader);              // 18
	tokenloader.token = "[ Landing ]";				tokenloader.state = STATE_AGENT_LAND; mAOTokens.push_back(tokenloader);              // 19
	tokenloader.token = "[ Standing ]";				tokenloader.state = STATE_AGENT_STAND; mAOTokens.push_back(tokenloader);             // 20
	tokenloader.token = "[ Swimming Down ]";		tokenloader.state = 999; mAOTokens.push_back(tokenloader);        // 21
	tokenloader.token = "[ Swimming Up ]";			tokenloader.state = 999; mAOTokens.push_back(tokenloader);          // 22
	tokenloader.token = "[ Swimming Forward ]";		tokenloader.state = 999; mAOTokens.push_back(tokenloader);     // 23
	tokenloader.token = "[ Floating ]";				tokenloader.state = 999; mAOTokens.push_back(tokenloader);             // 24


	mAOOverrides.clear();
	struct_overrides overrideloader;
	overrideloader.orig_id = ANIM_AGENT_WALK;					overrideloader.ao_id = LLUUID::null; overrideloader.state = STATE_AGENT_WALK;			mAOOverrides.push_back(overrideloader);
	overrideloader.orig_id = ANIM_AGENT_RUN;					overrideloader.ao_id = LLUUID::null; overrideloader.state = STATE_AGENT_RUN;			mAOOverrides.push_back(overrideloader);
	overrideloader.orig_id = ANIM_AGENT_PRE_JUMP;				overrideloader.ao_id = LLUUID::null; overrideloader.state = STATE_AGENT_PRE_JUMP;		mAOOverrides.push_back(overrideloader);
	overrideloader.orig_id = ANIM_AGENT_JUMP;					overrideloader.ao_id = LLUUID::null; overrideloader.state = STATE_AGENT_JUMP;			mAOOverrides.push_back(overrideloader);
	overrideloader.orig_id = ANIM_AGENT_TURNLEFT;				overrideloader.ao_id = LLUUID::null; overrideloader.state = STATE_AGENT_TURNLEFT;		mAOOverrides.push_back(overrideloader);
	overrideloader.orig_id = ANIM_AGENT_TURNRIGHT;				overrideloader.ao_id = LLUUID::null; overrideloader.state = STATE_AGENT_TURNRIGHT;		mAOOverrides.push_back(overrideloader);

	overrideloader.orig_id = ANIM_AGENT_SIT;					overrideloader.ao_id = LLUUID::null; overrideloader.state = STATE_AGENT_SIT;			mAOOverrides.push_back(overrideloader);
	overrideloader.orig_id = ANIM_AGENT_SIT_FEMALE;				overrideloader.ao_id = LLUUID::null; overrideloader.state = STATE_AGENT_SIT;			mAOOverrides.push_back(overrideloader);
	overrideloader.orig_id = ANIM_AGENT_SIT_GENERIC;			overrideloader.ao_id = LLUUID::null; overrideloader.state = STATE_AGENT_SIT;			mAOOverrides.push_back(overrideloader);
	overrideloader.orig_id = ANIM_AGENT_SIT_GROUND;				overrideloader.ao_id = LLUUID::null; overrideloader.state = STATE_AGENT_GROUNDSIT;		mAOOverrides.push_back(overrideloader);
	overrideloader.orig_id = ANIM_AGENT_SIT_GROUND_CONSTRAINED;	overrideloader.ao_id = LLUUID::null; overrideloader.state = STATE_AGENT_GROUNDSIT;		mAOOverrides.push_back(overrideloader);

	overrideloader.orig_id = ANIM_AGENT_HOVER;					overrideloader.ao_id = LLUUID::null; overrideloader.state = STATE_AGENT_HOVER;			mAOOverrides.push_back(overrideloader);
	overrideloader.orig_id = ANIM_AGENT_HOVER_DOWN;				overrideloader.ao_id = LLUUID::null; overrideloader.state = STATE_AGENT_HOVER_DOWN;		mAOOverrides.push_back(overrideloader);
	overrideloader.orig_id = ANIM_AGENT_HOVER_UP;				overrideloader.ao_id = LLUUID::null; overrideloader.state = STATE_AGENT_HOVER_UP;		mAOOverrides.push_back(overrideloader);

	overrideloader.orig_id = ANIM_AGENT_CROUCH;					overrideloader.ao_id = LLUUID::null; overrideloader.state = STATE_AGENT_CROUCH;			mAOOverrides.push_back(overrideloader);
	overrideloader.orig_id = ANIM_AGENT_CROUCHWALK;				overrideloader.ao_id = LLUUID::null; overrideloader.state = STATE_AGENT_CROUCHWALK;		mAOOverrides.push_back(overrideloader);

	overrideloader.orig_id = ANIM_AGENT_FALLDOWN;				overrideloader.ao_id = LLUUID::null; overrideloader.state = STATE_AGENT_FALLDOWN;		mAOOverrides.push_back(overrideloader);
	overrideloader.orig_id = ANIM_AGENT_STANDUP;				overrideloader.ao_id = LLUUID::null; overrideloader.state = STATE_AGENT_STANDUP;		mAOOverrides.push_back(overrideloader);
	overrideloader.orig_id = ANIM_AGENT_LAND;					overrideloader.ao_id = LLUUID::null; overrideloader.state = STATE_AGENT_LAND;			mAOOverrides.push_back(overrideloader);

	overrideloader.orig_id = ANIM_AGENT_FLY;					overrideloader.ao_id = LLUUID::null; overrideloader.state = STATE_AGENT_FLY;			mAOOverrides.push_back(overrideloader);
	overrideloader.orig_id = ANIM_AGENT_FLYSLOW;				overrideloader.ao_id = LLUUID::null; overrideloader.state = STATE_AGENT_FLYSLOW;		mAOOverrides.push_back(overrideloader);

	LLUUID configncitem = (LLUUID)gSavedPerAccountSettings.getString("EmeraldAOConfigNotecardID");
	BOOL success = FALSE;
	if (configncitem.notNull())
	{
		const LLInventoryItem* item = gInventory.getItem(configncitem);
		if(item)
		{
			if (gAgent.allowOperation(PERM_COPY, item->getPermissions(),GP_OBJECT_MANIPULATE) || gAgent.isGodlike())
			{
				if(!item->getAssetUUID().isNull())
				{
					LLUUID* new_uuid = new LLUUID(configncitem);
					LLHost source_sim = LLHost::invalid;
					invfolderid = item->getParentUUID();
					gAssetStorage->getInvItemAsset(source_sim,
													gAgent.getID(),
													gAgent.getSessionID(),
													item->getPermissions().getOwner(),
													LLUUID::null,
													item->getUUID(),
													item->getAssetUUID(),
													item->getType(),
													&onNotecardLoadComplete,
													(void*)new_uuid,
													TRUE);
					success = TRUE;
				}
			}
		}
	}

	if (!success)
	{
		cmdline_printchat("Could not read the specified Config Notecard");
	}

	mAnimationState = 0;
	mCurrentStandId = LLUUID::null;
	setAnimationState(STATE_AGENT_IDLE);

}

void LLFloaterAO::onClickRun(LLUICtrl *, void*)
{
	run();
}

void LLFloaterAO::run()
{
	if (gSavedSettings.getBOOL("EmeraldAOEnabled"))
	{
		new AOStandTimer();
	}
	else
	{
		stopMotion(getCurrentStandId(), FALSE, TRUE); //stop stand first then set state
		setAnimationState(STATE_AGENT_IDLE);
	}
//	return TRUE;
}

int LLFloaterAO::getAnimationState()
{
	return mAnimationState;
}

void LLFloaterAO::setAnimationState(const int state)
{
	mAnimationState = state;
}

LLUUID LLFloaterAO::getCurrentStandId()
{
	return mCurrentStandId;
}

void LLFloaterAO::setCurrentStandId(const LLUUID& id)
{
	mCurrentStandId = id;
}

void LLFloaterAO::AOItemDrop(LLViewerInventoryItem* item)
{
	gSavedPerAccountSettings.setString("EmeraldAOConfigNotecardID", item->getUUID().asString());
	sInstance->childSetValue("ao_nc_text","Currently set to: "+item->getName());
}

LLUUID LLFloaterAO::GetAnimID(const LLUUID& id)
{
	for (std::vector<struct_overrides>::iterator iter = mAOOverrides.begin(); iter != mAOOverrides.end(); ++iter)
	{
		if (iter->orig_id == id) return iter->ao_id;
	}
	return LLUUID::null;
}

int LLFloaterAO::GetStateFromAnimID(const LLUUID& id)
{
	for (std::vector<struct_overrides>::iterator iter = mAOOverrides.begin(); iter != mAOOverrides.end(); ++iter)
	{
		if (iter->orig_id == id) return iter->state;
	}
	return STATE_AGENT_IDLE;
}

int LLFloaterAO::GetStateFromToken(std::string strtoken)
{
	for (std::vector<struct_tokens>::iterator iter = mAOTokens.begin(); iter != mAOTokens.end(); ++iter)
	{
		if (iter->token == strtoken) return iter->state;
	}
	return STATE_AGENT_IDLE;
}

void LLFloaterAO::onClickPrevStand(void* user_data)
{
	if (!(mAOStands.size() > 0)) return;
	stand_iterator=stand_iterator-1;
	if (stand_iterator < 0) stand_iterator = int( mAOStands.size()-stand_iterator);
	if (stand_iterator > int( mAOStands.size()-1)) stand_iterator = 0;
	cmdline_printchat(llformat("Changing stand to %s.",mAOStands[stand_iterator].anim_name.c_str()));
	ChangeStand();
}

void LLFloaterAO::onClickNextStand(void* user_data)
{
	if (!(mAOStands.size() > 0)) return;
	stand_iterator=stand_iterator+1;
	if (stand_iterator < 0) stand_iterator = int( mAOStands.size()-stand_iterator);
	if (stand_iterator > int( mAOStands.size()-1)) stand_iterator = 0;
	cmdline_printchat(llformat("Changing stand to %s.",mAOStands[stand_iterator].anim_name.c_str()));
	ChangeStand();
}

BOOL LLFloaterAO::ChangeStand()
{
	if (gSavedSettings.getBOOL("EmeraldAOEnabled"))
	{
//		llinfos << "tick sit getAnimationState() " << getAnimationState() << llendl;
		if (gAgent.getAvatarObject())
		{
			if (gAgent.getAvatarObject()->mIsSitting)
			{
				stopMotion(getCurrentStandId(), FALSE, TRUE); //stop stand first then set state
				setAnimationState(STATE_AGENT_SIT);
				setCurrentStandId(LLUUID::null);
				return FALSE;
			}
		}
		if ((getAnimationState() == STATE_AGENT_IDLE) || (getAnimationState() == STATE_AGENT_STAND))// stands have lowest priority
		{
			if (!(mAOStands.size() > 0)) return TRUE;
			if (stand_iterator < 0) stand_iterator = int( mAOStands.size()-stand_iterator);
			if (stand_iterator > int( mAOStands.size()-1)) stand_iterator = 0;

			int stand_iterator_previous = stand_iterator -1;

			if (stand_iterator_previous < 0) stand_iterator_previous = int( mAOStands.size()-1);
			
			if (mAOStands[stand_iterator].ao_id.notNull())
			{
				stopMotion(getCurrentStandId(), FALSE, TRUE); //stop stand first then set state
				startMotion(mAOStands[stand_iterator].ao_id, 0, TRUE);

				setAnimationState(STATE_AGENT_STAND);
				setCurrentStandId(mAOStands[stand_iterator].ao_id);

//				llinfos << "changing stand to " << mAOStands[stand_iterator].anim_name << llendl;
				return FALSE;
			}
		}
	} 
	else
	{
		stopMotion(getCurrentStandId(), FALSE, TRUE);
		return TRUE; //stop if ao is off
	}
	return TRUE;
}


BOOL LLFloaterAO::startMotion(const LLUUID& id, F32 time_offset, BOOL stand)
{
	if (stand)
	{
		if (id.notNull())
		{
			BOOL sitting = FALSE;
			if (gAgent.getAvatarObject())
			{
				sitting = gAgent.getAvatarObject()->mIsSitting;
			}
			if (sitting) return FALSE;
			gAgent.sendAnimationRequest(id, ANIM_REQUEST_START);
			return TRUE;
		}
	}
	else
	{
		if (GetAnimID(id).notNull() && gSavedSettings.getBOOL("EmeraldAOEnabled"))
		{
			stopMotion(getCurrentStandId(), FALSE, TRUE); //stop stand first then set state 
			setAnimationState(GetStateFromAnimID(id));
		
//			llinfos << " state " << getAnimationState() << " start anim " << id << " overriding with " << GetAnimID(id) << llendl;
			if ((GetStateFromAnimID(id) == STATE_AGENT_SIT) && !(gSavedSettings.getBOOL("EmeraldAOSitsEnabled"))) return TRUE;
			gAgent.sendAnimationRequest(GetAnimID(id), ANIM_REQUEST_START);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL LLFloaterAO::stopMotion(const LLUUID& id, BOOL stop_immediate, BOOL stand)
{	
	if (stand)
	{
		setAnimationState(STATE_AGENT_IDLE);
		gAgent.sendAnimationRequest(id, ANIM_REQUEST_STOP);
		return TRUE;
	}
	else
	{
		if (GetAnimID(id).notNull() && gSavedSettings.getBOOL("EmeraldAOEnabled"))
		{
//			llinfos << "  state " << getAnimationState() << "/" << GetStateFromAnimID(id) << "(now 0)  stop anim " << id << " overriding with " << GetAnimID(id) << llendl;
			if (getAnimationState() == GetStateFromAnimID(id))
			{
				setAnimationState(STATE_AGENT_IDLE); // if anim id doesnt match the state means we pressed another key in the meantime, so dont reset the state, it should be correct
			}
			startMotion(getCurrentStandId(), 0, TRUE);
			gAgent.sendAnimationRequest(GetAnimID(id), ANIM_REQUEST_STOP);
			return TRUE;
		}
	}
	return FALSE;
}

void LLFloaterAO::onClickReloadCard(void* user_data)
{
//	cmdline_printchat("fetching inventory");
	if(gInventory.isEverythingFetched())
	{
//		cmdline_printchat("inventory fetched, loading ao");
		LLFloaterAO::init();
	}
//	init();
}

struct AOAssetInfo
{
	std::string path;
	std::string name;
};

void LLFloaterAO::onNotecardLoadComplete(LLVFS *vfs,const LLUUID& asset_uuid,LLAssetType::EType type,void* user_data, S32 status, LLExtStat ext_status)
{
	if(status == LL_ERR_NOERR)
	{
		S32 size = vfs->getSize(asset_uuid, type);
		U8* buffer = new U8[size];
		vfs->getData(asset_uuid, type, buffer, 0, size);

		if(type == LLAssetType::AT_NOTECARD)
		{
			LLViewerTextEditor* edit = new LLViewerTextEditor("",LLRect(0,0,0,0),S32_MAX,"");
			if(edit->importBuffer((char*)buffer, (S32)size))
			{
				llinfos << "ao nc decode success" << llendl;
				std::string card = edit->getText();
				edit->die();

				mAOStands.clear();
				struct_stands loader;

//				if (card.find("@gemini_uuid_ao") != card.npos ) llinfos << "@gemini_uuid_ao found" << llendl;

				typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
				boost::char_separator<char> sep("\n");
				tokenizer tokline(card, sep);

				for (tokenizer::iterator line = tokline.begin(); line != tokline.end(); ++line)
				{
//					llinfos << *line << llendl;
					std::string strline(*line);
//					llinfos << "uncommented line: " << strline << llendl;

					boost::regex type("^(\\s*)(\\[ )(.*)( \\])");
					boost::smatch what; 
					if (boost::regex_search(strline, what, type)) 
					{
//						llinfos << "type: " << what[0] << llendl;
//						llinfos << "anims in type: " << boost::regex_replace(strline, type, "") << llendl;

						boost::char_separator<char> sep("|,");
						std::string stranimnames(boost::regex_replace(strline, type, ""));
						tokenizer tokanimnames(stranimnames, sep);
						for (tokenizer::iterator anim = tokanimnames.begin(); anim != tokanimnames.end(); ++anim)
						{
							std::string strtoken(what[0]);
							std::string stranim(*anim);
							LLUUID animid(getAssetIDByName(stranim));

//							llinfos << invfolderid.asString().c_str() << llendl;
//							llinfos << "anim: " << stranim.c_str() << " assetid: " << animid << llendl;
							if (!(animid.notNull()))
							{
								cmdline_printchat(llformat("Warning: animation '%s' could not be found (Section: %s).",stranim.c_str(),strtoken.c_str()));
							}
							else
							{
								if (GetStateFromToken(strtoken.c_str()) == STATE_AGENT_STAND)
								{
									loader.ao_id = animid; loader.anim_name = stranim.c_str(); mAOStands.push_back(loader);
								}
								else
								{
//									loader.ao_id = animid; loader.anim_name = stranim.c_str(); mAOStands.push_back(loader);
									for (std::vector<struct_overrides>::iterator iter = mAOOverrides.begin(); iter != mAOOverrides.end(); ++iter)
									{
										if (GetStateFromToken(strtoken.c_str()) == iter->state)
										{
											iter->ao_id = animid;
										}
									}
								}
							}
						}
					} 
				}
				run(); // start overriding, stands etc..
			}
			else
			{
				llinfos << "ao nc decode error" << llendl;
			}
		}
	}
	else
	{
		llinfos << "ao nc read error" << llendl;
	}
}

class ObjectNameMatches : public LLInventoryCollectFunctor
{
public:
	ObjectNameMatches(std::string name)
	{
		sName = name;
	}
	virtual ~ObjectNameMatches() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item)
	{
		if(item)
		{
			if (item->getParentUUID() == LLFloaterAO::invfolderid)
			{
				return (item->getName() == sName);
			}
			return false;
		}
		return false;
	}
private:
	std::string sName;
};

const LLUUID& LLFloaterAO::getAssetIDByName(const std::string& name)
{
	if (name.empty() || !(gInventory.isEverythingFetched())) return LLUUID::null;

	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	ObjectNameMatches objectnamematches(name);
	gInventory.collectDescendentsIf(LLUUID::null,cats,items,FALSE,objectnamematches);

	if (items.count())
	{
		return items[0]->getAssetUUID();
	}
	return LLUUID::null;
};
