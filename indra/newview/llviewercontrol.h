/** 
 * @file llviewercontrol.h
 * @brief references to viewer-specific control files
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

#ifndef LL_LLVIEWERCONTROL_H
#define LL_LLVIEWERCONTROL_H

#include <map>
#include "llcontrol.h"

// Enabled this definition to compile a 'hacked' viewer that
// allows a hacked godmode to be toggled on and off.
#define TOGGLE_HACKED_GODLIKE_VIEWER 
#ifdef TOGGLE_HACKED_GODLIKE_VIEWER
extern BOOL gHackGodmode;
#endif

// These functions found in llcontroldef.cpp *TODO: clean this up!
//setting variables are declared in this function
void settings_setup_listeners();
static BOOL sFreezeTime;

extern std::map<std::string, LLControlGroup*> gSettings;

// for the graphics settings
void create_graphics_group(LLControlGroup& group);

// saved at end of session
extern LLControlGroup gSavedSettings;
extern LLControlGroup gSavedPerAccountSettings;

// Read-only
extern LLControlGroup gColors;

// Saved at end of session
extern LLControlGroup gCrashSettings;

// Set after settings loaded
extern std::string gLastRunVersion;
extern std::string gCurrentVersion;

//! Helper function for LLCachedControl
template <class T> 
eControlType get_control_type(const T& in, LLSD& out)
{
	llerrs << "Usupported control type: " << typeid(T).name() << "." << llendl;
	return TYPE_COUNT;
}

//! Publish/Subscribe object to interact with LLControlGroups.

//! An LLCachedControl instance to connect to a LLControlVariable
//! without have to manually create and bind a listener to a local
//! object.
template <class T>
class LLCachedControl
{
    T mCachedValue;
    LLPointer<LLControlVariable> mControl;
    boost::signals::connection mConnection;

public:
	LLCachedControl(const std::string& name, 
					const T& default_value, 
					const std::string& comment = "Declared In Code")
	{
		mControl = gSavedSettings.getControl(name);
		if(mControl.isNull())
		{
			declareTypedControl(gSavedSettings, name, default_value, comment);
			mControl = gSavedSettings.getControl(name);
			if(mControl.isNull())
			{
				llerrs << "The control could not be created!!!" << llendl;
			}

			mCachedValue = default_value;
		}
		else
		{
			mCachedValue = (const T&)mControl->getValue();
		}

		// Add a listener to the controls signal...
		mControl->getSignal()->connect(
			boost::bind(&LLCachedControl<T>::handleValueChange, this, _1)
			);
	}

	~LLCachedControl()
	{
		if(mConnection.connected())
		{
			mConnection.disconnect();
		}
	}

	LLCachedControl& operator =(const T& newvalue)
	{
	   setTypeValue(*mControl, newvalue);
	}

	operator const T&() { return mCachedValue; }

private:
	void declareTypedControl(LLControlGroup& group, 
							 const std::string& name, 
							 const T& default_value,
							 const std::string& comment)
	{
		LLSD init_value;
		eControlType type = get_control_type<T>(default_value, init_value);
		if(type < TYPE_COUNT)
		{
			group.declareControl(name, type, init_value, comment, FALSE);
		}
	}

	bool handleValueChange(const LLSD& newvalue)
	{
		mCachedValue = (const T &)newvalue;
		return true;
	}

	void setTypeValue(LLControlVariable& c, const T& v)
	{
		// Implicit conversion from T to LLSD...
		c.set(v);
	}
};

template <> eControlType get_control_type<U32>(const U32& in, LLSD& out);
template <> eControlType get_control_type<S32>(const S32& in, LLSD& out);
template <> eControlType get_control_type<F32>(const F32& in, LLSD& out);
template <> eControlType get_control_type<bool> (const bool& in, LLSD& out); 
// Yay BOOL, its really an S32.
//template <> eControlType get_control_type<BOOL> (const BOOL& in, LLSD& out) 
template <> eControlType get_control_type<std::string>(const std::string& in, LLSD& out);
template <> eControlType get_control_type<LLVector3>(const LLVector3& in, LLSD& out);
template <> eControlType get_control_type<LLVector3d>(const LLVector3d& in, LLSD& out); 
template <> eControlType get_control_type<LLRect>(const LLRect& in, LLSD& out);
template <> eControlType get_control_type<LLColor4>(const LLColor4& in, LLSD& out);
template <> eControlType get_control_type<LLColor3>(const LLColor3& in, LLSD& out);
template <> eControlType get_control_type<LLColor4U>(const LLColor4U& in, LLSD& out); 
template <> eControlType get_control_type<LLSD>(const LLSD& in, LLSD& out);

//#define TEST_CACHED_CONTROL 1
#ifdef TEST_CACHED_CONTROL
void test_cached_control();
#endif // TEST_CACHED_CONTROL


///////////////////////
struct jcb
{
#ifdef LL_WINDOWS
#define JC_BIND_INLINE __forceinline
#else
#define JC_BIND_INLINE inline
#endif

#define JC_BIND_UPDN(x,y) x ## y

#define JC_BIND_UPDATER_GENERATE(JC_BIND_TYPE, JC_BIND_RETURN) \
JC_BIND_INLINE static void JC_BIND_UPDN(bind_llcontrol_updater_,JC_BIND_TYPE)(const LLSD &data, JC_BIND_TYPE* reciever){ *reciever = JC_BIND_RETURN; }

#define JC_BIND_FUNC_GENERATE(JC_BIND_FUNCNAME, JC_BIND_CONTROLGROUP, JC_BIND_TYPE, JC_BIND_RETURN) \
JC_BIND_INLINE static bool JC_BIND_FUNCNAME(const char* name, JC_BIND_TYPE* reciever, bool init) \
{ \
	if(name) \
	{ \
		return JC_BIND_FUNCNAME(std::string(name), reciever, init); \
	} \
	return false; \
} \
JC_BIND_INLINE static bool JC_BIND_FUNCNAME(std::string& name, JC_BIND_TYPE* reciever, bool init) \
{ \
	LLControlVariable* var = JC_BIND_CONTROLGROUP.getControl(name); \
	if(var) \
	{ \
		boost::signal<void(const LLSD&)>* signal = var->getSignal(); \
		if(signal) \
		{ \
			signal->connect(boost::bind(&JC_BIND_UPDN(bind_llcontrol_updater_,JC_BIND_TYPE), _1, reciever)); \
			if(init)jcb::JC_BIND_UPDN(bind_llcontrol_updater_,JC_BIND_TYPE)(var->getValue(),reciever); \
			return true; \
		} \
	} \
	return false; \
}

typedef std::string jcb_string;//hack around :: in function names due to macro expansion

JC_BIND_UPDATER_GENERATE(S32, data.asInteger())
JC_BIND_UPDATER_GENERATE(F32, data.asReal())
JC_BIND_UPDATER_GENERATE(U32, data.asInteger())
JC_BIND_UPDATER_GENERATE(jcb_string, data.asString())
JC_BIND_UPDATER_GENERATE(LLVector3, data)
JC_BIND_UPDATER_GENERATE(LLVector3d, data)
JC_BIND_UPDATER_GENERATE(LLRect, data)
JC_BIND_UPDATER_GENERATE(LLColor4U, data)
JC_BIND_UPDATER_GENERATE(LLColor4, data)
JC_BIND_UPDATER_GENERATE(LLSD, data)

#define JC_BIND_FUNCS_GENERATE(JC_BIND_FUNCNAME, JC_BIND_CONTROLGROUP) \
JC_BIND_FUNC_GENERATE(JC_BIND_FUNCNAME, JC_BIND_CONTROLGROUP, S32, data.asInteger()) \
JC_BIND_FUNC_GENERATE(JC_BIND_FUNCNAME, JC_BIND_CONTROLGROUP, F32, data.asReal()) \
JC_BIND_FUNC_GENERATE(JC_BIND_FUNCNAME, JC_BIND_CONTROLGROUP, U32, data.asInteger()) \
JC_BIND_FUNC_GENERATE(JC_BIND_FUNCNAME, JC_BIND_CONTROLGROUP, jcb_string, data.asString()) \
JC_BIND_FUNC_GENERATE(JC_BIND_FUNCNAME, JC_BIND_CONTROLGROUP, LLVector3, data) \
JC_BIND_FUNC_GENERATE(JC_BIND_FUNCNAME, JC_BIND_CONTROLGROUP, LLVector3d, data) \
JC_BIND_FUNC_GENERATE(JC_BIND_FUNCNAME, JC_BIND_CONTROLGROUP, LLRect, data) \
JC_BIND_FUNC_GENERATE(JC_BIND_FUNCNAME, JC_BIND_CONTROLGROUP, LLColor4U, data) \
JC_BIND_FUNC_GENERATE(JC_BIND_FUNCNAME, JC_BIND_CONTROLGROUP, LLColor4, data) \
JC_BIND_FUNC_GENERATE(JC_BIND_FUNCNAME, JC_BIND_CONTROLGROUP, LLSD, data)

JC_BIND_FUNCS_GENERATE(bind_gsavedsetting, gSavedSettings)

JC_BIND_FUNCS_GENERATE(bind_gsavedperaccountsetting, gSavedPerAccountSettings)

JC_BIND_FUNCS_GENERATE(bind_gcolor, gColors)

//JC_BIND_FUNC_GENERATE(BOOL, data.asBoolean())
//equivalent to S32
//JC_BIND_FUNC_GENERATE(LLColor3, data)
//effectively equivalent to LLColor4

#undef JC_BIND_INLINE
#undef JC_BIND_UPDATER_GENERATE
#undef JC_BIND_FUNC_GENERATE
#undef JC_BIND_FUNCS_GENERATE
#undef JC_BIND_UPDN
#define bind_gsavedsetting jcb::bind_gsavedsetting
#define bind_gsavedperaccountsetting jcb::bind_gsavedperaccountsetting
#define bind_gcolor jcb::bind_gcolor
};
///////////////////////





#endif // LL_LLVIEWERCONTROL_H
