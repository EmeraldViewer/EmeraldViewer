/** 
 * @file importtracker.h
 * @brief A utility for importing linksets from XML.
 * Discrete wuz here
 */

#ifndef IMPORTTRACKER_H
#define IMPORTTRACKER_H

#include "llagent.h"

using namespace std;

class ImportTracker
{
	public:
		enum ImportState { IDLE, REZZING, COPYING, LINKING, POSITIONING };			
		
		ImportTracker() { state = IDLE; }
		ImportTracker(LLSD &data) { state = IDLE; linkset = data; }
		~ImportTracker() { localids.clear(); linkset.clear(); }
	
		void import(LLSD &file_data);
		void clear();
		void get_update(S32 newid, BOOL justCreated = false);
		
		const int getState() { return state; }
		
	protected:		
		void send_vectors(LLSD &prim);
		void send_shape(LLSD &prim);
		void send_image(LLSD &prim);
		void send_extras(LLSD &prim);
		void update_next();
		void link();
		void wear(LLSD &prim);
		void position(LLSD &prim);
		void plywood_above_head();
	
	private:
		int				state;
		S32				last;
		LLVector3			root;
		std::list<S32>			localids;
		LLSD				linkset;
};

extern ImportTracker gImportTracker;

extern LLAgent gAgent;

#endif