#pragma once

#include "7zheaders.h"

#include <vector>

class ArchiveWrap
{
public:
	static const UInt32 RootArchiveID = min(~(UInt32)0, ~(int64_t)0);
	static const std::wstring RootName;

	ArchiveWrap() {}
	~ArchiveWrap();

	bool Init(LPCWSTR filePath);

	UInt32 GetCount() { return _fileCount + _folders.size(); }
	STDMETHOD(GetProperty)(UInt32 index, PROPID propID, PROPVARIANT *value) MY_NO_THROW_DECL_ONLY;
	const std::wstring &GetPathProp(UInt32 index);
private:
	LPCWSTR DllName = L"7z.dll";
	UInt32 _fileCount = 0;
	CMyComPtr<IInArchive> _archive;
	CMyComPtr<IInStream> _inStream;
	HMODULE _dll = nullptr;

	std::vector<std::wstring> _folders;
	std::vector<std::wstring> _files;

	void CreateFilesAndFolders();
	std::wstring GetPathFromArchive(UInt32 id);
};
