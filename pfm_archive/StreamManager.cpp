#include "StreamManager.h"
#include <time.h>
#include <fstream>
using namespace std;


STDMETHODIMP MyCacheStream::Write(const void * data, UInt32 size, UInt32 * processedSize)
{
	if (_localCache)
	{
		// todo
	}
	else
	{
		if (_buffer == nullptr)
			_buffer = new byte[_size];

		*processedSize = size + _bufferPos > _size ? _size - _bufferPos : size;
		memcpy(_buffer + _bufferPos, data, *processedSize);
		_bufferPos += *processedSize;
		return S_OK;
		// todo lock
	}
	return E_NOTIMPL;
}

MyCacheStream::MyCacheStream(ULONGLONG hash, size_t size) : _hash(hash), _size(size)
{
	time(&_lastAccess);
}

MyCacheStream::~MyCacheStream()
{
	if (_localStream.is_open())
	{
		_localStream.close();
	}
}

size_t MyCacheStream::Read(byte * data, size_t offset, size_t size)
{
	size_t ret = 0;
	if (_localCache)
	{
		// todo
	}
	else
	{
		ret = offset + size > _bufferPos ? (_bufferPos > offset ? _bufferPos - offset : 0) : size;
		// todo wait
		memcpy(data, _buffer + offset, ret);
	}
	return ret;
}

void MyCacheStream::CloseFile()
{
	/*if (_localStream.is_open())
	{
		_localStream.close();
	}*/
	// todo close out stream
}

bool MyCacheStream::SetLocalCache(LPCWSTR parentfolder)
{
	_localCache = true;

	wstring path(parentfolder);
	path += L'\\';
	path += _hash;

	_localStream.open(path, std::ifstream::ate | std::ifstream::binary);
	if (_localStream.is_open() && _size == _localStream.tellg())
	{
		return true;
	}

	_localStream.close();

	//wofstream out(path)
	// todo
	return false;
}



HRESULT StreamManager::AddStream(ULONGLONG hash, size_t fileSize, std::function<HRESULT(MyCacheStream*)> extract)
{
	if (IsAdded(hash))
	{
		return S_OK;
	}

	MyCacheStream * stream = new MyCacheStream(hash, fileSize);
	stream->AddRef();
	_streams[hash] = stream;

	if (fileSize > SizeToLocalCache)
	{
		throw; // todo
	}

	return extract(stream);
}