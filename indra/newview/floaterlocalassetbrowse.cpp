/* Local Asset Browser: source */
/*

tag: vaa emerald local_asset_browser

other changed files:
	llviewermenu.cpp, menu_viewer.xml
	lltexturectrl.h, lltexturectrl.cpp, floater_texture_ctrl.xml
	pipeline.cpp, pipeline.h
	llvovolume.h

*/

/* basic headers */
#include "llviewerprecompiledheaders.h"
#include "lluictrlfactory.h"

/* boost madness from hell */
#ifdef equivalent
	#undef equivalent
#endif
#include <boost/filesystem.hpp>

/* own class header */
#include "floaterlocalassetbrowse.h"

/* image compression headers. */
#include "llimagebmp.h"
#include "llimagetga.h"
#include "llimagejpeg.h"
#include "llimagepng.h"

/* misc headers */
#include <time.h>
#include <ctime>
#include "llviewerimagelist.h"  // gImageList
#include "llviewerobjectlist.h" // gObjectList
#include "llfilepicker.h"
#include "llviewermenufile.h"
#include "llfloaterimagepreview.h"

/* repeated in header */
#include "lltexturectrl.h"   
#include "llscrolllistctrl.h"

/* including to force rebakes when needed */
#include "llagent.h"
#include "llvoavatar.h"

/* sculpt refresh */
#include "llvovolume.h"


/*=======================================*/
/*     Instantiating manager class       */
/*    and formally declaring it's list   */
/*=======================================*/ 
LocalAssetBrowser* gLocalBrowser;
LocalAssetBrowserTimer* gLocalBrowserTimer;
std::vector<LocalBitmap> LocalAssetBrowser::loaded_bitmaps;
bool    LocalAssetBrowser::mLayerUpdated;
bool    LocalAssetBrowser::mSculptUpdated;

/*=======================================*/
/*  LocalBitmap: unit class              */
/*=======================================*/ 
/*
	Product of a complete rewrite of this
	system.
	
	The idea is to have each unit manage
	itself as much as possible.
*/

LocalBitmap::LocalBitmap(std::string fullpath)
{
	this->valid = false;
	if ( gDirUtilp->fileExists(fullpath) )
	{
		/* taking care of basic properties */
		this->id.generate();
		this->filename	    = fullpath;
		this->linkstatus    = LINK_OFF;
		this->keep_updating = false;
		this->shortname     = gDirUtilp->getBaseFileName(this->filename, true);
		this->bitmap_type   = TYPE_TEXTURE;
		this->sculpt_dirty  = false;
		this->volume_dirty  = false;
		this->valid         = false;

		/* taking care of extension type now to avoid switch madness */
		std::string temp_exten = gDirUtilp->getExtension(this->filename);

		if (temp_exten == "bmp") { this->extension = IMG_EXTEN_BMP; }
		else if (temp_exten == "tga") { this->extension = IMG_EXTEN_TGA; }
		else if (temp_exten == "jpg") { this->extension = IMG_EXTEN_JPG; }
		else if (temp_exten == "png") { this->extension = IMG_EXTEN_PNG; }
		else { return; } // no valid extension.
		
		/* getting file's last modified */
		const std::time_t time = boost::filesystem::last_write_time( boost::filesystem::path( this->filename ) );
		this->last_modified = asctime( localtime(&time) );
		
		/* checking if the bitmap is valid && decoding if it is */
		LLImageRaw* raw_image = new LLImageRaw();
		if ( this->decodeSelf(raw_image) )
		{
			/* creating a shell LLViewerImage and fusing raw image into it */
			LLViewerImage* viewer_image = new LLViewerImage( "file://"+this->filename, this->id, LOCAL_USE_MIPMAPS );
			viewer_image->createGLTexture( LOCAL_DISCARD_LEVEL, raw_image );
			viewer_image->mCachedRawImage = raw_image;

			/* making damn sure gImageList will not delete it prematurely */
			viewer_image->ref(); 

			/* finalizing by adding LLViewerImage instance into gImageList */
			gImageList.addImage(viewer_image);

			/* filename is valid, bitmap is decoded and valid, i can haz liftoff! */
			this->valid = true;
		}
	}
}

LocalBitmap::~LocalBitmap()
{

}

/* [maintenence functions] */
void LocalBitmap::updateSelf()
{
	if ( this->linkstatus == LINK_ON )
	{
		/* making sure file still exists */
		if ( !gDirUtilp->fileExists(this->filename) ) { this->linkstatus = LINK_BROKEN; return; }

		/* exists, let's check if it's lastmod has changed */
		const std::time_t temp_time = boost::filesystem::last_write_time( boost::filesystem::path( this->filename ) );
		LLSD new_last_modified = asctime( localtime(&temp_time) );
		if ( this->last_modified.asString() == new_last_modified.asString() ) { return; }

		/* here we update the image */
		LLImageRaw* new_imgraw = new LLImageRaw();
		if ( !decodeSelf(new_imgraw) ) { this->linkstatus = LINK_BROKEN; return; }
		LLViewerImage* image = gImageList.hasImage(this->id);
		
		if (!image->mForSculpt) 
		    { image->createGLTexture( LOCAL_DISCARD_LEVEL, new_imgraw ); }
		else
		    { image->mCachedRawImage = new_imgraw; }

		/* finalizing by updating lastmod to current */
		this->last_modified = new_last_modified;

		switch (this->bitmap_type)
		{
			case TYPE_TEXTURE:
				  { break; }

			case TYPE_SCULPT:
				  {
					  /* sets a bool to run through all visible sculpts in one go, and update the ones necessary. */
					  this->sculpt_dirty = true;
					  this->volume_dirty = true;
					  gLocalBrowser->setSculptUpdated( true );
					  break;
				  }

			case TYPE_LAYER:
				  {
					  /* sets a bool to rebake layers after the iteration is done with */
					  gLocalBrowser->setLayerUpdated( true );
					  break;
				  }

			default:
				  { break; }

		}
	}

}

bool LocalBitmap::decodeSelf(LLImageRaw* rawimg)
{
	switch (this->extension)
	{
		case IMG_EXTEN_BMP:
			{
				LLPointer<LLImageBMP> bmp_image = new LLImageBMP;
				if ( !bmp_image->load(filename) ) { break; }
				if ( !bmp_image->decode(rawimg, 0.0f) ) { break; }

				rawimg->biasedScaleToPowerOfTwo( LLViewerImage::MAX_IMAGE_SIZE_DEFAULT );
				return true;
			}

		case IMG_EXTEN_TGA:
			{
				LLPointer<LLImageTGA> tga_image = new LLImageTGA;
				if ( !tga_image->load(filename) ) { break; }
				if ( !tga_image->decode(rawimg) ) { break; }

				if(	( tga_image->getComponents() != 3) &&
					( tga_image->getComponents() != 4) ) { break; }

				rawimg->biasedScaleToPowerOfTwo( LLViewerImage::MAX_IMAGE_SIZE_DEFAULT );
				return true;
			}

		case IMG_EXTEN_JPG:
			{
				LLPointer<LLImageJPEG> jpeg_image = new LLImageJPEG;
				if ( !jpeg_image->load(filename) ) { break; }
				if ( !jpeg_image->decode(rawimg, 0.0f) ) { break; }

				rawimg->biasedScaleToPowerOfTwo( LLViewerImage::MAX_IMAGE_SIZE_DEFAULT );
				return true;
			}

		case IMG_EXTEN_PNG:
			{
				LLPointer<LLImagePNG> png_image = new LLImagePNG;
				if ( !png_image->load(filename) ) { break; }
				if ( !png_image->decode(rawimg, 0.0f) ) { break; }

				rawimg->biasedScaleToPowerOfTwo( LLViewerImage::MAX_IMAGE_SIZE_DEFAULT );
				return true;
			}

		default:
			break;
	}
	return false;
}

void LocalBitmap::setUpdateBool()
{
	if ( this->linkstatus != LINK_BROKEN )
	{
		if ( !this->keep_updating )
		{
			this->linkstatus = LINK_ON;
			this->keep_updating = true;
		}
		else
		{
			this->linkstatus = LINK_OFF;
			this->keep_updating = false;
		}
	}
	else
	{
		this->keep_updating = false;
	}
}

void LocalBitmap::setType( S32 type )
{
	this->bitmap_type = type;
}

/* [information query functions] */
std::string LocalBitmap::getShortName()
{
	return this->shortname;
}

std::string LocalBitmap::getFileName()
{
	return this->filename;
}

LLUUID LocalBitmap::getID()
{
	return this->id;
}

LLSD LocalBitmap::getLastModified()
{
	return this->last_modified;
}

std::string LocalBitmap::getLinkStatus()
{
	switch(this->linkstatus)
	{
		case LINK_ON:
			return "On";

		case LINK_OFF:
			return "Off";

		case LINK_BROKEN:
			return "Broken";

		case LINK_UPDATING:
			return "Updating";

		default:
			return "Unknown";
	}
}

bool LocalBitmap::getUpdateBool()
{
	return this->keep_updating;
}

bool LocalBitmap::getIfValidBool()
{
	return this->valid;
}

LocalBitmap* LocalBitmap::getThis()
{
	return this;
}

S32 LocalBitmap::getType()
{
	return this->bitmap_type;
}

void LocalBitmap::getDebugInfo()
{
	/* debug function: dumps everything human readable into llinfos */
	llinfos << "===[local bitmap debug]==="               << llendl;
	llinfos << "path: "          << this->filename        << llendl;
	llinfos << "name: "          << this->shortname       << llendl;
	llinfos << "extension: "     << this->extension       << llendl;
	llinfos << "uuid: "          << this->id              << llendl;
	llinfos << "last modified: " << this->last_modified   << llendl;
	llinfos << "link status: "   << this->getLinkStatus() << llendl;
	llinfos << "keep updated: "  << this->keep_updating   << llendl;
	llinfos << "type: "          << this->bitmap_type     << llendl;
	llinfos << "is valid: "      << this->valid           << llendl;
	llinfos << "=========================="               << llendl;
	
}

/*=======================================*/
/*  LocalAssetBrowser: internal class  */
/*=======================================*/ 
/*
	Responsible for internal workings.
	Instantiated at the top of the source file.
	Sits in memory until the viewer is closed.
*/

LocalAssetBrowser::LocalAssetBrowser()
{
	this->mLayerUpdated = false;
	this->mSculptUpdated = false;
}

LocalAssetBrowser::~LocalAssetBrowser()
{

}

bool LocalAssetBrowser::AddBitmap(std::string filename)
{

	LocalBitmap* unit = new LocalBitmap( filename );

	if	( unit->getIfValidBool() )
	{
		loaded_bitmaps.push_back( *unit );
		onChangeHappened();
		return true;
	}

	return false;
}

bool LocalAssetBrowser::DelBitmap(LLUUID id)
{
	local_list_iter iter = loaded_bitmaps.begin();
	for (; iter != loaded_bitmaps.end(); iter++)
	{
		if ( iter->getID() == id )
		{
			gImageList.deleteImage( gImageList.hasImage(id) );
			loaded_bitmaps.erase(iter);
			onChangeHappened();
			return true;
		}
	}

	return false;
}

void LocalAssetBrowser::onUpdateBool(LLUUID id)
{
	LocalBitmap* unit = GetBitmapUnit( id );
	if ( unit ) 
	{ 
		unit->setUpdateBool(); 
		PingTimer();
	}
}

void LocalAssetBrowser::onSetType(LLUUID id, S32 type)
{
	LocalBitmap* unit = GetBitmapUnit( id );
	if ( unit ) 
	{ unit->setType(type); }
}

LocalBitmap* LocalAssetBrowser::GetBitmapUnit(LLUUID id)
{
	local_list_iter iter = loaded_bitmaps.begin();
	for (; iter != loaded_bitmaps.end(); iter++)
	{ 
		if ( iter->getID() == id ) 
		{
			return iter->getThis();
		} 
	}
	
	return NULL;
}

bool LocalAssetBrowser::IsDoingUpdates()
{
	local_list_iter iter = loaded_bitmaps.begin();
	for (; iter != loaded_bitmaps.end(); iter++)
	{ 
		if ( iter->getUpdateBool() ) 
		{ return true; } /* if at least one unit in the list needs updates - we need a timer. */
	}

	return false;
}


/* Reaction to a change in bitmaplist, this function finds a texture picker floater's appropriate scrolllist
   and passes this scrolllist's pointer to UpdateTextureCtrlList for processing.
   it also processes timer start/stops as needed */
void LocalAssetBrowser::onChangeHappened()
{

	/* texturepicker related */
	const LLView::child_list_t* child_list = gFloaterView->getChildList();
	LLView::child_list_const_iter_t child_list_iter = child_list->begin();

	for (; child_list_iter != child_list->end(); child_list_iter++)
	{
		LLView* view = *child_list_iter;
		if ( view->getName() == LOCAL_TEXTURE_PICKER_NAME )
		{
			LLScrollListCtrl* ctrl = view->getChild<LLScrollListCtrl>
									( LOCAL_TEXTURE_PICKER_LIST_NAME, 
									  LOCAL_TEXTURE_PICKER_RECURSE, 
									  LOCAL_TEXTURE_PICKER_CREATEIFMISSING );

			if ( ctrl ) { UpdateTextureCtrlList(ctrl); }
		}
	}

	/* poking timer to see if it's still needed/still not needed */
	PingTimer();

}

void LocalAssetBrowser::PingTimer()
{
	if ( !loaded_bitmaps.empty() && IsDoingUpdates() )
	{
		if (!gLocalBrowserTimer) 
		{ gLocalBrowserTimer = new LocalAssetBrowserTimer(); }
		
		if ( !gLocalBrowserTimer->isRunning() )
		{ gLocalBrowserTimer->start(); }
	}

	else
	{
		if (gLocalBrowserTimer)
		{
			if ( gLocalBrowserTimer->isRunning() ) 
			{ gLocalBrowserTimer->stop(); } 
		}
	}
}

/* This function refills the texture picker floater's scrolllist with the updated contents of bitmaplist */
void LocalAssetBrowser::UpdateTextureCtrlList(LLScrollListCtrl* ctrl)
{
	if ( ctrl ) // checking again in case called externally for some silly reason.
	{
		ctrl->clearRows(); 
		if ( !loaded_bitmaps.empty() )
		{
			local_list_iter iter = loaded_bitmaps.begin();
			for ( ; iter != loaded_bitmaps.end(); iter++ )
			{
				LLSD element;
				element["columns"][0]["column"] = "unit_name";
				element["columns"][0]["type"] = "text";
				element["columns"][0]["value"] = iter->shortname;

				element["columns"][1]["column"] = "unit_id_HIDDEN";
				element["columns"][1]["type"] = "text";
				element["columns"][1]["value"] = iter->id;

				ctrl->addElement(element);
			}
		}
	}
}

void LocalAssetBrowser::PerformTimedActions(void)
{
	// perform checking if updates are needed && update if so.
	local_list_iter iter;
	for (iter = loaded_bitmaps.begin(); iter != loaded_bitmaps.end(); iter++)
	{ iter->updateSelf(); }

	// one or more sculpts have been updated, refreshing them.
	if ( mSculptUpdated )
	{
		LocalAssetBrowser::local_list_iter iter;
		for(iter = loaded_bitmaps.begin(); iter != loaded_bitmaps.end(); iter++)
		{
			if ( iter->sculpt_dirty )
			{
				PerformSculptUpdates( iter->getThis() );
				iter->sculpt_dirty = false;
			}
		}
		mSculptUpdated = false;
	}

	// one of the layer bitmaps has been updated, we need to rebake.
	if ( mLayerUpdated )
	{
	    LLVOAvatar* avatar = gAgent.getAvatarObject();
	    if (avatar) { avatar->forceBakeAllTextures(SLAM_FOR_DEBUG); }
		
		mLayerUpdated = false;
	}
}

void LocalAssetBrowser::PerformSculptUpdates(LocalBitmap* unit)
{

	for( LLDynamicArrayPtr< LLPointer<LLViewerObject>, 256 >::iterator  iter = gObjectList.mObjects.begin();
		 iter != gObjectList.mObjects.end(); iter++ )
	{
		LLViewerObject* obj = *iter;
		
		if ( obj && obj->mDrawable && obj->isSculpted() && obj->getVolume() )
		{
			if ( unit->getID() == obj->getVolume()->getParams().getSculptID() )
			{
				// update code [begin]
				if ( unit->volume_dirty )
				{
					LLImageRaw* rawimage = gImageList.hasImage( unit->getID() )->getCachedRawImage();

					obj->getVolume()->sculpt(rawimage->getWidth(), rawimage->getHeight(), 
					                         rawimage->getComponents(), rawimage->getData(), 0);	

					unit->volume_dirty = false;
				}

					// tell affected drawable it's got updated
					obj->mDrawable->getVOVolume()->setSculptChanged( true );
					obj->mDrawable->getVOVolume()->markForUpdate( true );
				// update code [end]
			}
		}
	}

}

/*==================================================*/
/*  FloaterLocalAssetBrowser : floater class      */
/*==================================================*/ 
/*
	Responsible for talking to the user.
	Instantiated by user request.
	Destroyed when the floater is closed.

*/

// Floater Globals
FloaterLocalAssetBrowser* FloaterLocalAssetBrowser::sInstance = NULL;

// widgets:
	LLButton* mAddBtn;
	LLButton* mDelBtn;
	LLButton* mMoreBtn;
	LLButton* mLessBtn;
	LLButton* mUploadBtn;

	
	LLScrollListCtrl* mBitmapList;
	LLTextureCtrl*    mTextureView;
	LLCheckBoxCtrl*   mUpdateChkBox;

	LLLineEditor* mPathTxt;
	LLLineEditor* mUUIDTxt;
	LLLineEditor* mNameTxt;

	LLTextBox* mLinkTxt;
	LLTextBox* mTimeTxt;
	LLComboBox* mTypeComboBox;

	LLTextBox* mCaptionPathTxt;
	LLTextBox* mCaptionUUIDTxt;
	LLTextBox* mCaptionLinkTxt;
	LLTextBox* mCaptionNameTxt;
	LLTextBox* mCaptionTimeTxt;

FloaterLocalAssetBrowser::FloaterLocalAssetBrowser()
:   LLFloater(std::string("local_bitmap_browser_floater"))
{
	// xui creation:
    LLUICtrlFactory::getInstance()->buildFloater(this, "floater_local_asset_browse.xml");
	
	// setting element/xui children:
	mAddBtn         = getChild<LLButton>("add_btn");
	mDelBtn         = getChild<LLButton>("del_btn");
	mMoreBtn        = getChild<LLButton>("more_btn");
	mLessBtn        = getChild<LLButton>("less_btn");
	mUploadBtn      = getChild<LLButton>("upload_btn");

	mBitmapList     = getChild<LLScrollListCtrl>("bitmap_list");
	mTextureView    = getChild<LLTextureCtrl>("texture_view");
	mUpdateChkBox   = getChild<LLCheckBoxCtrl>("keep_updating_checkbox");

	mPathTxt            = getChild<LLLineEditor>("path_text");
	mUUIDTxt            = getChild<LLLineEditor>("uuid_text");
	mNameTxt            = getChild<LLLineEditor>("name_text");

	mLinkTxt		    = getChild<LLTextBox>("link_text");
	mTimeTxt		    = getChild<LLTextBox>("time_text");
	mTypeComboBox       = getChild<LLComboBox>("type_combobox");

	mCaptionPathTxt     = getChild<LLTextBox>("path_caption_text");
	mCaptionUUIDTxt     = getChild<LLTextBox>("uuid_caption_text");
	mCaptionLinkTxt     = getChild<LLTextBox>("link_caption_text");
	mCaptionNameTxt     = getChild<LLTextBox>("name_caption_text");
	mCaptionTimeTxt	    = getChild<LLTextBox>("time_caption_text");

	// pre-disabling line editors, they're for view only and buttons that shouldn't be on on-spawn.
	mPathTxt->setEnabled( false );
	mUUIDTxt->setEnabled( false );
	mNameTxt->setEnabled( false );

	mDelBtn->setEnabled( false );
	mUploadBtn->setEnabled( false );

	// setting button callbacks:
	mAddBtn->setClickedCallback(         onClickAdd,         this);
	mDelBtn->setClickedCallback(         onClickDel,         this);
	mMoreBtn->setClickedCallback(        onClickMore,        this);
	mLessBtn->setClickedCallback(        onClickLess,        this);
	mUploadBtn->setClickedCallback(      onClickUpload,      this);
	
	// combo callback
	mTypeComboBox->setCommitCallback(onCommitTypeCombo);

	// scrolllist callbacks
	mBitmapList->setCommitCallback(onChooseBitmapList);

	// checkbox callbacks
	mUpdateChkBox->setCommitCallback(onClickUpdateChkbox);
}

void FloaterLocalAssetBrowser::show(void*)
{
    if (!sInstance)
	sInstance = new FloaterLocalAssetBrowser();
    sInstance->open();
	sInstance->UpdateBitmapScrollList();
}

FloaterLocalAssetBrowser::~FloaterLocalAssetBrowser()
{
    sInstance=NULL;
}

void FloaterLocalAssetBrowser::onClickAdd(void* userdata)
{
	LLFilePicker& picker = LLFilePicker::instance();
	if ( !picker.getOpenFile(LLFilePicker::FFLOAD_IMAGE) ) 
	   { return; }

	std::string filename = picker.getFirstFile();

	if ( !filename.empty() )
	{ 
		if ( gLocalBrowser->AddBitmap(filename) )
		{
			sInstance->UpdateBitmapScrollList();
			sInstance->UpdateRightSide();
		}
	}
}

void FloaterLocalAssetBrowser::onClickDel(void* userdata)
{
	std::string temp_id = sInstance->mBitmapList->getSelectedItemLabel(BITMAPLIST_COL_ID);

	if ( !temp_id.empty() )
	{
		if ( gLocalBrowser->DelBitmap( (LLUUID)temp_id ) )
		{
			sInstance->UpdateBitmapScrollList();
			sInstance->UpdateRightSide();
		}
	}
}

/* what stopped me from using a single button and simply changing it's label
   is the fact that i'd need to hardcode the button labels here, and that is griff. */
void FloaterLocalAssetBrowser::onClickMore(void* userdata)
{
	FloaterResize(true);
}

void FloaterLocalAssetBrowser::onClickLess(void* userdata)
{
	FloaterResize(false);
}

void FloaterLocalAssetBrowser::onClickUpload(void* userdata)
{
	std::string filename = gLocalBrowser->GetBitmapUnit( 
		(LLUUID)sInstance->mBitmapList->getSelectedItemLabel(BITMAPLIST_COL_ID) )->getFileName();

	if ( !filename.empty() )
	{
		LLFloaterImagePreview* floaterp = new LLFloaterImagePreview(filename);
		LLUICtrlFactory::getInstance()->buildFloater(floaterp, "floater_image_preview.xml");
	}
}

void FloaterLocalAssetBrowser::onChooseBitmapList(LLUICtrl* ctrl, void *userdata)
{
	bool button_status = sInstance->mBitmapList->isEmpty();
	sInstance->mDelBtn->setEnabled(!button_status);
	sInstance->mUploadBtn->setEnabled(!button_status);

	sInstance->UpdateRightSide();
}

void FloaterLocalAssetBrowser::onClickUpdateChkbox(LLUICtrl *ctrl, void *userdata)
{
	std::string temp_str = sInstance->mBitmapList->getSelectedItemLabel(BITMAPLIST_COL_ID);
	if ( !temp_str.empty() )
	{
		gLocalBrowser->onUpdateBool( (LLUUID)temp_str );
		sInstance->UpdateRightSide();
	}
}

void FloaterLocalAssetBrowser::onCommitTypeCombo(LLUICtrl* ctrl, void *userdata)
{
	std::string temp_str = sInstance->mBitmapList->getSelectedItemLabel(BITMAPLIST_COL_ID);

	if ( !temp_str.empty() )
	{
		S32 selection = sInstance->mTypeComboBox->getCurrentIndex();
		gLocalBrowser->onSetType( (LLUUID)temp_str, selection ); 

	}
}

void FloaterLocalAssetBrowser::FloaterResize(bool expand)
{
	sInstance->mMoreBtn->setVisible(!expand);
	sInstance->mLessBtn->setVisible(expand);
	sInstance->mTextureView->setVisible(expand);
	sInstance->mUpdateChkBox->setVisible(expand);
	sInstance->mCaptionPathTxt->setVisible(expand);
	sInstance->mCaptionUUIDTxt->setVisible(expand);
	sInstance->mCaptionLinkTxt->setVisible(expand);
	sInstance->mCaptionNameTxt->setVisible(expand);
	sInstance->mCaptionTimeTxt->setVisible(expand);
	sInstance->mTypeComboBox->setVisible(expand);

	sInstance->mTimeTxt->setVisible(expand);
	sInstance->mPathTxt->setVisible(expand);
	sInstance->mUUIDTxt->setVisible(expand);
	sInstance->mLinkTxt->setVisible(expand);
	sInstance->mNameTxt->setVisible(expand);

	if(expand)
	{ 
		sInstance->reshape(LLF_FLOATER_EXPAND_WIDTH, LLF_FLOATER_HEIGHT); 
		sInstance->setResizeLimits(LLF_FLOATER_EXPAND_WIDTH, LLF_FLOATER_HEIGHT);
		sInstance->UpdateRightSide();
	}
	else
	{ 
		sInstance->reshape(LLF_FLOATER_CONTRACT_WIDTH, LLF_FLOATER_HEIGHT);
		sInstance->setResizeLimits(LLF_FLOATER_CONTRACT_WIDTH, LLF_FLOATER_HEIGHT);
	}

}

void FloaterLocalAssetBrowser::UpdateBitmapScrollList()
{
	sInstance->mBitmapList->clearRows();

	if (!gLocalBrowser->loaded_bitmaps.empty())
	{
		
		LocalAssetBrowser::local_list_iter iter;
		for(iter = gLocalBrowser->loaded_bitmaps.begin(); iter != gLocalBrowser->loaded_bitmaps.end(); iter++)
		{
			LLSD element;
			element["columns"][BITMAPLIST_COL_NAME]["column"] = "bitmap_name";
			element["columns"][BITMAPLIST_COL_NAME]["type"]   = "text";
			element["columns"][BITMAPLIST_COL_NAME]["value"]  = iter->getShortName();

			element["columns"][BITMAPLIST_COL_ID]["column"] = "bitmap_uuid";
			element["columns"][BITMAPLIST_COL_ID]["type"]   = "text";
			element["columns"][BITMAPLIST_COL_ID]["value"]  = iter->getID();

			sInstance->mBitmapList->addElement(element);
		}

	}
	sInstance->UpdateRightSide();
}

void FloaterLocalAssetBrowser::UpdateRightSide()
{
	/*
	Since i'm not keeping a bool on if the floater is expanded or not, i'll
	just check if one of the widgets that shows when the floater is expanded is visible.

	Also obviously before updating - checking if something IS actually selected :o
	*/

	if ( !sInstance->mTextureView->getVisible() ) { return; }

	if ( !sInstance->mBitmapList->getAllSelected().empty() )
	{ 
		LocalBitmap* unit = gLocalBrowser->GetBitmapUnit( LLUUID(sInstance->mBitmapList->getSelectedItemLabel(BITMAPLIST_COL_ID)) );
		
		if ( unit )
		{
			sInstance->mTextureView->setImageAssetID( unit->getID() );
			sInstance->mUpdateChkBox->set( unit->getUpdateBool() );
			sInstance->mPathTxt->setText( unit->getFileName() );
			sInstance->mUUIDTxt->setText( unit->getID().asString() );
			sInstance->mNameTxt->setText( unit->getShortName() );
			sInstance->mTimeTxt->setText( unit->getLastModified().asString() );
			sInstance->mLinkTxt->setText( unit->getLinkStatus() );
			sInstance->mTypeComboBox->selectNthItem( unit->getType() );

			sInstance->mTextureView->setEnabled(true);
			sInstance->mUpdateChkBox->setEnabled(true);
			sInstance->mTypeComboBox->setEnabled(true);
		}
	}
	else
	{
		sInstance->mTextureView->setImageAssetID( NO_IMAGE );
		sInstance->mTextureView->setEnabled( false );
		sInstance->mUpdateChkBox->set( false );
		sInstance->mUpdateChkBox->setEnabled( false );

		sInstance->mTypeComboBox->selectFirstItem();
		sInstance->mTypeComboBox->setEnabled( false );
		
		sInstance->mPathTxt->setText( LLStringExplicit("None") );
		sInstance->mUUIDTxt->setText( LLStringExplicit("None") );
		sInstance->mNameTxt->setText( LLStringExplicit("None") );
		sInstance->mLinkTxt->setText( LLStringExplicit("None") );
		sInstance->mTimeTxt->setText( LLStringExplicit("None") );
	}
}


/*==================================================*/
/*     LocalAssetBrowserTimer: timer class          */
/*==================================================*/ 
/*
	A small, simple timer class inheriting from
	LLEventTimer, responsible for pinging the
	LocalAssetBrowser class to perform it's
	updates / checks / etc.

*/

LocalAssetBrowserTimer::LocalAssetBrowserTimer() : LLEventTimer( (F32)TIMER_HEARTBEAT )
{

}

LocalAssetBrowserTimer::~LocalAssetBrowserTimer()
{

}

BOOL LocalAssetBrowserTimer::tick()
{
	gLocalBrowser->PerformTimedActions();
	return FALSE;
}

void LocalAssetBrowserTimer::start()
{
	mEventTimer.start();
}

void LocalAssetBrowserTimer::stop()
{
	mEventTimer.stop();
}

bool LocalAssetBrowserTimer::isRunning()
{
	return mEventTimer.getStarted();
}


