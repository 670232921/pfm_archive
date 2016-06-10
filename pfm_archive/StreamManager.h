#pragma once
#include "7zheaders.h"
#include <map>
#include <functional>

class MyCacheStream :
	public ISequentialOutStream,
	public CMyUnknownImp
{
public:
	MY_QUERYINTERFACE_BEGIN2(ISequentialOutStream)
	MY_QUERYINTERFACE_END
	MY_ADDREF_RELEASE

	STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);

	MyCacheStream();
	~MyCacheStream();

	void CloseFile();
private:

};

class StreamManager
{
public:
	StreamManager() { }
	~StreamManager() { for each (auto item in _streams) { delete item.second; } }

	inline bool IsAdded(ULONGLONG hash) { return _streams.find(hash) != _streams.end(); }

	HRESULT AddStream(ULONGLONG hash, size_t fileSize, std::function<HRESULT(MyCacheStream*)>);
	size_t Read(ULONGLONG hash, byte * data, size_t offset, size_t size);
private:
	std::map<ULONGLONG, MyCacheStream*> _streams;
};