/* Copyright (c) 2009
 *
 * Modular Systems All rights reserved.
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
 *   3. Neither the name Modular Systems nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MODULAR SYSTEMS AND CONTRIBUTORS “AS IS”
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

#include <sstream>
#include "llviewerprecompiledheaders.h"

#include "scriptcounter.h"
#include "llviewerobjectlist.h"
#include "llvoavatar.h"
#include "llviewerregion.h"
#include "llwindow.h"
#include "lltransfersourceasset.h"
#include "llviewernetwork.h"

ScriptCounter* ScriptCounter::sInstance;
U32 ScriptCounter::invqueries;
U32 ScriptCounter::status;
U32 ScriptCounter::scriptcount;
std::set<std::string> ScriptCounter::objIDS;
void cmdline_printchat(std::string chat);
ScriptCounter::ScriptCounter()
{
	llassert_always(sInstance == NULL);
	sInstance = this;

}

ScriptCounter::~ScriptCounter()
{
	sInstance = NULL;
}
void ScriptCounter::init()
{
	if(!sInstance)
	{
		sInstance = new ScriptCounter();
	}
	status = IDLE;
}

LLVOAvatar* find_avatar_from_object( LLViewerObject* object );

LLVOAvatar* find_avatar_from_object( const LLUUID& object_id );

void ScriptCounter::serializeSelection()
{
	LLDynamicArray<LLViewerObject*> catfayse;
	for (LLObjectSelection::valid_root_iterator iter = LLSelectMgr::getInstance()->getSelection()->valid_root_begin();
			 iter != LLSelectMgr::getInstance()->getSelection()->valid_root_end(); iter++)
	{
		LLSelectNode* selectNode = *iter;
		LLViewerObject* object = selectNode->getObject();
		if(object)catfayse.put(object);
	}
	scriptcount=0;
	cmdline_printchat("Counting scripts. Please wait.");
	serialize(catfayse);
}

void ScriptCounter::serialize(LLDynamicArray<LLViewerObject*> objects)
{

	init();

	status = COUNTING;

	F32 throttle = gSavedSettings.getF32("OutBandwidth");
	// Gross magical value that is 128kbit/s
	// Sim appears to drop requests if they come in faster than this. *sigh*
	if((throttle == 0.f) || (throttle > 128000.f))
	{
		gMessageSystem->mPacketRing.setOutBandwidth(128000);
		gMessageSystem->mPacketRing.setUseOutThrottle(TRUE);
	}

	for(LLDynamicArray<LLViewerObject*>::iterator itr = objects.begin(); itr != objects.end(); ++itr)
	{
		LLViewerObject* object = *itr;
		if (object)
			subserialize(object);
	}
	if(invqueries == 0)
	{
		init();
	}
	if(throttle != 0.f)
	{
		gMessageSystem->mPacketRing.setOutBandwidth(throttle);
		gMessageSystem->mPacketRing.setUseOutThrottle(TRUE);
	}
	else
	{
		gMessageSystem->mPacketRing.setOutBandwidth(0.0);
		gMessageSystem->mPacketRing.setUseOutThrottle(FALSE);
	}
}

void ScriptCounter::subserialize(LLViewerObject* linkset)
{
	//Chalice - Changed to support exporting linkset groups.
	LLViewerObject* object = linkset;

	// Build a list of everything that we'll be checking inventory of.
	LLDynamicArray<LLViewerObject*> count_objects;
	
	// Add the root object to the list
	count_objects.put(object);
	
	// Iterate over all of this objects children
	LLViewerObject::child_list_t child_list = object->getChildren();
	
	for (LLViewerObject::child_list_t::iterator i = child_list.begin(); i != child_list.end(); ++i)
	{
		LLViewerObject* child = *i;
		if(!child->isAvatar())
		{
			// Put the child objects on the export list
			count_objects.put(child);
		}
	}
		
	S32 object_index = 0;

	while ((object_index < count_objects.count()))
	{
		object = count_objects.get(object_index++);
		LLUUID id = object->getID();
		objIDS.insert(id.asString());
		llinfos << "Counting scripts in prim " << object->getID().asString() << llendl;
		object->registerInventoryListener(sInstance,NULL);
		object->dirtyInventory();
		object->requestInventory();
		invqueries += 1;
	}
}

void ScriptCounter::completechk()
{
	if(invqueries == 0)
	{
		std::stringstream sstr;
		sstr << "Scripts counted. Result: ";
		sstr << scriptcount;
		cmdline_printchat(sstr.str());
		scriptcount=0;
		objIDS.clear();
		init();
	}
}

void ScriptCounter::inventoryChanged(LLViewerObject* obj,
								 InventoryObjectList* inv,
								 S32 serial_num,
								 void* user_data)
{
	if(status == IDLE)
	{
		obj->removeInventoryListener(sInstance);
		return;
	}
	if(objIDS.find(obj->getID().asString()) != objIDS.end())
	{
		InventoryObjectList::const_iterator it = inv->begin();
		InventoryObjectList::const_iterator end = inv->end();
		for( ;	it != end;	++it)
		{
			LLInventoryObject* asset = (*it);
			if(asset)
			{
				if((asset->getType() == LLAssetType::AT_SCRIPT) || (asset->getType() == LLAssetType::AT_LSL_TEXT))
					scriptcount+=1;
			}
		}
		invqueries -= 1;
		objIDS.erase(obj->getID().asString());
		obj->removeInventoryListener(sInstance);
		completechk();
	}
}