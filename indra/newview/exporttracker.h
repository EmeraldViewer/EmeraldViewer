
#include "llagent.h"


class JCExportTracker
{
public:
	JCExportTracker();
	~JCExportTracker();

private:
	static JCExportTracker* sInstance;
	static void init();
public:
	static JCExportTracker* getInstance(){ init(); return sInstance; }

	static bool serialize(LLDynamicArray<LLViewerObject*> objects);
	static bool serializeSelection();

private:
	static LLSD subserialize(LLViewerObject* linkset);
};