#pragma once
#include "7zheaders.h"
#include <map>
#include <functional>
#include <fstream>

class MyCacheStream :
	public ISequentialOutStream,
	public CMyUnknownImp
{
public:
	MY_QUERYINTERFACE_BEGIN2(ISequentialOutStream)
	MY_QUERYINTERFACE_END
	MY_ADDREF_RELEASE

	STDMETHOD(Write)(const void *data, UInt32 size, UInt32 *processedSize);

	MyCacheStream(ULONGLONG hash, size_t size);
	~MyCacheStream();

	size_t Read(byte * data, size_t offset, size_t size);

	void CloseFile();
private:
	ULONGLONG _hash = 0;
	size_t _size = 0;
	time_t _lastAccess = 0;
	bool _localCache = false;
	
	// mem
	byte *_buffer = nullptr;
	size_t _bufferPos = 0;

	// local
	std::wifstream _localStream;
public:
	bool SetLocalCache(LPCWSTR parentfolder);

	inline size_t Size() { return _size; }
	inline time_t LastAccess() { return _lastAccess; }
};

class StreamManager
{
public:
	static const size_t SizeToLocalCache = 300 * 1024 * 1024;

	StreamManager() { }
	~StreamManager() { for each (auto item in _streams) { delete item.second; } }

	inline bool IsAdded(ULONGLONG hash) { return _streams.find(hash) != _streams.end(); }

	HRESULT AddStream(ULONGLONG hash, size_t fileSize, std::function<HRESULT(MyCacheStream*)>);
	inline size_t Read(ULONGLONG hash, byte * data, size_t offset, size_t size) { return IsAdded(hash) ? _streams[hash]->Read(data, offset, size) : 0; }
private:
	std::map<ULONGLONG, MyCacheStream*> _streams;
};