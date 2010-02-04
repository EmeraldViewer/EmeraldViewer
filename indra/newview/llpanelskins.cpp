/** 
 * @file llpanelskins.cpp
 * @brief General preferences panel in preferences floater
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llpanelskins.h"

// linden library includes
#include "llradiogroup.h"
#include "llbutton.h"
#include "lluictrlfactory.h"

// project includes
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llcombobox.h"
#include "llsdserialize.h"


LLPanelSkins* LLPanelSkins::sInstance;
LLPanelSkins::LLPanelSkins()
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_preferences_skins.xml");
	if(sInstance)delete sInstance;
	sInstance = this;
}

LLPanelSkins::~LLPanelSkins()
{
}

BOOL LLPanelSkins::postBuild()
{
	getChild<LLComboBox>("emrd_skin_combo")->setCommitCallback(onComboBoxCommit);
	refresh();
	return TRUE;
}

void LLPanelSkins::refresh()
{
	mSkin = gSavedSettings.getString("SkinCurrent");
	LLComboBox* comboBox = getChild<LLComboBox>("emrd_skin_combo");

	if(comboBox != NULL) 
	{
		comboBox->removeall();
		//comboBox->add("===OFF===");
		std::string path_name(gDirUtilp->getSkinBaseDir());
		bool found = true;			
		while(found) 
		{
			std::string name;
			found = gDirUtilp->getNextFileInDir(path_name, "*.xml", name, false);
			if(found)
			{
				LLSD data;
				datas.push_back(data);
				llifstream importer(name);
				LLSDSerialize::fromXMLDocument(data, importer);
				comboBox->add(data["skin_name"].asString());
				if(data["folder_name"].asString()==mSkin)
				{
					comboBox->setSimple(data["skin_name"].asString());
					//LLButton* b;
					//b.setImageOverlay()
					childSetValue("emrd_skin_author",data["author_name"].asString());
					childSetValue("emrd_skin_ad_authors",data["additional_author_names"].asString());
					childSetValue("emrd_skin_info",data["skin_info"].asString());
					//LLButton* b = getChild<LLButton>("emrd_skin_preview");
					//b->setsetImageSelected()
					//<button scale_image="true" image_selected="skin_thumbnail_default.png"
					//image_unselected="skin_thumbnail_default.png" 
					//	image_hover_selected="skin_thumbnail_default.png" 
					//	image_hover_unselected="skin_thumbnail_default.png"/>

					//set the rest here!
				}
			}
		}
	}
}

void LLPanelSkins::apply()
{
	if (mSkin != gSavedSettings.getString("SkinCurrent"))
	{
		  LLNotifications::instance().add("ChangeSkin");
		  refresh();
	}
}

void LLPanelSkins::cancel()
{
	// reverts any changes to current skin
	gSavedSettings.setString("SkinCurrent", mSkin);
}

//static
void LLPanelSkins::onComboBoxCommit(LLUICtrl* ctrl, void* userdata)
{
	LLComboBox* box = (LLComboBox*)ctrl;
	if(box)
	{
		std::string skinName = box->getValue().asString();
		for(int i =0;i<(int)sInstance->datas.size();i++)
		{
			LLSD tdata=sInstance->datas[i];
			if(tdata["skin_name"].asString()==skinName)
			{
				gSavedSettings.setString("SkinCurrent",tdata["skin_folder"].asString());
			}
		}
		if(sInstance)sInstance->refresh();
	}	
}

