
#include "llviewerprecompiledheaders.h"

#include "lggBeamMapFloater.h"

#include "llagentdata.h"
#include "llcachename.h"
#include "llcommandhandler.h"
#include "llfloater.h"
#include "llfloateravatarinfo.h"
#include "llfloatergroupinfo.h"
#include "llfloatermute.h"
#include "llmutelist.h"
#include "llsdutil.h"
#include "lluictrlfactory.h"
#include "llurldispatcher.h"
#include "llviewercontrol.h"
#include "lllandmark.h"
#include "llagent.h"
#include "llworldmap.h"
#include "llpanel.h"
#include "llregionhandle.h"
#include "lliconctrl.h"
class lggPoint;
class lggBeamMapFloater;

////////////////////////////////////////////////////////////////////////////
// lggBeamMapFloater
class lggBeamMapFloater : public LLFloater, public LLFloaterSingleton<lggBeamMapFloater>
{
public:
	lggBeamMapFloater(const LLSD& seed);
	virtual ~lggBeamMapFloater();
	

	BOOL postBuild(void);
	BOOL handleMouseDown(S32 x,S32 y,MASK mask);
	void update();

	void draw();
	void clearPoints();

	// UI Handlers
	static void onClickSave(void* data);
	static void onClickClear(void* data);

	
private:
	std::vector<lggPoint> dots;
};
class lggPoint
{
	public:
		S32 x;
		S32 y;
};


void lggBeamMapFloater::clearPoints()
{
	dots.clear();
	LLPanel* panel = getChild<LLPanel>("beamshape_draw");
	if(panel)
	{
		panel->deleteAllChildren();
	}
}
void lggBeamMapFloater::draw()
{
	LLFloater::draw();
}

lggBeamMapFloater::~lggBeamMapFloater()
{
	//if(mCallback) mCallback->detach();
}

/*lggBeamMapFloater::lggBeamMapFloater(const LLSD& seed)
: mObjectID(), mObjectName(), mSlurl(), mOwnerID(), mOwnerName(), mOwnerIsGroup(false), lookingforRegion(false), mRegionName()
*/
lggBeamMapFloater::lggBeamMapFloater(const LLSD& seed)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_beamshape.xml");
	
	if (getRect().mLeft == 0 
		&& getRect().mBottom == 0)
	{
		center();
	}

}

BOOL lggBeamMapFloater::postBuild(void)
{
	childSetAction("beamshape_save",onClickSave,this);
	childSetAction("beamshape_clear",onClickClear,this);
	
	LLPanel* panel = getChild<LLPanel>("beamshape_draw");
	if(panel)
	{
		
	}
	
	return true;
}
BOOL lggBeamMapFloater::handleMouseDown(S32 x,S32 y,MASK mask)
{
	lggPoint a;
	a.x=x;
	a.y=y;
	dots.push_back(a);
	LLPanel* panel = getChild<LLPanel>("beamshape_draw");
	if(panel)
	{
		llinfos << "we got clicked at (" << x << ", " << y << llendl;
		//Emerald TODO these icons dont show up!!! DX
		LLIconCtrl* icon = new LLIconCtrl(std::string("Point "+dots.size()),
			LLRect(x-10,y-10,x+10,y+10),
			std::string("lag_status_good.tga"));
			icon->setMouseOpaque(FALSE);
			icon->setVisible(true);
		
	
		panel->addChildAtEnd(icon);
		panel->sendChildToFront(icon);
		//panel->addCtrl((LLUICtrl*)newButton,0);
	}
	return LLFloater::handleMouseDown(x,y,mask);
}

void lggBeamMapFloater::update()
{
	
}
void lggBeamMapFloater::onClickSave(void* data)
{
	//lggBeamMapFloater* self = (lggBeamMapFloater*)data;

	
}
void lggBeamMapFloater::onClickClear(void* data)
{
	lggBeamMapFloater* self = (lggBeamMapFloater*)data;
	self->clearPoints();
	
}

void LggBeamMap::show(BOOL showin)
{
	//lggBeamMapFloater* beam_floater = 
	if(showin)
	lggBeamMapFloater::showInstance();
	//beam_floater->update();
}