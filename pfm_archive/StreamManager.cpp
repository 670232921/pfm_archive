#include "StreamManager.h"




STDMETHODIMP MyCacheStream::Write(const void * data, UInt32 size, UInt32 * processedSize)
{
	return E_NOTIMPL;
}

MyCacheStream::MyCacheStream()
{
}

MyCacheStream::~MyCacheStream()
{
}

void MyCacheStream::CloseFile()
{
}



HRESULT StreamManager::AddStream(ULONGLONG hash, size_t fileSize, std::function<HRESULT(MyCacheStream*)>)
{
	return E_NOTIMPL;
}

size_t StreamManager::Read(ULONGLONG hash, byte * data, size_t offset, size_t size)
{
	return size_t();
}
