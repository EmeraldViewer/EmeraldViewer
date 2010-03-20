/* Local Asset Browser: header 

tag: vaa emerald local_asset_browser

*/


#ifndef VAA_LOCALBROWSER
#define VAA_LOCALBROWSER

#include "llfloater.h"
#include "llscrolllistctrl.h"
#include "lltexturectrl.h"
#include "lldrawable.h"


/*=======================================*/
/*  Global structs / enums / defines     */
/*=======================================*/ 

#define LLF_FLOATER_EXPAND_WIDTH 735
#define LLF_FLOATER_CONTRACT_WIDTH 415
#define LLF_FLOATER_HEIGHT 260

#define LOCAL_USE_MIPMAPS true
#define LOCAL_DISCARD_LEVEL 0
#define NO_IMAGE LLUUID::null

#define TIMER_HEARTBEAT 3.0

#define SLAM_FOR_DEBUG true

enum bitmaplist_cols
{
	BITMAPLIST_COL_NAME,
	BITMAPLIST_COL_ID
};

/* texture picker defines */

#define LOCAL_TEXTURE_PICKER_NAME "texture picker"
#define LOCAL_TEXTURE_PICKER_LIST_NAME "local_name_list"
#define LOCAL_TEXTURE_PICKER_RECURSE true
#define LOCAL_TEXTURE_PICKER_CREATEIFMISSING true

/*=======================================*/
/*  LLLocalBitmapUnit: unit class        */
/*=======================================*/ 
/*
	Product of a complete rewrite of this
	system.
	
	The idea is to have each unit manage
	itself as much as possible.
*/

class LLLocalBitmap
{
	public:
		LLLocalBitmap(std::string filename);
		virtual ~LLLocalBitmap(void);
		friend class LLLocalAssetBrowser;

	public: /* [enums, typedefs, etc] */
		enum link_status
		{
			LINK_UNKNOWN, /* default fallback */
			LINK_ON,
			LINK_OFF,
			LINK_BROKEN,
			LINK_UPDATING /* currently redundant, but left in case necessary later. */
		};

		enum extension_type
		{
			IMG_EXTEN_BMP,
			IMG_EXTEN_TGA,
			IMG_EXTEN_JPG,
			IMG_EXTEN_PNG
		};

		enum bitmap_type
		{
			TYPE_TEXTURE = 0,
			TYPE_SCULPT = 1,
			TYPE_LAYER = 2
		};

	public: /* [information query functions] */
		std::string getShortName(void);
		std::string getFileName(void);
		LLUUID      getID(void);
		LLSD        getLastModified(void);
		std::string getLinkStatus(void);
		bool        getUpdateBool(void);
		void        setType( S32 );
		bool        getIfValidBool(void);
		S32		    getType(void);
		void        getDebugInfo(void);

	private: /* [maintenence functions] */
		void updateSelf(void);
		bool decodeSelf(void);
		void setUpdateBool(void);
		LLLocalBitmap* getThis(void);

	protected: /* [basic properties] */
		std::string    shortname;
		std::string    filename;
		extension_type extension;
		LLUUID         id;
		LLSD           last_modified;
		link_status    linkstatus;
		bool           keep_updating;
		bool           valid;
		LLImageRaw*    rawpointer;
		S32		       bitmap_type;
		bool           sculpt_dirty;
		bool           volume_dirty;
};

/*=======================================*/
/*  LLLocalAssetBrowser: main class      */
/*=======================================*/ 
/*
	Responsible for internal workings.
	Instantiated at the top of the source file.
	Sits in memory until the viewer is closed.

*/


class LLLocalAssetBrowser
{
	public:
		LLLocalAssetBrowser();
		virtual ~LLLocalAssetBrowser();
		friend class LLFloaterLocalAssetBrowser;
		friend class LLLocalAssetBrowserTimer;
		static void UpdateTextureCtrlList(LLScrollListCtrl*);
		static void setLayerUpdated(bool toggle) { mLayerUpdated = toggle; }
		static void setSculptUpdated(bool toggle) { mSculptUpdated = toggle; }

		/* UpdateTextureCtrlList was made public cause texturectrl requests it once on spawn. 
		   i've made it update on spawn instead of on pressing 'local' because the former does it once, 
		   the latter - each time the button's pressed. */

	private:
		static void onChangeHappened(void);
		static bool AddBitmap(std::string);
		static bool DelBitmap(LLUUID);
		static void onUpdateBool(LLUUID);
		static void onSetType(LLUUID, S32);
		static LLLocalBitmap* GetBitmapUnit(LLUUID);
		static bool IsDoingUpdates(void);
		static void PingTimer(void);
		static void PerformTimedActions(void);
		static void PerformSculptUpdates(LLLocalBitmap*);

	protected:
		static  std::vector<LLLocalBitmap> loaded_bitmaps;
		typedef std::vector<LLLocalBitmap>::iterator local_list_iter;
		static  bool    mLayerUpdated;
		static  bool    mSculptUpdated;
};

/*==================================================*/
/*  LLLocalAssetBrowserInterface : interface class  */
/*==================================================*/ 
/*
	Responsible for talking to the user.
	Instantiated by user request.
	Destroyed when the floater is closed.

*/
class LLFloaterLocalAssetBrowser : public LLFloater
{
public:
    LLFloaterLocalAssetBrowser();
    virtual ~LLFloaterLocalAssetBrowser();
    static void show(void*);


 
private: 
	/* Widget related callbacks */
    // Button callback declarations
    static void onClickAdd(void* userdata);
	static void onClickDel(void* userdata);
	static void onClickMore(void* userdata);
	static void onClickLess(void* userdata);
	static void onClickUpload(void* userdata);

	// ScrollList callback declarations
	static void onChooseBitmapList(LLUICtrl* ctrl, void* userdata);

	// Checkbox callback declarations
	static void onClickUpdateChkbox(LLUICtrl* ctrl, void* userdata);

	// Combobox type select
	static void onCommitTypeCombo(LLUICtrl* ctrl, void* userdata);

	// Widgets
	LLButton* mAddBtn;
	LLButton* mDelBtn;
	LLButton* mMoreBtn;
	LLButton* mLessBtn;
	LLButton* mUploadBtn;

	LLScrollListCtrl* mBitmapList;
	LLScrollListCtrl* mUsedList;
	LLTextureCtrl*    mTextureView;
	LLCheckBoxCtrl*   mUpdateChkBox;

	LLLineEditor* mPathTxt;
	LLLineEditor* mUUIDTxt;
	LLLineEditor* mNameTxt;

	LLTextBox*  mLinkTxt;
	LLTextBox*  mTimeTxt;
	LLComboBox* mTypeComboBox;

	LLTextBox* mCaptionPathTxt;
	LLTextBox* mCaptionUUIDTxt;
	LLTextBox* mCaptionLinkTxt;
	LLTextBox* mCaptionNameTxt;
	LLTextBox* mCaptionTimeTxt;

	/* static pointer to self, wai? oh well. */
    static LLFloaterLocalAssetBrowser* sInstance;

	// non-widget functions
	static void FloaterResize(bool expand);
	static void UpdateBitmapScrollList(void);
	static void UpdateRightSide(void);


};

/*==================================================*/
/*     LLLocalAssetBrowserTimer : timer class       */
/*==================================================*/ 
/*
	A small, simple timer class inheriting from
	LLEventTimer, responsible for pinging the
	LLLocalAssetBrowser class to perform it's
	updates / checks / etc.

*/
class LLLocalAssetBrowserTimer : public LLEventTimer
{
	public:
		LLLocalAssetBrowserTimer();
		~LLLocalAssetBrowserTimer();
		virtual BOOL tick();
		void		 start();
		void		 stop();
		bool         isRunning();
};

#endif

