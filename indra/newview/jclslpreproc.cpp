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
 * THIS SOFTWARE IS PROVIDED BY MODULAR SYSTEMS AND CONTRIBUTORS �AS IS�
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

#include "jclslpreproc.h"

#include "llagent.h"

#include "emerald.h"

#include "llcurl.h"

#include "llscrolllistctrl.h"

#include "llviewertexteditor.h"

#include "llinventorymodel.h"

#include "llversionviewer.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>


//apparently LL #defined this function which happens to precisely match
//a boost::wave function name, destroying the internet, silly grey furries
#undef equivalent

#include <boost/assert.hpp>
#include <boost/program_options.hpp>

#include <boost/wave.hpp>


#include <boost/wave/cpplexer/cpp_lex_token.hpp>    // token class
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp> // lexer class

#include <boost/wave/preprocessing_hooks.hpp>

///////////////////////////////////////////////////////////////////////////////
//  include lexer specifics, import lexer names
#if BOOST_WAVE_SEPARATE_LEXER_INSTANTIATION == 0
#include <boost/wave/cpplexer/re2clex/cpp_re2c_lexer.hpp>
#endif 

#include <boost/regex.hpp>

#include <boost/filesystem.hpp>

using namespace boost::spirit::classic;
namespace po = boost::program_options;


using namespace boost::regex_constants;

void cmdline_printchat(std::string message);

#define encode_start std::string("//start_unprocessed_text\n/*")
#define encode_end std::string("*/\n//end_unprocessed_text")

std::string JCLSLPreprocessor::encode(std::string script)
{

	std::string otext = JCLSLPreprocessor::decode(script);

	otext = boost::regex_replace(otext, boost::regex("([/*])(?=[/*|])",boost::regex::perl), "$1|");

	//otext = curl_escape(otext.c_str(), otext.size());

	otext = encode_start+otext+encode_end;
	otext += "\n//nfo_preprocessor_version 0";
	otext += "\n//^ = determine what featureset is supported";
	std::string version = llformat("%d.%d.%d (%d)",
						LL_VERSION_MAJOR, LL_VERSION_MINOR, LL_VERSION_PATCH, LL_VERSION_BUILD);
	otext += llformat("\n//nfo_program_version %s %s",LL_CHANNEL,version.c_str());

	return otext;
}

std::string JCLSLPreprocessor::decode(std::string script)
{
	static S32 startpoint = encode_start.length();
	std::string tip = script.substr(0,startpoint);
	if(tip != encode_start)
	{
		//cmdline_printchat("no start");
		//if(sp != -1)trigger warningg/error?
		return script;
	}

	S32 end = script.find(encode_end);
	if(end == -1)
	{
		//cmdline_printchat("no end");
		return script;
	}


	std::string data = script.substr(startpoint,end-startpoint);
	//cmdline_printchat("data="+data);

	std::string otext = data;

	otext = boost::regex_replace(otext, boost::regex("([/*])\\|",boost::regex::perl), "$1");

	//otext = curl_unescape(otext.c_str(),otext.length());

	return otext;
}


struct trace_include_files : public boost::wave::context_policies::default_preprocessing_hooks
{
	trace_include_files(JCLSLPreprocessor* proc) 
    :   mProc(proc) 
    {
		mAssetStack.push(LLUUID::null.asString());
	}


template <typename ContextT>
    bool found_include_directive(ContextT const& ctx, 
        std::string const &filename, bool include_next)
	{
		std::string cfilename = filename.substr(1,filename.length()-2);
		cmdline_printchat(cfilename+":found_include_directive");
        std::set<std::string>::iterator it = mProc->cached_files.find(cfilename);
        if (it == mProc->cached_files.end())
		{
			std::set<std::string>::iterator it = mProc->caching_files.find(cfilename);
			if (it == mProc->caching_files.end())
			{
				LLUUID item_id = JCLSLPreprocessor::findInventoryByName(cfilename);
				if(item_id.notNull())
				{
					LLViewerInventoryItem* item = gInventory.getItem(item_id);
					if(item)
					{
						mProc->caching_files.insert(cfilename);
						//mProc->mCore->mErrorList->addCommentText(std::string("Caching ")+cfilename);
						ProcCacheInfo* info = new ProcCacheInfo;
						info->item = item;
						info->self = mProc;
						LLPermissions perm(((LLInventoryItem*)item)->getPermissions());
						gAssetStorage->getInvItemAsset(LLHost::invalid,
						gAgent.getID(),
						gAgent.getSessionID(),
						perm.getOwner(),
						LLUUID::null,
						item->getUUID(),
						LLUUID::null,
						item->getType(),
						&JCLSLPreprocessor::JCProcCacheCallback,
						info,
						TRUE);
						return true;
					}//else mProc->mCore->mErrorList->addCommentText(std::string("Bad item? ("+cfilename+")"));
				}//else mProc->mCore->mErrorList->addCommentText(std::string("Unable to find "+cfilename+" in agent inventory."));
				cmdline_printchat(cfilename+":bad item or null include");
			}else
			{
				cmdline_printchat(cfilename+":being cached but we still hit it somehow");
				return true;
			}
        }else
		{
			cmdline_printchat(cfilename+":cached item");
		}
        //++include_depth;
		return false;
    }

template <typename ContextT>
	void opened_include_file(ContextT const& ctx, 
		std::string const &relname, std::string const& absname,
		bool is_system_include)
	{
		ContextT& usefulctx = const_cast<ContextT&>(ctx);
		std::string id;
		std::string filename = boost::filesystem::path(std::string(relname)).filename();
		std::set<std::string>::iterator it = mProc->cached_files.find(filename);
		if(it != mProc->cached_files.end())
		{
			id = mProc->cached_assetids[filename];
		}else id = "NOT_IN_WORLD";//I guess, still need to add external includes atm
		mAssetStack.push(id);
		std::string macro = "__ASSETID__";
		usefulctx.remove_macro_definition(macro, true);
		std::string def = llformat("%s=\"%s\"",macro.c_str(),id.c_str());
		usefulctx.add_macro_definition(def,false);
	}


template <typename ContextT>
	void returning_from_include_file(ContextT const& ctx)
	{
		ContextT& usefulctx = const_cast<ContextT&>(ctx);
		if(mAssetStack.size() > 1)
		{
			mAssetStack.pop();
			std::string id = mAssetStack.top();
			std::string macro = "__ASSETID__";
			usefulctx.remove_macro_definition(macro, true);
			std::string def = llformat("%s=\"%s\"",macro.c_str(),id.c_str());
			usefulctx.add_macro_definition(def,false);
		}//else wave did something really fucked up
	}

    /*template <typename ContextT>
    void returning_from_include_file(ContextT const& ctx) 
    {
        --include_depth;
    }*/
    JCLSLPreprocessor* mProc;

	std::stack<std::string> mAssetStack;

	//ContextT *usefulctx;
    //std::size_t include_depth;
	//stroutputfunc lolcall;
};

//typedef void (*stroutputfunc)(std::string message);

std::string cachepath(std::string name)
{
	return gDirUtilp->getExpandedFilename(LL_PATH_CACHE,"lslpreproc",name);
}

void cache_script(std::string name, std::string content)
{
	cmdline_printchat("writing "+name+" to cache");
	std::string path = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,"lslpreproc",name);
	LLAPRFile infile;
	infile.open(path.c_str(), LL_APR_WB);
	apr_file_t *fp = infile.getFileHandle();
	if(fp)infile.write(content.c_str(), content.length());
	infile.close();
}

struct ProcCacheInfo
{
	LLViewerInventoryItem* item;
	JCLSLPreprocessor* self;
};

void JCLSLPreprocessor::JCProcCacheCallback(LLVFS *vfs, const LLUUID& uuid, LLAssetType::EType type, void *userdata, S32 result, LLExtStat extstat)
{
	cmdline_printchat("cachecallback called");
	ProcCacheInfo* info =(ProcCacheInfo*)userdata;
	LLViewerInventoryItem* item = info->item;
	JCLSLPreprocessor* self = info->self;
	if(item && self)
	{
		std::string name = item->getName();
		if(result == LL_ERR_NOERR)
		{
			LLVFile file(vfs, uuid, type);
			S32 file_length = file.getSize();
			char* buffer = new char[file_length+1];
			file.read((U8*)buffer, file_length);
			// put a EOS at the end
			buffer[file_length] = 0;

			std::string content(buffer);
			content = utf8str_removeCRLF(content);
			content = self->decode(content);
			/*content += llformat("\n#define __UP_ITEMID__ __ITEMID__\n#define __ITEMID__ %s\n",uuid.asString().c_str())+content;
			content += "\n#define __ITEMID__ __UP_ITEMID__\n";*/
			//prolly wont work and ill have to be not lazy, but worth a try
			delete buffer;
			if(boost::filesystem::native(name))
			{
				cmdline_printchat("native name of "+name);
				self->mCore->mErrorList->addCommentText(std::string("Cached ")+name);
				cache_script(name, content);
				std::set<std::string>::iterator loc = self->caching_files.find(name);
				if(loc != self->caching_files.end())
				{
					cmdline_printchat("finalizing cache");
					self->caching_files.erase(loc);
					self->cached_files.insert(name);
					self->cached_assetids[name] = uuid.asString();//.insert(uuid.asString());
					self->start_process();
				}else
				{
					cmdline_printchat("something fucked");
				}
			}else self->mCore->mErrorList->addCommentText(std::string("Error: script named '")+name+"' isn't safe to copy to the filesystem. This include will fail.");
		}else
		{
			self->mCore->mErrorList->addCommentText(std::string("Error caching "+name));
		}
	}
	if(info)delete info;
}

class ScriptMatches : public LLInventoryCollectFunctor
{
public:
	ScriptMatches(std::string name)
	{
		sName = name;
	}
	virtual ~ScriptMatches() {}
	virtual bool operator()(LLInventoryCategory* cat,
							LLInventoryItem* item)
	{
		if(item)
		{
			//LLViewerInventoryCategory* folderp = gInventory.getCategory((item->getParentUUID());
			if(item->getName() == sName)
			{
				if(item->getType() == LLAssetType::AT_LSL_TEXT)return true;
			}
			//return (item->getName() == sName);// && cat->getName() == "#v");
		}
		return false;
	}
private:
	std::string sName;
};

LLUUID JCLSLPreprocessor::findInventoryByName(std::string name)
{
	LLViewerInventoryCategory::cat_array_t cats;
	LLViewerInventoryItem::item_array_t items;
	ScriptMatches namematches(name);
	gInventory.collectDescendentsIf(gAgent.getInventoryRootID(),cats,items,FALSE,namematches);

	if (items.count())
	{
		return items[0]->getUUID();
	}
	return LLUUID::null;
}

void JCLSLPreprocessor::preprocess_script(BOOL close)
{
	mClose = close;
	cached_files.clear();
	caching_files.clear();
	cached_assetids.clear();
	mCore->mErrorList->addCommentText(std::string("Starting..."));
	LLFile::mkdir(gDirUtilp->getExpandedFilename(LL_PATH_CACHE,"")+gDirUtilp->getDirDelimiter()+"lslpreproc");
	std::string script = mCore->mEditor->getText();
	//this script is special
	LLViewerInventoryItem* item = mCore->mItem;
	std::string name = item->getName();
	cached_files.insert(name);
	cached_assetids[name] = LLUUID::null.asString();
	cache_script(name, script);
	//start the party
	start_process();
}

const std::string lazy_list_set_func("\
list lazy_list_set(list target, integer pos, list newval)\n\
{\n\
    integer end = llGetListLength(target);\n\
    if(end > pos)\n\
    {\n\
        target = llListReplaceList(target,newval,pos,pos);\n\
    }else if(end == pos)\n\
    {\n\
        target += newval;\n\
    }else\n\
    {\n\
        do\n\
        {\n\
            target += [0];\n\
            end += 1;\n\
        }while(end < pos);\n\
        target += newval;\n\
    }\n\
    return target;\n\
}\n\
");

std::string reformat_lazy_lists(std::string script)
{
	BOOL add_set = FALSE;
	std::string nscript = script;
	nscript = boost::regex_replace(nscript, boost::regex("([a-zA-Z0-9_]+)\\[([a-zA-Z0-9_()\"]+)]\\s*=\\s*([a-zA-Z0-9_()\"\\+\\-\\*/]+)([;)])",boost::regex::perl), "$1=lazy_list_set($1,$2,[$3])$4");
	if(nscript != script)add_set = TRUE;
	
	//...

	//boost::regex_search(
	

	if(add_set == TRUE)
	{
		//add lazy_list_set function to top of script, as it is used
		nscript = utf8str_removeCRLF(lazy_list_set_func) + "\n" + nscript;
	}


	return nscript;
}

void JCLSLPreprocessor::start_process()
{
	if(waving)
	{
		cmdline_printchat("already waving?");
		return;
	}
	waving = TRUE;
	cmdline_printchat("entering waving");
	//mCore->mErrorList->addCommentText(std::string("Completed caching."));

	boost::wave::util::file_position_type current_position;

	//static const std::string predefined_stuff('

	std::string input = mCore->mEditor->getText();
	std::string output;
	std::string name = mCore->mItem->getName();
	//BOOL use_switches;
	BOOL lazy_lists = FALSE;

	BOOL errored = FALSE;
	std::string err;
	try
	{	
		trace_include_files tracer(this);

		//  This token type is one of the central types used throughout the library, 
		//  because it is a template parameter to some of the public classes and  
		//  instances of this type are returned from the iterators.
		typedef boost::wave::cpplexer::lex_token<> token_type;
	    
		//  The template boost::wave::cpplexer::lex_iterator<> is the lexer type to
		//  to use as the token source for the preprocessing engine. It is 
		//  parametrized with the token type.
		typedef boost::wave::cpplexer::lex_iterator<token_type> lex_iterator_type;
	        
		//  This is the resulting context type to use. The first template parameter
		//  should match the iterator type to be used during construction of the
		//  corresponding context object (see below).
		typedef boost::wave::context<std::string::iterator, lex_iterator_type, boost::wave::iteration_context_policies::load_file_to_string, trace_include_files >
				context_type;

		context_type ctx(input.begin(), input.end(), name.c_str(), tracer);
		//tracer.usefulctx = &ctx;

		ctx.set_language(boost::wave::enable_long_long(ctx.get_language()));
		ctx.set_language(boost::wave::enable_preserve_comments(ctx.get_language()));
		ctx.set_language(boost::wave::enable_prefer_pp_numbers(ctx.get_language()));

		std::string path = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,"")+gDirUtilp->getDirDelimiter()+"lslpreproc"+gDirUtilp->getDirDelimiter();
		ctx.add_include_path(path.c_str());

		std::string def = llformat("__AGENTKEY__=\"%s\"",gAgent.getID().asString().c_str());//legacy because I used it earlier
		ctx.add_macro_definition(def,false);
		def = llformat("__AGENTID__=\"%s\"",gAgent.getID().asString().c_str());
		ctx.add_macro_definition(def,false);
		def = llformat("__AGENTIDRAW__=%s",gAgent.getID().asString().c_str());
		ctx.add_macro_definition(def,false);

		std::string aname;
		gAgent.getName(aname);
		def = llformat("__AGENTNAME__=\"%s\"",aname.c_str());
		ctx.add_macro_definition(def,false);
		def = llformat("__ASSETID__=%s",LLUUID::null.asString().c_str());
		ctx.add_macro_definition(def,false);
		//  Get the preprocessor iterators and use them to generate 
		//  the token sequence.
		context_type::iterator_type first = ctx.begin();
		context_type::iterator_type last = ctx.end();
	        

		//  The input stream is preprocessed for you while iterating over the range
		//  [first, last)
        while (first != last)
		{
			if(caching_files.size() != 0)
			{
				cmdline_printchat("caching somethin, exiting waving");
				mCore->mErrorList->addCommentText("Caching something...");
				waving = FALSE;
				first = last;
				return;
			}
            current_position = (*first).get_position();
			std::string token = std::string((*first).get_value().c_str());//stupid boost bitching even though we know its a std::string
			if(token == "#line")token = "//#line";
			//line directives are emitted in case the frontend of the future can find use for them
			output += token;
			if(lazy_lists == FALSE)lazy_lists = ctx.is_defined_macro(std::string("USE_LAZY_LISTS"));
			/*this needs to be checked for because if it is *ever* enabled it indicates that the transform is a dependency*/
            ++first;
        }
	}
	catch(boost::wave::cpp_exception const& e)
	{
		errored = TRUE;
		// some preprocessing error
		err = name + "(" + llformat("%d",e.line_no()) + "): " + e.description();
		cmdline_printchat(err);
		mCore->mErrorList->addCommentText(err);
	}
	catch(std::exception const& e)
	{
		errored = TRUE;
		err = std::string(current_position.get_file().c_str()) + "(" + llformat("%d",current_position.get_line()) + "): ";
		err += std::string("exception caught: ") + e.what();
		cmdline_printchat(err);
		mCore->mErrorList->addCommentText(err);
	}
	catch (...)
	{
		errored = TRUE;
		err = std::string(current_position.get_file().c_str()) + llformat("%d",current_position.get_line());
		err += std::string("): unexpected exception caught.");
		cmdline_printchat(err);
		mCore->mErrorList->addCommentText(err);
	}
	
	if(lazy_lists == TRUE)output = reformat_lazy_lists(output);
	//output = implement_switches(output);
	if(errored)
	{
		output += "\nPreprocessor exception:\n"+err;
	}
	output = encode(input)+"\n\n"+output;


	LLTextEditor* outfield = mCore->mPostEditor;//getChild<LLViewerTextEditor>("post_process");
	if(outfield)
	{
		outfield->setText(LLStringExplicit(output));
	}
	mCore->doSaveComplete((void*)mCore,mClose);
	waving = FALSE;
}