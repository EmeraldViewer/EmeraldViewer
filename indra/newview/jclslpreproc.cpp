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

//void cmdline_printchat(std::string message);

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


//typedef void (*stroutputfunc)(std::string message);
struct trace_include_files : public boost::wave::context_policies::default_preprocessing_hooks 
{
	trace_include_files(std::set<std::string> &files_) 
    :   files(files_) 
    {}


template <typename ContextT>
    bool found_include_directive(ContextT const& ctx, 
        std::string const &filename, bool include_next)
	{
		std::string cfilename = filename.substr(1,filename.length()-2);
        std::set<std::string>::iterator it = files.find(cfilename);
        if (it == files.end())
		{
			//std::string out;
            // print indented filename
            //for (std::size_t i = 0; i < include_depth; ++i)out += " ";
            //out += filename;
			//out += "\n";
			//if(lolcall)lolcall(out);
            files.insert(cfilename);
        }
        //++include_depth;
		return true;
    }

    /*template <typename ContextT>
    void returning_from_include_file(ContextT const& ctx) 
    {
        --include_depth;
    }*/
    std::set<std::string> &files;
    //std::size_t include_depth;
	//stroutputfunc lolcall;
};
std::vector<std::string> JCLSLPreprocessor::scan_includes(std::string filename, std::string script)
{
	mCore->mErrorList->addCommentText("Scanning "+filename+" for includes to cache");
	boost::wave::util::file_position_type current_position;
	std::set<std::string> files;
	try
	{
		// current file position is saved for exception handling

		typedef boost::wave::cpplexer::lex_iterator<
							boost::wave::cpplexer::lex_token<> >
						lex_iterator_type;
		typedef boost::wave::context<
		std::string::iterator, lex_iterator_type,
		boost::wave::iteration_context_policies::load_file_to_string,
		trace_include_files
		> context_type;

		//set<string> files;
		trace_include_files trace(files);

		//trace.lolcall = cmdline_printchat;

		context_type ctx(script.begin(), script.end(), "script", trace);

		context_type::iterator_type first = ctx.begin();
		context_type::iterator_type last = ctx.end();

		while (first != last)
		{
			current_position = (*first).get_position();
			++first;
		}
	}catch(boost::wave::cpp_exception const& e)
	{
		// some preprocessing error
		std::string err = filename + "(" + llformat("%d",e.line_no()) + "): " + e.description();
		mCore->mErrorList->addCommentText(err);
	}
	catch(std::exception const& e)
	{
		std::string err = std::string(current_position.get_file().c_str()) + "(" + llformat("%d",current_position.get_line()) + "): ";
		err += std::string("exception caught: ") + e.what();
		mCore->mErrorList->addCommentText(err);
	}
	catch (...)
	{
		std::string err = std::string(current_position.get_file().c_str()) + llformat("%d",current_position.get_line());
		err += std::string("): unexpected exception caught.");
		mCore->mErrorList->addCommentText(err);
	}

	std::vector<std::string> nfiles(files.begin(), files.end());
	return nfiles;
}

std::string cachepath(std::string name)
{
	return gDirUtilp->getExpandedFilename(LL_PATH_CACHE,"lslpreproc",name);
}

void cache_script(std::string name, std::string content)
{
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
				self->mCore->mErrorList->addCommentText(std::string("Cached ")+name);
				cache_script(name, content);

				std::vector<std::string>::iterator pos = std::find(self->caching_files.begin(), self->caching_files.end(), name);//self->caching_files.find(name);//self->caching_files.erase(
				if(pos != self->caching_files.end())
				{
					self->caching_files.erase(pos);
					self->cached_files.push_back(name);

					//we have to eliminate interfering directives that might otherwise break the scan cache
					//this will delete all directives but 
					//#import
					//#include
					//#include_next
					std::string scanscript = boost::regex_replace(content, boost::regex("#unassert|#warning|#pragma|#define|#assert|#ifndef|#undef|#ifdef|#ident|#endif|#error|#else|#line|#elif|#sccs|#if",boost::regex::perl), "");
					std::vector<std::string> files = self->scan_includes(name, scanscript);
					scanscript = "";
					if(self->cachecheck(files))
					{
						if(self->caching_files.size() == 0)self->start_process();
						else self->mCore->mErrorList->addCommentText(std::string("Files remaining: "+llformat("%d",self->caching_files.size())));
					}
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

BOOL JCLSLPreprocessor::cachecheck(std::vector<std::string> files)
{
	BOOL allcached = TRUE;
	for(int i = 0; i < (int)files.size(); i++)
	{
		std::string name = files[i].c_str();
		//llinfos << "Suggestion for " << testWord.c_str() << ":" << suggList[i].c_str() << llendl;
		//std::vector<std::string>::iterator it = cached_files.find(filename);
        if (std::find(cached_files.begin(), cached_files.end(), name) == cached_files.end())
		{
			if(std::find(caching_files.begin(), caching_files.end(), name) == caching_files.end())
			{
				LLUUID item_id = JCLSLPreprocessor::findInventoryByName(name);
				if(item_id.notNull())
				{
					LLViewerInventoryItem* item = gInventory.getItem(item_id);
					if(item)
					{
						caching_files.push_back(name);
						mCore->mErrorList->addCommentText(std::string("Caching ")+name);
						ProcCacheInfo* info = new ProcCacheInfo;
						info->item = item;
						info->self = this;
						LLPermissions perm(((LLInventoryItem*)item)->getPermissions());
						gAssetStorage->getInvItemAsset(LLHost::invalid,
						gAgent.getID(),
						gAgent.getSessionID(),
						perm.getOwner(),
						LLUUID::null,
						item->getUUID(),
						LLUUID::null,
						item->getType(),
						JCProcCacheCallback,
						info,
						TRUE);
					}else mCore->mErrorList->addCommentText(std::string("Bad item? ("+name+")"));
				}else mCore->mErrorList->addCommentText(std::string("Unable to find "+name+" in agent inventory."));
				allcached = FALSE;
			}
		}
	}
	return allcached;
}

void JCLSLPreprocessor::preprocess_script(BOOL close)
{
	mClose = close;
	cached_files.clear();
	caching_files.clear();
	mCore->mErrorList->addCommentText(std::string("Starting..."));
	LLFile::mkdir(gDirUtilp->getExpandedFilename(LL_PATH_CACHE,"")+gDirUtilp->getDirDelimiter()+"lslpreproc");
	std::string script = mCore->mEditor->getText();
	//this script is special
	LLViewerInventoryItem* item = mCore->mItem;
	std::string name = item->getName();
	cached_files.push_back(name);
	cache_script(name, script);
	//start the party
	std::vector<std::string> files = scan_includes(name, script);
	if(cachecheck(files))start_process();
}
/*
std::string implement_switches(std::string input)
{
	std::string swinit = "switch(";
	S32 swp = input.find(swinit);
	if(swp != -1)
	{
		S32 scope = 0;
		std::string tmp = input.substr(swp + swinit.length());
		S32 up = tmp.find("{");
		S32 down = tmp.find("}");
		S32 truedown = 0;
		while(up != -1 && up < down)//inefficient, i know.
		{
			//if(up != -1)
			{
				tmp = tmp.substr(up+1);
				scope += 1;
				truedown += up+1;
			}
			up = tmp.find("{");
		}
		while(down != -1 && scope > 0)//inefficient, i know.
		{
			//if(up != -1)
			{
				tmp = tmp.substr(down+1);
				scope -= 1;
				truedown += down+1;
			}
			down = tmp.find("}");
		}
		if(down != -1)
		{
			tmp = input.substr(swp + swinit.length(),(truedown-1) - (swp + swinit.length()));
			cmdline_printchat(std::string("<SWITCH\n")+tmp+"\nSWITCH>");
		}
		swp = input.find(swinit);
	}
	return input;
}*/

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
	mCore->mErrorList->addCommentText(std::string("Completed caching."));

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
		typedef boost::wave::context<std::string::iterator, lex_iterator_type>
				context_type;

		// The preprocessor iterator shouldn't be constructed directly. It is 
		// to be generated through a wave::context<> object. This wave:context<> 
		// object is to be used additionally to initialize and define different 
		// parameters of the actual preprocessing (not done here).
		//
		// The preprocessing of the input stream is done on the fly behind the 
		// scenes during iteration over the context_type::iterator_type stream.
		context_type ctx (input.begin(), input.end(), name.c_str());

		ctx.set_language(boost::wave::enable_long_long(ctx.get_language()));
		ctx.set_language(boost::wave::enable_preserve_comments(ctx.get_language()));
		ctx.set_language(boost::wave::enable_prefer_pp_numbers(ctx.get_language()));

		std::string path = gDirUtilp->getExpandedFilename(LL_PATH_CACHE,"")+gDirUtilp->getDirDelimiter()+"lslpreproc"+gDirUtilp->getDirDelimiter();
		ctx.add_include_path(path.c_str());

		std::string def = llformat("__AGENTKEY__=%s",gAgent.getID().asString().c_str());
		ctx.add_macro_definition(def,false);
		std::string aname;
		gAgent.getName(aname);
		def = llformat("__AGENTNAME__=%s",aname.c_str());
		ctx.add_macro_definition(def,false);
		def = llformat("__ITEMID__=%s",mCore->mItem->getUUID().asString().c_str());
		ctx.add_macro_definition(def,false);
		//tba
		def = llformat("__BASE_ITEMID__=%s",mCore->mItem->getUUID().asString().c_str());
		ctx.add_macro_definition(def,false);

		//  Get the preprocessor iterators and use them to generate 
		//  the token sequence.
		context_type::iterator_type first = ctx.begin();
		context_type::iterator_type last = ctx.end();
	        

		//  The input stream is preprocessed for you while iterating over the range
		//  [first, last)
        while (first != last) {
            current_position = (*first).get_position();
			std::string token = std::string((*first).get_value().c_str());
			if(token == "#line")token = "//#line";
			output += token;//stupid boost bitching even though we know its a std::string
            ++first;
        }
		lazy_lists = ctx.is_defined_macro(std::string("USE_LAZY_LISTS"));
	}
	catch(boost::wave::cpp_exception const& e)
	{
		errored = TRUE;
		// some preprocessing error
		err = name + "(" + llformat("%d",e.line_no()) + "): " + e.description();
		mCore->mErrorList->addCommentText(err);
	}
	catch(std::exception const& e)
	{
		errored = TRUE;
		err = std::string(current_position.get_file().c_str()) + "(" + llformat("%d",current_position.get_line()) + "): ";
		err += std::string("exception caught: ") + e.what();
		mCore->mErrorList->addCommentText(err);
	}
	catch (...)
	{
		errored = TRUE;
		err = std::string(current_position.get_file().c_str()) + llformat("%d",current_position.get_line());
		err += std::string("): unexpected exception caught.");
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
}



/*else if(command == "wavetest")
			{
				boost::wave::util::file_position_type current_position;
				try 
				{

					typedef boost::wave::cpplexer::lex_iterator<
							boost::wave::cpplexer::lex_token<> >
						lex_iterator_type;
					typedef boost::wave::context<
							std::string::iterator, lex_iterator_type>
						context_type;

					context_type ctx(command.begin(), command.end(), "input.cpp");

					context_type::iterator_type first = ctx.begin();
					context_type::iterator_type last = ctx.end();



					while (first != last)
					{
						current_position = (*first).get_position();
						std::string tmp = (*first).get_value().c_str();
						cmdline_printchat(tmp);
						//std::cout << (*first).get_value();
						++first;
					}
				}catch(boost::wave::cpp_exception const& e)
				{
				// some preprocessing error
					std::string err;
					err += e.file_name();
					err += "(";
					err += llformat("%d",e.line_no());
					err += "): ";
					err += e.description();
					cmdline_printchat(err);
					//return 2;
				}
				catch(std::exception const& e)
				{
					std::string err;
				// use last recognized token to retrieve the error position
					err += current_position.get_file().c_str();
					err += "(";
					err += llformat("%d",current_position.get_line());
					err += "): ";
					err += "exception caught: ";
					err += e.what();
					cmdline_printchat(err);
					//return 3;
				}
				catch (...)
				{
					std::string err;
				// use last recognized token to retrieve the error position
					err += current_position.get_file().c_str();
					err += llformat("%d",current_position.get_line());
					err += "): ";
					err += "unexpected exception caught.";
					cmdline_printchat(err);
					//return 4;
				}
			}
		}*/