#pragma once

#include "7zheaders.h"
#include "StreamManager.h"
#include <vector>

class ArchiveWrap
{
public:
	static const UInt32 RootArchiveID = min(~(UInt32)0, ~(int64_t)0);
	static const std::wstring RootName;
	static ULONGLONG GetHash(LPCWSTR s, UInt32 id);

	ArchiveWrap() {}
	~ArchiveWrap();

	bool Init(LPCWSTR filePath);

	UInt32 GetCount() { return _fileCount + _folders.size(); }
	STDMETHOD(GetProperty)(UInt32 index, PROPID propID, PROPVARIANT *value) MY_NO_THROW_DECL_ONLY;
	const std::wstring &GetPathProp(UInt32 index);

	size_t Read(UInt32 id, byte * data, size_t offset, size_t size);
private:
	LPCWSTR DllName = L"7z.dll";
	std::wstring _filePath;
	UInt32 _fileCount = 0;
	CMyComPtr<IInArchive> _archive;
	CMyComPtr<IInStream> _inStream;
	HMODULE _dll = nullptr;

	StreamManager _streamManager;

	std::vector<std::wstring> _folders;
	std::vector<std::wstring> _files;

	void CreateFilesAndFolders();
	std::wstring GetPathFromArchive(UInt32 id);
};
