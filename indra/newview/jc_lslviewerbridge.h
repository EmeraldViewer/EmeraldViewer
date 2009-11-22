
#include "llviewerprecompiledheaders.h"

#include "llchatbar.h"

#include "llviewerobject.h"

class JCBridgeCallback : public LLRefCount
{
public:
	virtual void fire(LLSD data) = 0;
};

class JCLSLBridge : public LLEventTimer
{
public:
	JCLSLBridge();
	~JCLSLBridge();
	BOOL tick();
	static bool lsltobridge(std::string message, std::string from_name, LLUUID source_id, LLUUID owner_id);
	static void bridgetolsl(std::string cmd, JCBridgeCallback* cb);
	static S32 bridge_channel(LLUUID user);
	static JCLSLBridge* sInstance;
	static const LLUUID& findInventoryByName(const std::string& object_name);

	static U8 sBridgeStatus;
	//0 = uninitialized
	//1 = not found, building
	//2 = moving to our folder <3
	//3 = finished
	//4 = failed

	enum BridgeStat
	{
		UNINITIALIZED,
		BUILDING,
		RENAMING,
		FOLDERING,
		RECHAN,
		ACTIVE,
		FAILED
	};
	static LLViewerObject* sBridgeObject;
	static void setBridgeObject(LLViewerObject* obj);

	static U32 lastcall;

	static std::map<U32,JCBridgeCallback*> callback_map;

	static void callback_fire(U32 callback_id, LLSD data);
	static U32 registerCB(JCBridgeCallback* cb);

	static LLSD parse_string_to_list(std::string list, char sep);

	static void processSoundTrigger(LLMessageSystem *msg, void**);
/*
	static void loadPermissions();
	static void storePermissions();

	static std::string RECHAN_permissions;
*/
	static void processAvatarNotesReply(LLMessageSystem *msg, void**);

	static S32 l2c;
	static bool l2c_inuse;
};
/*
#ifndef LOLPERM
#define LOLPERM
std::string permission_characters("!@#$%^&*()_+=-0987654321");
enum PermissionTypes
{
	PERMISSION_CONTROL_AVATAR,
	PERMISSION_CAPTURE_MOUSE,
	PERMISSION_CHANGE_WORLD,
	PERMISSION_CHANGE_UI,
	PERMISSION_GET_STATS,
	PERMISSION_OVERRIDE_MEDIA
};
U32 Perm2Int(std::string cha){ return permission_characters.find(cha); };

std::string Perm2Str(U32 cha){ return permission_characters.substr(cha,cha); };
#endif*/