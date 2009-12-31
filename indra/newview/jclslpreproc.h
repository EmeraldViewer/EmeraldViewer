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

#include "llviewerprecompiledheaders.h"

#include "llpreviewscript.h"

#if LL_WINDOWS
#include "hunspell/hunspelldll.h"
#else
#include "hunspell.hxx"
#endif
/*class JCLSLPreprocessorCallback : public LLRefCount
{
public:
	virtual void fire(std::string output) = 0;
};*/

class JCLSLPreprocessor
{
public:

	JCLSLPreprocessor(LLScriptEdCore* corep)
		: mCore(corep), waving(FALSE), mClose(FALSE)/*,
		state(IDLE),
		last(0),
		groupcounter(0),
		updated(0)
		*/{}

	static BOOL mono_directive(std::string& text, bool agent_inv = true);

	std::string encode(std::string script);
	std::string decode(std::string script);

	std::string lslopt(std::string script);


	std::vector<std::string> scan_includes(std::string filename, std::string script);

	static LLUUID findInventoryByName(std::string name);

	static void JCProcCacheCallback(LLVFS *vfs, const LLUUID& uuid, LLAssetType::EType type, void *userdata, S32 result, LLExtStat extstat);

	void preprocess_script(BOOL close = FALSE, BOOL defcache = FALSE);
	void start_process();

	//void getfuncmatches(std::string token);

	std::set<std::string> caching_files;
	std::set<std::string> cached_files;

	std::set<std::string> defcached_files;

	BOOL mDefinitionCaching;

	std::map<std::string,std::string> cached_assetids;

	LLScriptEdCore* mCore;

	BOOL waving;

	BOOL mClose;

	static Hunspell* LSLHunspell;
};