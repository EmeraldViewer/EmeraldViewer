

#include "llfloater.h"
#include "llfloaterreporter.h"
#include "lluuid.h"
#include "lltimer.h"
#include "llchat.h"
#include "llscrolllistctrl.h"

#include "llviewerregion.h"



class FloaterAvatarList : public LLFloater, public LLEventTimer
{

private:
	FloaterAvatarList();
public:
	~FloaterAvatarList();

	/*virtual*/ void onClose(bool app_quitting);
	/*virtual*/ void onOpen();
	/*virtual*/ BOOL postBuild();

	BOOL tick();

	private:
	static FloaterAvatarList* sInstance;

public:
	static FloaterAvatarList* getInstance(){ return sInstance; }
	static void lookAtAvatar(LLUUID &uuid);

	static void toggle();

	static void showInstance(){ if(getInstance() == NULL || getInstance()->getVisible() == FALSE)toggle(); }

	static void processAvatarPropertiesReply(LLMessageSystem *msg, void**);

	static void processSoundTrigger(LLMessageSystem *msg, void**);

	std::string getSelectedNames(const std::string& separator = ", ");
	std::string getSelectedName();
	LLUUID getSelectedID();


private:
	
	enum AVATARS_COLUMN_ORDER
	{
		LIST_AVATAR_NAME,
		LIST_DISTANCE,
		LIST_AGE,
		LIST_SIM,
		LIST_PAYMENT,
		LIST_TIME,
		LIST_CLIENT
	};

	typedef void (*avlist_command_t)(LLUUID &avatar);
	static void doCommand(avlist_command_t cmd);

	static void onClickProfile(void *userdata);
	static void onClickIM(void *userdata);
	static void onClickTeleportOffer(void *userdata);
	static void onClickTrack(void *userdata);

	static void onClickGetKey(void *userdata);

	static void onDoubleClick(void *userdata);

	static void onClickFreeze(void *userdata);
	static void onClickEject(void *userdata);
	static void onClickBan(void *userdata);
	static void onClickUnban(void *userdata);
	static void onClickMute(void *userdata);
	static void onClickUnmute(void *userdata);
	static void onClickAR(void *userdata);
	static void onClickTeleport(void *userdata);
	static void onClickKickFromEstate(void *userdata);
	static void onClickBanFromEstate(void *userdata);
	static void onClickTPHFromEstate(void *userdata);
	static void onClickGTFOFromEstate(void *userdata);
	static void onClickScriptCount(void *userdata);

	static void callbackFreeze(const LLSD& notification, const LLSD& response);

	// tracking data
	BOOL mTracking;             // tracking?
	BOOL mTrackByLocation;      // TRUE if tracking by known position, FALSE for tracking a friend
	LLUUID mTrackedAvatar;     // who we're tracking

private:

	LLScrollListCtrl*			mAvatarList;

	enum ACCOUNT_TYPE
	{
		ACCOUNT_RESIDENT,         /** Normal resident */
		ACCOUNT_TRIAL,            /** Trial account */
		ACCOUNT_CHARTER_MEMBER,   /** Lifetime account obtained during beta */
		ACCOUNT_EMPLOYEE,         /** Linden Lab employee */
		ACCOUNT_CUSTOM            /** Custom account title specified. Seems to apply to Philip Linden */
	};

	/**
	 * @brief Payment data
	 */
	enum PAYMENT_TYPE
	{
		PAYMENT_NONE,             /** No payment data on file */
		PAYMENT_ON_FILE,          /** Payment data filled, but not used */
		PAYMENT_USED,             /** Payment data used */
		PAYMENT_LINDEN            /** Payment info doesn't apply (Linden, etc) */
	};

	struct avatar_entry
	{
		LLUUID id;
		std::string name;
		bool is_linden;
		std::string birthdate;
		U32 account_age;
		bool has_info;
		struct agent_info_struct
		{
			std::string caption;
			U8 caption_index;
			U8 payment;
		};
		agent_info_struct agent_info;
		F32 last_info_req;
		bool info_requested(){ return last_info_req != 0.0f; }
		F32 request_timeout;
		bool age_alerted;

		LLVector3d position;

		bool same_region;

		int position_state;

		U32 last_update_frame;

		LLFrameTimer last_update_sec;
	
		time_t sight_time;

		LLFrameTimer lifetime;

		bool last_in_sim;
		bool last_in_draw;
		bool last_in_chat;

		avatar_entry()
		{
			last_in_sim = false;
			last_in_draw = false;
			last_in_chat = false;
			request_timeout = 16.0f;
			has_info = false;
			age_alerted = false;
			last_update_frame = 0;
			last_update_sec.start();
			lifetime.start();
		}

	};

	static void chat_alerts(avatar_entry *entry, bool same_region, bool in_draw, bool in_chat);

	std::map<LLUUID, avatar_entry>	mAvatars;

	LLFrameTimer last_av_req;

public:
	std::map<LLViewerRegion*, std::set<LLUUID> >	mEstateMemoryBans;
};
