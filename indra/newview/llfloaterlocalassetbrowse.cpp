/* Local Asset Browser: source */
/*

tag: vaa emerald local_asset_browser

other changed files:
	llviewermenu.cpp, menu_viewer.xml
	lltexturectrl.h, lltexturectrl.cpp, floater_texture_ctrl.xml
	pipelin.cpp, pipeline.h
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
#include "llfloaterlocalassetbrowse.h"

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
#include "lldrawable.h"
#include "llvovolume.h"
#include "pipeline.h"
#include "llspatialpartition.h"


/*=======================================*/
/*     Instantiating manager class       */
/*    and formally declaring it's list   */
/*=======================================*/ 
LLLocalAssetBrowser* gLocalBrowser;
LLLocalAssetBrowserTimer* gLocalBrowserTimer;
std::vector<LLLocalBitmap> LLLocalAssetBrowser::loaded_bitmaps;
bool    LLLocalAssetBrowser::mLayerUpdated;
bool    LLLocalAssetBrowser::mSculptUpdated;

/*=======================================*/
/*  LLLocalBitmapUnit: unit class        */
/*=======================================*/ 
/*
	Product of a complete rewrite of this
	system.
	
	The idea is to have each unit manage
	itself as much as possible.
*/

LLLocalBitmap::LLLocalBitmap(std::string fullpath)
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
		this->rawpointer    = new LLImageRaw;
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
		if ( this->decodeSelf() )
		{
			/* creating a shell LLViewerImage and fusing raw image into it */
			LLViewerImage* viewer_image = new LLViewerImage( "file://"+this->filename, this->id, LOCAL_USE_MIPMAPS );
			viewer_image->createGLTexture( LOCAL_DISCARD_LEVEL, this->rawpointer );

			/* making damn sure gImageList will not delete it prematurely */
			viewer_image->ref(); 

			/* finalizing by adding LLViewerImage instance into gImageList */
			gImageList.addImage(viewer_image);

			/* filename is valid, bitmap is decoded and valid, i can haz liftoff! */
			this->valid = true;
		}
	}
}

LLLocalBitmap::~LLLocalBitmap()
{

}

/* [maintenence functions] */
void LLLocalBitmap::updateSelf()
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
		if ( !decodeSelf() ) { this->linkstatus = LINK_BROKEN; return; }
		gImageList.hasImage(this->id)->createGLTexture( LOCAL_DISCARD_LEVEL, this->rawpointer );

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

bool LLLocalBitmap::decodeSelf()
{
	switch (this->extension)
	{
		case IMG_EXTEN_BMP:
			{
				LLPointer<LLImageBMP> bmp_image = new LLImageBMP;
				if ( !bmp_image->load(filename) ) { break; }
				if ( !bmp_image->decode(rawpointer, 0.0f) ) { break; }

				this->rawpointer->biasedScaleToPowerOfTwo( LLViewerImage::MAX_IMAGE_SIZE_DEFAULT );
				return true;
			}

		case IMG_EXTEN_TGA:
			{
				LLPointer<LLImageTGA> tga_image = new LLImageTGA;
				if ( !tga_image->load(filename) ) { break; }
				if ( !tga_image->decode(rawpointer) ) { break; }

				if(	( tga_image->getComponents() != 3) &&
					( tga_image->getComponents() != 4) ) { break; }

				this->rawpointer->biasedScaleToPowerOfTwo( LLViewerImage::MAX_IMAGE_SIZE_DEFAULT );
				return true;
			}

		case IMG_EXTEN_JPG:
			{
				LLPointer<LLImageJPEG> jpeg_image = new LLImageJPEG;
				if ( !jpeg_image->load(filename) ) { break; }
				if ( !jpeg_image->decode(rawpointer, 0.0f) ) { break; }

				this->rawpointer->biasedScaleToPowerOfTwo( LLViewerImage::MAX_IMAGE_SIZE_DEFAULT );
				return true;
			}

		case IMG_EXTEN_PNG:
			{
				LLPointer<LLImagePNG> png_image = new LLImagePNG;
				if ( !png_image->load(filename) ) { break; }
				if ( !png_image->decode(rawpointer, 0.0f) ) { break; }

				this->rawpointer->biasedScaleToPowerOfTwo( LLViewerImage::MAX_IMAGE_SIZE_DEFAULT );
				return true;
			}

		default:
			break;
	}
	return false;
}

void LLLocalBitmap::setUpdateBool()
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

void LLLocalBitmap::setType( S32 type )
{
	this->bitmap_type = type;
}

/* [information query functions] */
std::string LLLocalBitmap::getShortName()
{
	return this->shortname;
}

std::string LLLocalBitmap::getFileName()
{
	return this->filename;
}

LLUUID LLLocalBitmap::getID()
{
	return this->id;
}

LLSD LLLocalBitmap::getLastModified()
{
	return this->last_modified;
}

std::string LLLocalBitmap::getLinkStatus()
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

bool LLLocalBitmap::getUpdateBool()
{
	return this->keep_updating;
}

bool LLLocalBitmap::getIfValidBool()
{
	return this->valid;
}

LLLocalBitmap* LLLocalBitmap::getThis()
{
	return this;
}

S32 LLLocalBitmap::getType()
{
	return this->bitmap_type;
}

void LLLocalBitmap::getDebugInfo()
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
/*  LLLocalAssetBrowser: internal class  */
/*=======================================*/ 
/*
	Responsible for internal workings.
	Instantiated at the top of the source file.
	Sits in memory until the viewer is closed.
*/

LLLocalAssetBrowser::LLLocalAssetBrowser()
{
	this->mLayerUpdated = false;
	this->mSculptUpdated = false;
}

LLLocalAssetBrowser::~LLLocalAssetBrowser()
{

}

bool LLLocalAssetBrowser::AddBitmap(std::string filename)
{

	LLLocalBitmap* unit = new LLLocalBitmap( filename );

	if	( unit->getIfValidBool() )
	{
		loaded_bitmaps.push_back( *unit );
		onChangeHappened();
		return true;
	}

	return false;
}

bool LLLocalAssetBrowser::DelBitmap(LLUUID id)
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

void LLLocalAssetBrowser::onUpdateBool(LLUUID id)
{
	LLLocalBitmap* unit = GetBitmapUnit( id );
	if ( unit ) 
	{ 
		unit->setUpdateBool(); 
		PingTimer();
	}
}

void LLLocalAssetBrowser::onSetType(LLUUID id, S32 type)
{
	LLLocalBitmap* unit = GetBitmapUnit( id );
	if ( unit ) 
	{ unit->setType(type); }
}

LLLocalBitmap* LLLocalAssetBrowser::GetBitmapUnit(LLUUID id)
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

bool LLLocalAssetBrowser::IsDoingUpdates()
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
void LLLocalAssetBrowser::onChangeHappened()
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

void LLLocalAssetBrowser::PingTimer()
{
	if ( !loaded_bitmaps.empty() && IsDoingUpdates() )
	{
		if (!gLocalBrowserTimer) 
		{ gLocalBrowserTimer = new LLLocalAssetBrowserTimer(); }
		
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
void LLLocalAssetBrowser::UpdateTextureCtrlList(LLScrollListCtrl* ctrl)
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

void LLLocalAssetBrowser::PerformTimedActions(void)
{
	// perform checking if updates are needed && update if so.
	local_list_iter iter;
	for (iter = loaded_bitmaps.begin(); iter != loaded_bitmaps.end(); iter++)
	{ iter->updateSelf(); }

	// one or more sculpts have been updated, refreshing them.
	if ( mSculptUpdated )
	{
		LLLocalAssetBrowser::local_list_iter iter;
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

void LLLocalAssetBrowser::PerformSculptUpdates(LLLocalBitmap* unit)
{
	// i hate the pipeline. someone please shoot it :o

	for ( LLCullResult::sg_list_t::iterator sg_iter = gPipeline.getsCull()->beginVisibleGroups();
		  sg_iter != gPipeline.getsCull()->endVisibleGroups(); sg_iter++ )
	{
		LLSpatialGroup* sgroup = *sg_iter;
		
		for ( LLOctreeNode<LLDrawable>::element_iter elem_iter = sgroup->getData().begin();
		      elem_iter != sgroup->getData().end(); elem_iter++ )
		{

			LLDrawable* pdraw = *elem_iter;
			if ( pdraw && pdraw->getVObj() && pdraw->getVObj()->isSculpted() )
			{
				/* update code here */
				
				if ( unit->getID() == pdraw->getVObj()->getVolume()->getParams().getSculptID() )
				{

					// do shared volume recalculation only once per sharing drawables
					if ( unit->volume_dirty )
					{
						
						pdraw->getVObj()->getVolume()->sculpt(unit->rawpointer->getWidth(), unit->rawpointer->getHeight(), 
													unit->rawpointer->getComponents(), unit->rawpointer->getData(), 0);	

						unit->volume_dirty = false;
					}

					// tell affected drawable it's got updated
					pdraw->getVOVolume()->setSculptChanged( true );
					pdraw->getVOVolume()->markForUpdate( true );

				}

				/* end update code */
			}

		} // [end for treenode]

	} // [end for spatial group]	
}

/*==================================================*/
/*  LLFloaterLocalAssetBrowser : floater class      */
/*==================================================*/ 
/*
	Responsible for talking to the user.
	Instantiated by user request.
	Destroyed when the floater is closed.

*/

// Floater Globals
LLFloaterLocalAssetBrowser* LLFloaterLocalAssetBrowser::sInstance = NULL;

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

LLFloaterLocalAssetBrowser::LLFloaterLocalAssetBrowser()
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

void LLFloaterLocalAssetBrowser::show(void*)
{
    if (!sInstance)
	sInstance = new LLFloaterLocalAssetBrowser();
    sInstance->open();
	sInstance->UpdateBitmapScrollList();
}

LLFloaterLocalAssetBrowser::~LLFloaterLocalAssetBrowser()
{
    sInstance=NULL;
}

void LLFloaterLocalAssetBrowser::onClickAdd(void* userdata)
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

void LLFloaterLocalAssetBrowser::onClickDel(void* userdata)
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
void LLFloaterLocalAssetBrowser::onClickMore(void* userdata)
{
	FloaterResize(true);
}

void LLFloaterLocalAssetBrowser::onClickLess(void* userdata)
{
	FloaterResize(false);
}

void LLFloaterLocalAssetBrowser::onClickUpload(void* userdata)
{
	std::string filename = gLocalBrowser->GetBitmapUnit( 
		(LLUUID)sInstance->mBitmapList->getSelectedItemLabel(BITMAPLIST_COL_ID) )->getFileName();

	if ( !filename.empty() )
	{
		LLFloaterImagePreview* floaterp = new LLFloaterImagePreview(filename);
		LLUICtrlFactory::getInstance()->buildFloater(floaterp, "floater_image_preview.xml");
	}
}

void LLFloaterLocalAssetBrowser::onChooseBitmapList(LLUICtrl* ctrl, void *userdata)
{
	bool button_status = sInstance->mBitmapList->isEmpty();
	sInstance->mDelBtn->setEnabled(!button_status);
	sInstance->mUploadBtn->setEnabled(!button_status);

	sInstance->UpdateRightSide();
}

void LLFloaterLocalAssetBrowser::onClickUpdateChkbox(LLUICtrl *ctrl, void *userdata)
{
	std::string temp_str = sInstance->mBitmapList->getSelectedItemLabel(BITMAPLIST_COL_ID);
	if ( !temp_str.empty() )
	{
		gLocalBrowser->onUpdateBool( (LLUUID)temp_str );
		sInstance->UpdateRightSide();
	}
}

void LLFloaterLocalAssetBrowser::onCommitTypeCombo(LLUICtrl* ctrl, void *userdata)
{
	std::string temp_str = sInstance->mBitmapList->getSelectedItemLabel(BITMAPLIST_COL_ID);

	if ( !temp_str.empty() )
	{
		S32 selection = sInstance->mTypeComboBox->getCurrentIndex();
		gLocalBrowser->onSetType( (LLUUID)temp_str, selection ); 

	}
}

void LLFloaterLocalAssetBrowser::FloaterResize(bool expand)
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

void LLFloaterLocalAssetBrowser::UpdateBitmapScrollList()
{
	sInstance->mBitmapList->clearRows();

	if (!gLocalBrowser->loaded_bitmaps.empty())
	{
		
		LLLocalAssetBrowser::local_list_iter iter;
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

void LLFloaterLocalAssetBrowser::UpdateRightSide()
{
	/*
	Since i'm not keeping a bool on if the floater is expanded or not, i'll
	just check if one of the widgets that shows when the floater is expanded is visible.

	Also obviously before updating - checking if something IS actually selected :o
	*/

	if ( !sInstance->mTextureView->getVisible() ) { return; }

	if ( !sInstance->mBitmapList->getAllSelected().empty() )
	{ 
		LLLocalBitmap* unit = gLocalBrowser->GetBitmapUnit( LLUUID(sInstance->mBitmapList->getSelectedItemLabel(BITMAPLIST_COL_ID)) );
		
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
/*     LLLocalAssetBrowserTimer : timer class       */
/*==================================================*/ 
/*
	A small, simple timer class inheriting from
	LLEventTimer, responsible for pinging the
	LLLocalAssetBrowser class to perform it's
	updates / checks / etc.

*/

LLLocalAssetBrowserTimer::LLLocalAssetBrowserTimer() : LLEventTimer( (F32)TIMER_HEARTBEAT )
{

}

LLLocalAssetBrowserTimer::~LLLocalAssetBrowserTimer()
{

}

BOOL LLLocalAssetBrowserTimer::tick()
{
	gLocalBrowser->PerformTimedActions();
	return FALSE;
}

void LLLocalAssetBrowserTimer::start()
{
	mEventTimer.start();
}

void LLLocalAssetBrowserTimer::stop()
{
	mEventTimer.stop();
}

bool LLLocalAssetBrowserTimer::isRunning()
{
	return mEventTimer.getStarted();
}


