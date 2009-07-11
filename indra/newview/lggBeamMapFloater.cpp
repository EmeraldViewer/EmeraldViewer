
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

class lggBeamMapFloater;

////////////////////////////////////////////////////////////////////////////
// lggBeamMapFloater
class lggBeamMapFloater : public LLFloater, public LLFloaterSingleton<lggBeamMapFloater>
{
public:
	lggBeamMapFloater(const LLSD& seed);
	virtual ~lggBeamMapFloater();

	BOOL postBuild(void);

	void update();

	void draw();

	// UI Handlers
	static void onClickSave(void* data);
	static void onClickClear(void* data);

	
private:
	LLUUID mObjectID;
};

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
		//panel->handleMouseDown
	}

	return true;
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
	//ggBeamMapFloater* self = (lggBeamMapFloater*)data;

	
}

void LggBeamMap::show(BOOL showin)
{
	//lggBeamMapFloater* beam_floater = 
	if(showin)
	lggBeamMapFloater::showInstance();
	//beam_floater->update();
}