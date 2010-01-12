#ifndef EM_KDU_H
#define EM_KDU_H

//#define EXPORT_SYMBOLS
#ifdef EXPORT_SYMBOLS
#if _WIN32
#define EMKDU_EXPORT __declspec(dllexport)
#else
#define EMKDU_EXPORT extern
#endif
#else
#define EMKDU_EXPORT
#endif

typedef unsigned char U8;
typedef signed char S8;

class EMImageDims
{
public:
	EMImageDims()
		: mHeight(0),
		mWidth(0),
		mComponents(0)
	{}
	~EMImageDims() {}
	int mHeight;
	int mWidth;
	int	mComponents;
};

class EMImageData
{
public:
	EMImageData()
		: mHeight(-1),
		mWidth(-1),
		mFirstComp(-1),
		mComponents(-1),
		mData(0),
		mLength(0),
		mDiscard(-1)
	{}
	~EMImageData() {}
	int mHeight;
	int mWidth;
	int mFirstComp;
	int mComponents;
	U8* mData;
	unsigned int mLength;
	S8 mDiscard;
};

class EMKDU
{	
public:
	EMKDU();
	~EMKDU();
	bool encodeData(EMImageData* image_data);
	bool decodeData(EMImageData* image_data);
	bool validateData(EMImageData* image_data, bool for_encode_decode = false);
	EMImageDims getMetadata(EMImageData* image_data);
};

#endif //EM_KDU_H