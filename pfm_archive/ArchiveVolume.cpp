#include "ArchiveVolume.h"
#include <iostream>
#include <fstream>
using namespace std;

#define WAIT
#define EXIT throw
#define CHECKNULL(x) if (x == nullptr) { cout << "nullptr error :" << __LINE__; WAIT; EXIT;}
#define CHECKHRESULT(x) if (x != S_OK) { cout << "hresult error :" << __LINE__; WAIT; EXIT;}
#define CHECKBOOL(x) if (!x) { cout << "bool error :" << __LINE__; WAIT; EXIT;}

static PfmOpenAttribs zeroOpenAttribs = {};
static PfmAttribs zeroAttribs = {};
static PfmMediaInfo zeroMediaInfo = {};

PfmFormatterDispatch* GetPfmFormatterDispatch(wchar_t * name)
{
	return new ArchiveVolume(name);
}

void CCALL ArchiveVolume::Open(PfmMarshallerOpenOp* op, void* formatterUse)
{
	const PfmNamePart* nameParts = op->NameParts();
	size_t namePartCount = op->NamePartCount();
	int8_t createFileType = op->CreateFileType();
	int64_t newExistingOpenId = op->NewExistingOpenId();
	int perr = 0;
	bool existed = false;
	PfmOpenAttribs openAttribs = zeroOpenAttribs;
	int64_t parentFileId = 0;
	wstring endName;

	if (!namePartCount)
	{
		existed = true;

		if (IsOpenedId(_archive.RootArchiveID))
		{
			openAttribs.openId = GetOpenID(_archive.RootArchiveID);
		}
		else
		{
			SaveID(newExistingOpenId, _archive.RootArchiveID);
			openAttribs.openId = newExistingOpenId;
		}

		openAttribs.openSequence = 1;
		openAttribs.accessLevel = pfmAccessLevelReadData;
		openAttribs.attribs.fileType = pfmFileTypeFolder;
		openAttribs.attribs.fileId = _archive.RootArchiveID;

		endName = L"Root"; // todo
	}
	else
	{
		UInt32 id = GetArchiveID(nameParts, namePartCount);
		if (id == _archive.RootArchiveID - 1)
		{
			perr = pfmErrorNotFound;
		}
		else if (namePartCount == 1)
		{
			parentFileId = _archive.RootArchiveID;
		}
		else
		{
			parentFileId = GetArchiveID(nameParts, namePartCount - 1);
		}

		if (id == _archive.RootArchiveID - 1 || parentFileId == _archive.RootArchiveID - 1)
		{
			perr = pfmErrorNotFound;
		}
		else
		{
			existed = true;

			if (IsOpenedId(id))
			{
				openAttribs.openId = GetOpenID(id);
			}
			else
			{
				openAttribs.openId = newExistingOpenId;
				SaveID(newExistingOpenId, id);
			}

			openAttribs.openSequence = 1;
			openAttribs.accessLevel = pfmAccessLevelReadData;
			openAttribs.attribs = GetFileAttribute(id);
			endName = GetEndName(id);
		}
	}

	if (perr == pfmErrorNotFound && createFileType != 0)
	{
		perr = pfmErrorAccessDenied;
	}

	op->Complete(perr, existed, &openAttribs, parentFileId, endName.c_str(), 0, 0, 0, 0);
}

void CCALL ArchiveVolume::Replace(PfmMarshallerReplaceOp* op, void* formatterUse)
{
	op->Complete(pfmErrorAccessDenied, 0/*openAttribs*/, 0);
}

void CCALL ArchiveVolume::Move(PfmMarshallerMoveOp* op, void* formatterUse)
{
	op->Complete(pfmErrorAccessDenied, 0/*existed*/, 0/*openAttribs*/, 0/*parentFileId*/, 0/*endName*/, 0, 0, 0, 0);
}

void CCALL ArchiveVolume::MoveReplace(PfmMarshallerMoveReplaceOp* op, void* formatterUse)
{
	op->Complete(pfmErrorAccessDenied);
}

void CCALL ArchiveVolume::Delete(PfmMarshallerDeleteOp* op, void* formatterUse)
{
	op->Complete(pfmErrorAccessDenied);
}

void CCALL ArchiveVolume::Close(PfmMarshallerCloseOp* op, void* formatterUse)
{
	op->Complete(pfmErrorSuccess);
}

void CCALL ArchiveVolume::FlushFile(PfmMarshallerFlushFileOp* op, void* formatterUse)
{
	int64_t openId = op->OpenId();
	uint8_t fileFlags = op->FileFlags();
	uint8_t color = op->FileFlags();
	int64_t createTime = op->CreateTime();
	int64_t writeTime = op->WriteTime();
	int perr = 0;
	PfmOpenAttribs openAttribs = zeroOpenAttribs;

	if (fileFlags != pfmFileFlagsInvalid ||
		color != pfmColorInvalid ||
		createTime != pfmTimeInvalid ||
		writeTime != pfmTimeInvalid)
	{
		perr = pfmErrorAccessDenied;
	}
	else if (!IsOpened(openId))
	{
		perr = pfmErrorNotFound;
		// todo log
	}
	else
	{
		openAttribs.openId = openId;
		openAttribs.openSequence = 1;
		openAttribs.accessLevel = pfmAccessLevelReadData;
		openAttribs.attribs = GetFileAttribute(GetArchiveID(openId));
	}

	op->Complete(perr, &openAttribs, 0);
}

void CCALL ArchiveVolume::List(PfmMarshallerListOp* op, void* formatterUse)
{
	int64_t openId = op->OpenId();
	int perr = 0;
	PfmAttribs attribs = zeroAttribs;

	if (!IsOpened(openId))
	{
		perr = pfmErrorAccessDenied;
	}
	else
	{
		List(GetArchiveID(openId), op);
		/*attribs.fileType = pfmFileTypeFile;
		attribs.fileId = helloFileId;
		attribs.fileSize = helloDataSize;
		op->Add(&attribs, helloFileName);*/
	}

	op->Complete(perr, true/*noMore*/);
}

void CCALL ArchiveVolume::ListEnd(PfmMarshallerListEndOp* op, void* formatterUse)
{
	op->Complete(pfmErrorSuccess);
}

void CCALL ArchiveVolume::Read(PfmMarshallerReadOp* op, void* formatterUse)
{
	int64_t openId = op->OpenId();
	uint64_t fileOffset = op->FileOffset();
	void* data = op->Data();
	size_t requestedSize = op->RequestedSize();
	int perr = 0;
	size_t actualSize = 0;

	if (!IsOpened(openId))
	{
		perr = pfmErrorAccessDenied;
	}
	else
	{
		// todo read
	}

	op->Complete(perr, actualSize);
}

void CCALL ArchiveVolume::Write(PfmMarshallerWriteOp* op, void* formatterUse)
{
	op->Complete(pfmErrorAccessDenied, 0/*actualSize*/);
}

void CCALL ArchiveVolume::SetSize(PfmMarshallerSetSizeOp* op, void* formatterUse)
{
	op->Complete(pfmErrorAccessDenied);
}

void CCALL ArchiveVolume::Capacity(PfmMarshallerCapacityOp* op, void* formatterUse)
{
	auto oid = op->OpenId();
	if (oid == 0)
	{
		op->Complete(pfmErrorSuccess, _size/*totalCapacity*/, 0/*availableCapacity*/); // todo
	}
	else if (!IsOpened(op->OpenId()))
	{
		op->Complete(pfmErrorNotFound, 0, 0);
	}
	else
	{
		auto id = GetArchiveID(oid);
		if (id == _archive.RootArchiveID)
		{
			op->Complete(pfmErrorSuccess, _size/*totalCapacity*/, 0/*availableCapacity*/); // todo
		}
		else
		{
			op->Complete(pfmErrorSuccess, GetFileAttribute(id).fileSize/*totalCapacity*/, 0/*availableCapacity*/);
		}
	}
}

void CCALL ArchiveVolume::FlushMedia(PfmMarshallerFlushMediaOp* op, void* formatterUse)
{
	op->Complete(pfmErrorSuccess, -1/*msecFlushDelay*/);
}

void CCALL ArchiveVolume::Control(PfmMarshallerControlOp* op, void* formatterUse)
{
	op->Complete(pfmErrorInvalid, 0/*outputSize*/);
}

void CCALL ArchiveVolume::MediaInfo(PfmMarshallerMediaInfoOp* op, void* formatterUse)
{
	PfmMediaInfo mediaInfo = zeroMediaInfo;

	op->Complete(pfmErrorSuccess, &mediaInfo, GetEndName(_fileName).c_str()/*mediaLabel*/);
}

void CCALL ArchiveVolume::Access(PfmMarshallerAccessOp* op, void* formatterUse)
{
	int64_t openId = op->OpenId();
	int perr = 0;
	PfmOpenAttribs openAttribs = zeroOpenAttribs;

	if (!IsOpened(openId))
	{
		perr = pfmErrorNotFound;
	}
	else
	{
		openAttribs.openId = openId;
		openAttribs.openSequence = 1;
		openAttribs.accessLevel = pfmAccessLevelReadData;
		openAttribs.attribs = GetFileAttribute(GetArchiveID(openId));
		perr = 0;
	}

	op->Complete(perr, &openAttribs, 0);
}

void CCALL ArchiveVolume::ReadXattr(PfmMarshallerReadXattrOp* op, void* formatterUse)
{
	op->Complete(pfmErrorNotFound, 0/*xattrSize*/, 0/*transferredSize*/);
}

void CCALL ArchiveVolume::WriteXattr(PfmMarshallerWriteXattrOp* op, void* formatterUse)
{
	op->Complete(pfmErrorAccessDenied, 0/*transferredSize*/);
}

ArchiveVolume::ArchiveVolume(LPCWSTR filePath)
{
	_fileName = filePath;

	{
		wifstream in(filePath, std::ifstream::ate | std::ifstream::binary);
		if (in.is_open())
		{
			_size = in.tellg();
			in.close();
		}
	}

	_aviliable = _archive.Init(filePath);
}

PfmAttribs ArchiveVolume::GetFileAttribute(UInt32 id)
{
	/*static int8_t const pfmFileTypeFile = 1;
	static int8_t const pfmFileTypeFolder = 2;*/
	PfmAttribs att = zeroAttribs;
	NWindows::NCOM::CPropVariant v;

	_ret = _archive.GetProperty(id, kpidIsDir, &v);
	CHECKHRESULT(_ret);
	if (v.vt == VT_BOOL)
		att.fileType = v.boolVal ? pfmFileTypeFolder : pfmFileTypeFile;

	att.fileId = id;

	v.Clear();
	_ret = _archive.GetProperty(id, kpidSize, &v);
	CHECKHRESULT(_ret);
	if (v.vt == VT_UI8)
		att.fileSize = v.lVal;
	return att;
}

UInt32 ArchiveVolume::GetArchiveID(const PfmNamePart * parts, size_t count)
{
	wstring s = parts[0].name;
	for (size_t i = 1; i < count; i++)
	{
		s += L"\\";
		s += parts[i].name;
	}

	for (UInt32 i = 0; i < _archive.GetCount(); i++)
	{
		if (_archive.GetPathProp(i) == s)
			return i;
	}
	return _archive.RootArchiveID - 1; // not found
}

int64_t ArchiveVolume::GetOpenID(UInt32 id)
{
	for each(auto item in _openIDArchiveIDMap)
	{
		if (item.second == id)
			return item.first;
	}
	throw;
}

bool ArchiveVolume::IsOpenedId(UInt32 id)
{
	for each(auto item in _openIDArchiveIDMap)
	{
		if (item.second == id)
			return true;
	}
	return false;
}

size_t ArchiveVolume::List(UInt32 id, PfmMarshallerListOp* op)
{
	size_t ret = 0;
	if (id == _archive.RootArchiveID)
	{
		for (UInt32 i = 0; i < _archive.GetCount(); i++)
		{
			wstring name = _archive.GetPathProp(i);
			if (name.find(L'\\') == name.npos)
			{
				PfmAttribs att = GetFileAttribute(i);
				op->Add(&att, name.c_str());
				ret++;
			}
		}
		return ret;
	}

	wstring parentPath = _archive.GetPathProp(id) + L'\\';
	for (UInt32 i = 0; i < _archive.GetCount(); i++)
	{
		wstring name = _archive.GetPathProp(i);
		if (name.compare(0, parentPath.length(), parentPath) == 0)
		{
			wstring endname = name.substr(parentPath.length());
			if (endname.find(L'\\') == endname.npos)
			{
				PfmAttribs att = GetFileAttribute(i);
				op->Add(&att, endname.c_str());
				ret++;
			}
		}
	}

	return ret;
}

std::wstring ArchiveVolume::GetEndName(UInt32 id)
{
	wstring path = _archive.GetPathProp(id);
	return GetEndName(path);
}

std::wstring ArchiveVolume::GetEndName(wstring path)
{

	size_t pos = path.rfind(L'\\');
	if (pos == path.npos)
	{
		return path;
	}
	else
	{
		return path.substr(pos + 1);
	}
}
