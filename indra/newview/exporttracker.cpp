

#include "llviewerprecompiledheaders.h"

#include "exporttracker.h"
#include "llviewerobjectlist.h"
#include "llvoavatar.h"
#include "llsdutil.h"
#include "llsdserialize.h"
#include "lldirpicker.h"
#include "llfilepicker.h"

JCExportTracker* JCExportTracker::sInstance;

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
	if(!sInstance)sInstance = new JCExportTracker();
}

LLVOAvatar* find_avatar_from_object( LLViewerObject* object );

LLVOAvatar* find_avatar_from_object( const LLUUID& object_id );

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

		// Changed to use link numbers zero-indexed.
		llsd[object_index - 1] = prim_llsd;
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

	if(success && !total.isUndefined())
	{
		LLSD file;
		LLSD header;
		header["Version"] = 2;//for now
		file["Header"] = header;
		file["Objects"] = total;

		LLFilePicker& file_picker = LLFilePicker::instance();
			
		if (!file_picker.getSaveFile(LLFilePicker::FFSAVE_XML))
			return false; // User canceled save.
			
		std::string file_name = file_picker.getFirstFile();
		// Create a file stream and write to it
		llofstream export_file(file_name);

		LLSDSerialize::toPrettyXML(file, export_file);
		// Open the file save dialog
		export_file.close();
	}

	return success;

}