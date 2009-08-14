/* Copyright (c) 2009
 *
 * Modular Systems Ltd. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY MODULAR SYSTEMS LTD AND CONTRIBUTORS “AS IS”
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

#include "exporttracker.h"
#include "llviewerobjectlist.h"
#include "llvoavatar.h"
#include "llsdutil.h"
#include "llsdserialize.h"
#include "lldirpicker.h"
#include "llfilepicker.h"

JCExportTracker* JCExportTracker::sInstance;
LLDynamicArray<LLSD*> JCExportTracker::primitives;
LLSD JCExportTracker::data;
U32 JCExportTracker::totalprims;
U32 JCExportTracker::propertyqueries;
U32 JCExportTracker::level;
U32 JCExportTracker::status;
JCExportTracker::JCExportTracker()
{
	llassert_always(sInstance == NULL);
	sInstance = this;

}

JCExportTracker::~JCExportTracker()
{
	sInstance = NULL;
}

void JCExportTracker::init()
{
	if(!sInstance)
	{
		sInstance = new JCExportTracker();
	}
	status = IDLE;
	level = PROPERTIES;//DEFAULT;
	propertyqueries = 0;
	totalprims = 0;
	data = LLSD();
	primitives.clear();
}

LLVOAvatar* find_avatar_from_object( LLViewerObject* object );

LLVOAvatar* find_avatar_from_object( const LLUUID& object_id );

void propreq(LLViewerObject* prim)
{
	gMessageSystem->newMessageFast(_PREHASH_ObjectSelect);
	gMessageSystem->nextBlockFast(_PREHASH_AgentData);
	gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	gMessageSystem->nextBlockFast(_PREHASH_ObjectData);
	gMessageSystem->addU32Fast(_PREHASH_ObjectLocalID, prim->getLocalID());
	gAgent.sendReliableMessage();
}

LLSD JCExportTracker::subserialize(LLViewerObject* linkset)
{
	//Chalice - Changed to support exporting linkset groups.
	LLViewerObject* object = linkset;
	//if(!linkset)return LLSD();
	
	// Create an LLSD object that will hold the entire tree structure that can be serialized to a file
	LLSD llsd;

	//if (!node)
	//	return llsd;

	//object = root_object = node->getObject();
	
	if (!object)
		return llsd;
	if(!(!object->isAvatar() && object->permYouOwner() && object->permModify() && object->permCopy() && object->permTransfer()))
		return llsd;
	// Build a list of everything that we'll actually be exporting
	LLDynamicArray<LLViewerObject*> export_objects;
	
	// Add the root object to the export list
	export_objects.put(object);
	
	// Iterate over all of this objects children
	LLViewerObject::child_list_t child_list = object->getChildren();
	
	for (LLViewerObject::child_list_t::iterator i = child_list.begin(); i != child_list.end(); ++i)
	{
		LLViewerObject* child = *i;
		if(!child->isAvatar())
		{
			// Put the child objects on the export list
			export_objects.put(child);
		}
	}
		
	S32 object_index = 0;
	
	while ((object_index < export_objects.count()))
	{
		object = export_objects.get(object_index++);
		LLUUID id = object->getID();
	
		llinfos << "Exporting prim " << object->getID().asString() << llendl;
	
		// Create an LLSD object that represents this prim. It will be injected in to the overall LLSD
		// tree structure
		LLSD prim_llsd;
	
		if (object_index == 1)
		{
			LLVOAvatar* avatar = find_avatar_from_object(object);
			if (avatar)
			{
				LLViewerJointAttachment* attachment = avatar->getTargetAttachmentPoint(object);
				U8 attachment_id = 0;
				
				if (attachment)
				{
					for (LLVOAvatar::attachment_map_t::iterator iter = avatar->mAttachmentPoints.begin();
										iter != avatar->mAttachmentPoints.end(); ++iter)
					{
						if (iter->second == attachment)
						{
							attachment_id = iter->first;
							break;
						}
					}
				}
				else
				{
					// interpret 0 as "default location"
					attachment_id = 0;
				}
				
				prim_llsd["Attachment"] = attachment_id;
				prim_llsd["attachpos"] = object->getPosition().getValue();
				prim_llsd["attachrot"] = ll_sd_from_quaternion(object->getRotation());
			}
			
			prim_llsd["position"] = LLVector3(0, 0, 0).getValue();
			prim_llsd["rotation"] = ll_sd_from_quaternion(object->getRotation());
		}
		else
		{
			prim_llsd["position"] = object->getPosition().getValue();
			prim_llsd["rotation"] = ll_sd_from_quaternion(object->getRotation());
		}
		prim_llsd["name"] = "";//node->mName;
		prim_llsd["description"] = "";//node->mDescription;
		// Transforms
		prim_llsd["scale"] = object->getScale().getValue();
		// Flags
		prim_llsd["shadows"] = object->flagCastShadows();
		prim_llsd["phantom"] = object->flagPhantom();
		prim_llsd["physical"] = (BOOL)(object->mFlags & FLAGS_USE_PHYSICS);
		LLVolumeParams params = object->getVolume()->getParams();
		prim_llsd["volume"] = params.asLLSD();
		if (object->isFlexible())
		{
			LLFlexibleObjectData* flex = (LLFlexibleObjectData*)object->getParameterEntry(LLNetworkData::PARAMS_FLEXIBLE);
			prim_llsd["flexible"] = flex->asLLSD();
		}
		if (object->getParameterEntryInUse(LLNetworkData::PARAMS_LIGHT))
		{
			LLLightParams* light = (LLLightParams*)object->getParameterEntry(LLNetworkData::PARAMS_LIGHT);
			prim_llsd["light"] = light->asLLSD();
		}
		if (object->getParameterEntryInUse(LLNetworkData::PARAMS_SCULPT))
		{
			LLSculptParams* sculpt = (LLSculptParams*)object->getParameterEntry(LLNetworkData::PARAMS_SCULPT);
			prim_llsd["sculpt"] = sculpt->asLLSD();
		}

		// Textures
		LLSD te_llsd;
		U8 te_count = object->getNumTEs();
		
		for (U8 i = 0; i < te_count; i++)
		{
			te_llsd.append(object->getTE(i)->asLLSD());
		}
		
		prim_llsd["textures"] = te_llsd;

		prim_llsd["id"] = object->getID().asString();
		if(level >= PROPERTIES)
		{
			propertyqueries += 1;
			propreq(object);
		}
		totalprims += 1;

		// Changed to use link numbers zero-indexed.
		llsd[object_index - 1] = prim_llsd;
		primitives.put(&llsd[object_index - 1]);
	}
	return llsd;
}

bool JCExportTracker::serializeSelection()
{
	init();
	LLDynamicArray<LLViewerObject*> catfayse;
	for (LLObjectSelection::valid_root_iterator iter = LLSelectMgr::getInstance()->getSelection()->valid_root_begin();
			 iter != LLSelectMgr::getInstance()->getSelection()->valid_root_end(); iter++)
	{
		LLSelectNode* selectNode = *iter;
		LLViewerObject* object = selectNode->getObject();
		if(object)catfayse.put(object);
	}
	return serialize(catfayse);
}

bool JCExportTracker::serialize(LLDynamicArray<LLViewerObject*> objects)
{

	init();

	LLSD total;
	U32 count = 0;
	LLVector3 first_pos;

	bool success = true;

	for(LLDynamicArray<LLViewerObject*>::iterator itr = objects.begin(); itr != objects.end(); ++itr)
	{
		LLViewerObject* object = *itr;
		if(!(!object->isAvatar() && object->permYouOwner() && object->permModify() && object->permCopy() && object->permTransfer()))
		{
			success = false;
			break;
		}
		LLVector3 object_pos = object->getPosition();
		LLSD origin;
		if(count == 0)
		{
			first_pos = object_pos;
			origin["ObjectPos"] = LLVector3(0.0f,0.0f,0.0f).getValue();
			total["Origin"] = object_pos.getValue();//for use in region origin import?
		}
		else
		{
			origin["ObjectPos"] = (object_pos - first_pos).getValue();
		}
		if (object)//impossible condition, you check avatar above//&& !(object->isAvatar()))
		{
			LLSD linkset = subserialize(object);

			if(!linkset.isUndefined())origin["Object"] = linkset;

			total[count] = origin;
		}
		count += 1;
	}

	if(success && !total.isUndefined() && level == DEFAULT)
	{
		finalize(total);
	}else
	{
		status = SERVERSIDE;
		data = total;
	}

	return success;

}

void JCExportTracker::finalize(LLSD data)
{
	LLSD file;
	LLSD header;
	header["Version"] = 2;//for now
	file["Header"] = header;
	file["Objects"] = data;

	LLFilePicker& file_picker = LLFilePicker::instance();
		
	if (!file_picker.getSaveFile(LLFilePicker::FFSAVE_XML))
		return; // User canceled save.
		
	std::string file_name = file_picker.getFirstFile();
	// Create a file stream and write to it
	llofstream export_file(file_name);
	LLSDSerialize::toPrettyXML(file, export_file);
	// Open the file save dialog
	export_file.close();
}
void cmdline_printchat(std::string message);
//LLSD* chkdata(LLUUID id, LLSD* data)
//{
//	if(primitives
//}
LLSD* JCExportTracker::getprim(LLUUID id)
{
	std::string idstr = id.asString();
	LLDynamicArray<LLSD*>::iterator iter;
	for(iter = primitives.begin(); iter != primitives.end(); ++iter)
	{
		LLSD* prim = *iter;
		if(prim->has("id"))
		{
			if((*prim)["id"].asString() == idstr)return prim;
		}
	}
	cmdline_printchat("no match for"+idstr);
	return NULL;
}
void JCExportTracker::processObjectProperties(LLMessageSystem* msg, void** user_data)
{
	if(level < PROPERTIES)
	{
		cmdline_printchat("level < PROPERTIES");
		return;
	}
	if(status == IDLE)
	{
		cmdline_printchat("status == IDLE");
		return;
	}
	S32 i;
	S32 count = msg->getNumberOfBlocksFast(_PREHASH_ObjectData);
	for (i = 0; i < count; i++)
	{
		LLUUID id;
		msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_ObjectID, id, i);
		LLSD* prim = getprim(id);
		if(prim)
		{
			cmdline_printchat("matched "+id.asString());
			LLUUID creator_id;
			LLUUID owner_id;
			LLUUID group_id;
			LLUUID last_owner_id;
			U64 creation_date;
			LLUUID extra_id;
			U32 base_mask, owner_mask, group_mask, everyone_mask, next_owner_mask;
			LLSaleInfo sale_info;
			LLCategory category;
			LLAggregatePermissions ag_perms;
			LLAggregatePermissions ag_texture_perms;
			LLAggregatePermissions ag_texture_perms_owner;
			
			msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_CreatorID, creator_id, i);
			msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_OwnerID, owner_id, i);
			msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_GroupID, group_id, i);
			msg->getU64Fast(_PREHASH_ObjectData, _PREHASH_CreationDate, creation_date, i);
			msg->getU32Fast(_PREHASH_ObjectData, _PREHASH_BaseMask, base_mask, i);
			msg->getU32Fast(_PREHASH_ObjectData, _PREHASH_OwnerMask, owner_mask, i);
			msg->getU32Fast(_PREHASH_ObjectData, _PREHASH_GroupMask, group_mask, i);
			msg->getU32Fast(_PREHASH_ObjectData, _PREHASH_EveryoneMask, everyone_mask, i);
			msg->getU32Fast(_PREHASH_ObjectData, _PREHASH_NextOwnerMask, next_owner_mask, i);
			sale_info.unpackMultiMessage(msg, _PREHASH_ObjectData, i);

			ag_perms.unpackMessage(msg, _PREHASH_ObjectData, _PREHASH_AggregatePerms, i);
			ag_texture_perms.unpackMessage(msg, _PREHASH_ObjectData, _PREHASH_AggregatePermTextures, i);
			ag_texture_perms_owner.unpackMessage(msg, _PREHASH_ObjectData, _PREHASH_AggregatePermTexturesOwner, i);
			category.unpackMultiMessage(msg, _PREHASH_ObjectData, i);

			S16 inv_serial = 0;
			msg->getS16Fast(_PREHASH_ObjectData, _PREHASH_InventorySerial, inv_serial, i);

			LLUUID item_id;
			msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_ItemID, item_id, i);
			LLUUID folder_id;
			msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_FolderID, folder_id, i);
			LLUUID from_task_id;
			msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_FromTaskID, from_task_id, i);

			msg->getUUIDFast(_PREHASH_ObjectData, _PREHASH_LastOwnerID, last_owner_id, i);

			std::string name;
			msg->getStringFast(_PREHASH_ObjectData, _PREHASH_Name, name, i);
			std::string desc;
			msg->getStringFast(_PREHASH_ObjectData, _PREHASH_Description, desc, i);

			std::string touch_name;
			msg->getStringFast(_PREHASH_ObjectData, _PREHASH_TouchName, touch_name, i);
			std::string sit_name;
			msg->getStringFast(_PREHASH_ObjectData, _PREHASH_SitName, sit_name, i);

			//unpack TE IDs
			std::vector<LLUUID> texture_ids;
			S32 size = msg->getSizeFast(_PREHASH_ObjectData, i, _PREHASH_TextureID);
			if (size > 0)
			{
				S8 packed_buffer[SELECT_MAX_TES * UUID_BYTES];
				msg->getBinaryDataFast(_PREHASH_ObjectData, _PREHASH_TextureID, packed_buffer, 0, i, SELECT_MAX_TES * UUID_BYTES);

				for (S32 buf_offset = 0; buf_offset < size; buf_offset += UUID_BYTES)
				{
					LLUUID tid;
					memcpy(tid.mData, packed_buffer + buf_offset, UUID_BYTES);		/* Flawfinder: ignore */
					texture_ids.push_back(tid);
				}
			}
			(*prim)["creator_id"] = creator_id.asString();
			(*prim)["owner_id"] = owner_id.asString();
			(*prim)["group_id"] = group_id.asString();
			(*prim)["last_owner_id"] = last_owner_id.asString();
			(*prim)["creation_date"] = llformat("%d",creation_date);
			(*prim)["extra_id"] = extra_id.asString();
			(*prim)["base_mask"] = llformat("%d",base_mask);
			(*prim)["owner_mask"] = llformat("%d",owner_mask);
			(*prim)["group_mask"] = llformat("%d",group_mask);
			(*prim)["everyone_mask"] = llformat("%d",everyone_mask);
			(*prim)["next_owner_mask"] = llformat("%d",next_owner_mask);
			(*prim)["sale_info"] = sale_info.asLLSD();

			(*prim)["inv_serial"] = (S32)inv_serial;
			(*prim)["item_id"] = item_id.asString();
			(*prim)["folder_id"] = folder_id.asString();
			(*prim)["from_task_id"] = from_task_id.asString();
			(*prim)["name"] = name;
			(*prim)["description"] = desc;
			(*prim)["touch_name"] = touch_name;
			(*prim)["sit_name"] = sit_name;

			propertyqueries -= 1;
			cmdline_printchat(llformat("%d queries left",propertyqueries));
			if(propertyqueries == 0)
			{
				cmdline_printchat("done nigga");
				finalize(data);
			}
		}
	}
}

