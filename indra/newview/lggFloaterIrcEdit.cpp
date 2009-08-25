/* Copyright (c) 2009
 *
 * LordGregGreg Back. All rights reserved.
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
 *   3. Neither the name Modular Systems Ltd nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MODULAR SYSTEMS LTD AND CONTRIBUTORS "AS IS"
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
#include "lggFloaterIrcEdit.h"
#include "lggFloaterIrc.h"
//#include "lggIrcGroupHandler.h"

#include "llagentdata.h"
#include "llcommandhandler.h"
#include "llfloater.h"
#include "llsdutil.h"
#include "lluictrlfactory.h"
#include "llviewercontrol.h"
#include "llagent.h"
#include "llfilepicker.h"
#include "llpanel.h"
#include "lliconctrl.h"
#include "llbutton.h"
#include "llcolorswatch.h"
#include "lggBeamMaps.h"

#include "llsdserialize.h"

class lggFloaterIrcEdit;

////////////////////////////////////////////////////////////////////////////
// lggBeamMapFloater
class lggFloaterIrcEdit : public LLFloater, public LLFloaterSingleton<lggFloaterIrcEdit>
{
public:
	lggPanelIRC* caller;
	lggFloaterIrcEdit(const LLSD& seed);
	virtual ~lggFloaterIrcEdit();
	

	BOOL postBuild(void);
	void update(lggIrcData dat,void* data);
	
	void draw();
	//void showInstance(lggIrcData dat);
	
	// UI Handlers
	static void onClickSave(void* data);
	//static void onClickClear(void* data);
	static void onClickCancel(void* data);

	
private:
	static void onBackgroundChange(LLUICtrl* ctrl, void* userdata);
	
};
//void lggFloaderIrcEdit::showInstance(lggIrcData dat)
//{
//	LLFloater::showInstance();
//}
void lggFloaterIrcEdit::draw()
{
	LLFloater::draw();
}

lggFloaterIrcEdit::~lggFloaterIrcEdit()
{
	//if(mCallback) mCallback->detach();
}

lggFloaterIrcEdit::lggFloaterIrcEdit(const LLSD& seed)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_IRCinfo.xml");
	
	if (getRect().mLeft == 0 
		&& getRect().mBottom == 0)
	{
		center();
	}

}

BOOL lggFloaterIrcEdit::postBuild(void)
{
	//setCanMinimize(false);
	childSetAction("EmeraldIRC_save",onClickSave,this);
	childSetAction("EmeraldIRC_cancel",onClickCancel,this);
	
	return true;
}
void lggFloaterIrcEdit::update(lggIrcData dat, void* data)
{
	caller = (lggPanelIRC*)data;
	childSetValue("EmeraldIRC_nick",dat.nick);
	childSetValue("EmeraldIRC_server",dat.server);
	childSetValue("EmeraldIRC_password",dat.password);
	childSetValue("EmeraldIRC_channel",dat.channel);
	childSetValue("EmeraldIRC_tag",dat.name);
	childSetValue("EmeraldIRC_port",dat.port);

}
void lggFloaterIrcEdit::onClickSave(void* data)
{
	llinfos << "lggPanelIRCedit::save" << llendl;
	
	lggFloaterIrcEdit* self = (lggFloaterIrcEdit*)data;
	//LLFilePicker& picker = LLFilePicker::instance();
	lggIrcData dat(
	self->childGetValue("EmeraldIRC_server"),	
	self->childGetValue("EmeraldIRC_tag"),
	self->childGetValue("EmeraldIRC_port"),
	self->childGetValue("EmeraldIRC_nick"),	
	self->childGetValue("EmeraldIRC_channel"),
	self->childGetValue("EmeraldIRC_password"),
	LLUUID::generateNewID());

	std::string path_name2(gDirUtilp->getExpandedFilename( LL_PATH_USER_SETTINGS , "IRCGroups", ""));
				
	std::string filename=path_name2 + dat.name+".xml";
		
	
	
	LLSD main=dat.toLLSD();
	llofstream export_file;
	export_file.open(filename);
	LLSDSerialize::toPrettyXML(main, export_file);
	export_file.close();
	//lggPanelIRC* instance = (lggPanelIRC*)caller;	if(instance)	instance.refresh();
	
	//gSavedSettings.setString("EmeraldBeamShape",gDirUtilp->getBaseFileName(filename,true));
	if(self->caller)
	{
		self->caller->reset();
	}
	
	self->close();
}

void lggFloaterIrcEdit::onClickCancel(void* data)
{
	lggFloaterIrcEdit* self = (lggFloaterIrcEdit*)data;
	self->close();
	
}
void LggIrcFloaterStarter::show(lggIrcData dat,void* data)
{
	
	lggFloaterIrcEdit* floater = lggFloaterIrcEdit::showInstance();
	floater->update(dat, data);
	
	//beam_floater->update();
}


