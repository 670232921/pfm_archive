#include "ArchiveVolume.h"
#include <iostream>
#include <fstream>
using namespace std;

#define WAIT
#define EXIT throw
#define CHECKNULL(x) if (x == nullptr) { cout << "nullptr error :" << __LINE__; WAIT; EXIT;}
#define CHECKHRESULT(x) if (x != S_OK) { cout << "hresult error :" << __LINE__; WAIT; EXIT;}
#define CHECKBOOL(x) if (!x) { cout << "bool error :" << __LINE__; WAIT; EXIT;}

DEFINE_GUID(CLSID_CFormat7z,
	0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x07, 0x00, 0x00);
#define CLSID_Format CLSID_CFormat7z

static PfmOpenAttribs zeroOpenAttribs = {};
static PfmAttribs zeroAttribs = {};
static PfmMediaInfo zeroMediaInfo = {};

PfmFormatterDispatch* GetPfmFormatterDispatch(wchar_t * name)
{
	return new ArchiveVolume(name);
}


class InStream : public IInStream, public CMyUnknownImp
{
	ifstream _ifs;
	FILE *_f = nullptr;
	size_t len = 0;
public:
	InStream(LPCWSTR filename)
	{
		_ifs.open(filename, ios::binary);
		if (!_ifs.is_open())
		{
			throw L"open failed";
		}
	}

	STDMETHODIMP Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition)
	{
		_ifs.seekg(offset, seekOrigin == SZ_SEEK_SET ? _ifs.beg : (seekOrigin == SZ_SEEK_CUR ? _ifs.cur : _ifs.end));
		if (newPosition) *newPosition = _ifs.tellg();
		return S_OK;
	}

	STDMETHODIMP Read(void *data, UInt32 size, UInt32 *processedSize)
	{
		_ifs.read((char*)data, size);
		if (processedSize) *processedSize = _ifs.gcount();
		return S_OK;
	}

	MY_QUERYINTERFACE_BEGIN2(IInStream)
	MY_QUERYINTERFACE_END
	MY_ADDREF_RELEASE
};

class CArchiveOpenCallback :
	public IArchiveOpenCallback,
	public ICryptoGetTextPassword,
	public CMyUnknownImp
{
public:
	MY_UNKNOWN_IMP1(ICryptoGetTextPassword)

	STDMETHOD(SetTotal)(const UInt64 *files, const UInt64 *bytes);
	STDMETHOD(SetCompleted)(const UInt64 *files, const UInt64 *bytes);

	STDMETHOD(CryptoGetTextPassword)(BSTR *password);

	CArchiveOpenCallback() {}
};

STDMETHODIMP CArchiveOpenCallback::SetTotal(const UInt64 * /* files */, const UInt64 * /* bytes */)
{
	return S_OK;
}

STDMETHODIMP CArchiveOpenCallback::SetCompleted(const UInt64 * /* files */, const UInt64 * /* bytes */)
{
	return S_OK;
}

STDMETHODIMP CArchiveOpenCallback::CryptoGetTextPassword(BSTR *password)
{
	return E_ABORT;
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
	int64_t parentFileId = 0; //todo
	const wchar_t* endName = 0; //todo

	if (!namePartCount)
	{
		existed = true;
		openAttribs.openId = newExistingOpenId;
		openAttribs.openSequence = 1;
		openAttribs.accessLevel = pfmAccessLevelReadData;
		openAttribs.attribs.fileType = pfmFileTypeFolder;
		openAttribs.attribs.fileId = RootArchiveID;
		SaveID(newExistingOpenId, RootArchiveID);
	}
	else if (namePartCount == 1)
	{
		/*if (sswcmpf(nameParts[0].name, helloFileName) != 0)
		{
			perr = pfmErrorNotFound;
		}
		else
		{
			if (!fileOpenId)
			{
				fileOpenId = newExistingOpenId;
			}
			existed = true;
			openAttribs.openId = fileOpenId;
			openAttribs.openSequence = 1;
			openAttribs.accessLevel = pfmAccessLevelReadData;
			openAttribs.attribs.fileType = pfmFileTypeFile;
			openAttribs.attribs.fileId = helloFileId;
			openAttribs.attribs.fileSize = helloDataSize;
			endName = helloFileName;
		}*/
	}
	else
	{
		perr = pfmErrorParentNotFound;
	}
	if (perr == pfmErrorNotFound && createFileType != 0)
	{
		perr = pfmErrorAccessDenied;
	}

	op->Complete(perr, existed, &openAttribs, parentFileId, endName, 0, 0, 0, 0);
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
	op->Complete(pfmErrorSuccess,
		GetFileAttribute(GetArchiveID(op->OpenId())).fileSize/*totalCapacity*/, 0/*availableCapacity*/);
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

	op->Complete(pfmErrorSuccess, &mediaInfo, L"hellofs"/*mediaLabel*/);
	// todo filename
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

	_dll = ::LoadLibrary(DllName);
	CHECKNULL(_dll);

	Func_CreateObject pco = reinterpret_cast<Func_CreateObject>(::GetProcAddress(_dll, "CreateObject"));
	CHECKNULL(pco);

	pco(&CLSID_Format, &IID_IInArchive, reinterpret_cast<void **>(&_archive));
	CHECKNULL(_archive);

	const UInt64 scanSize = 1 << 23;
	_inStream = new InStream(filePath);
	CMyComPtr<IArchiveOpenCallback> callback = new CArchiveOpenCallback();
	_ret = _archive->Open(_inStream, &scanSize, callback);
	CHECKHRESULT(_ret);

	_ret = _archive->GetNumberOfItems(&_fileCount);
	CHECKHRESULT(_ret);
}

ArchiveVolume::~ArchiveVolume(void)
{
	if (_dll != nullptr)
		::FreeLibrary(_dll);
}

PfmAttribs ArchiveVolume::GetFileAttribute(UInt32 id)
{
	/*static int8_t const pfmFileTypeFile = 1;
	static int8_t const pfmFileTypeFolder = 2;*/
	PfmAttribs att = zeroAttribs;
	PROPVARIANT v;

	_ret = _archive->GetProperty(id, kpidIsDir, &v);
	CHECKHRESULT(_ret);
	if (v.vt == VT_BOOL)
		att.fileType = v.boolVal ? pfmFileTypeFolder : pfmFileTypeFile;
	CleanVAR(&v);

	att.fileId = id;

	_ret = _archive->GetProperty(id, kpidSize, &v);
	CHECKHRESULT(_ret);
	if (v.vt == VT_UI8)
		att.fileSize = v.lVal;
	CleanVAR(&v);
	return att;
}

UInt32 ArchiveVolume::GetArchiveID(PfmNamePart * parts, size_t count)
{
	wstring s = parts[0].name;
	for (size_t i = 1; i < count; i++)
	{
		s += L"\\";
		s += parts[i].name;
	}

	for (UInt32 i = 0; i < _fileCount; i++)
	{
		if (GetPathPro(i) == s)
			return i;
	}
	throw;
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

size_t ArchiveVolume::List(UInt32 id, PfmMarshallerListOp* op)
{
	return size_t();
}

wstring &ArchiveVolume::GetPathPro(UInt32 id)
{
	PROPVARIANT v;
	wstring ret;
	_ret = _archive->GetProperty(id, kpidPath, &v);
	CHECKHRESULT(_ret);
	if (v.vt == VT_BSTR)
		ret = v.bstrVal;
	CleanVAR(&v);
	return ret;
}

void ArchiveVolume::CleanVAR(PROPVARIANT *prop)
{
	switch (prop->vt)
	{
	case VT_EMPTY:
	case VT_UI1:
	case VT_I1:
	case VT_I2:
	case VT_UI2:
	case VT_BOOL:
	case VT_I4:
	case VT_UI4:
	case VT_R4:
	case VT_INT:
	case VT_UINT:
	case VT_ERROR:
	case VT_FILETIME:
	case VT_UI8:
	case VT_R8:
	case VT_CY:
	case VT_DATE:
		prop->vt = VT_EMPTY;
		prop->wReserved1 = 0;
		prop->wReserved2 = 0;
		prop->wReserved3 = 0;
		prop->uhVal.QuadPart = 0;
		return;
	}
	::VariantClear((VARIANTARG *)prop);
	// return ::PropVariantClear(prop);
	// PropVariantClear can clear VT_BLOB.
}
