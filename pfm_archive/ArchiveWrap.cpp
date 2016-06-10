#include "ArchiveWrap.h"
#include <fstream>
#include <iostream>
#include <algorithm>
using namespace std;

#define WAIT
#define EXIT return false;
#define CHECKNULL(x) if (x == nullptr) { cout << "nullptr error :" << __LINE__; WAIT; EXIT;}
#define CHECKHRESULT(x) if (x != S_OK) { cout << "hresult error :" << __LINE__; WAIT; EXIT;}
#define CHECKBOOL(x) if (!x) { cout << "bool error :" << __LINE__; WAIT; EXIT;}

DEFINE_GUID(CLSID_CFormat7z,
	0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x07, 0x00, 0x00);
#define CLSID_Format CLSID_CFormat7z

#pragma region IMP
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
///////////////////////////////////////////////////////////////////////
class CArchiveOpenCallback :
	public IArchiveOpenCallback,
	public ICryptoGetTextPassword,
	public CMyUnknownImp
{
public:
	MY_UNKNOWN_IMP1(ICryptoGetTextPassword)

	CArchiveOpenCallback() {}


	STDMETHODIMP SetTotal(const UInt64 * /* files */, const UInt64 * /* bytes */)
	{
		return S_OK;
	}

	STDMETHODIMP SetCompleted(const UInt64 * /* files */, const UInt64 * /* bytes */)
	{
		return S_OK;
	}

	STDMETHODIMP CryptoGetTextPassword(BSTR *password)
	{
		return E_ABORT;
	}
};
/////////////////////////////////////////////////////////////////////
class CArchiveExtractCallback :
	public IArchiveExtractCallback,
	public ICryptoGetTextPassword,
	public CMyUnknownImp
{
public:
	MY_UNKNOWN_IMP1(ICryptoGetTextPassword)

private:
	CMyComPtr<MyCacheStream> _stream;
	bool _extractMode;

public:

	UInt64 NumErrors;

	CArchiveExtractCallback(MyCacheStream * stream) : _stream(stream), NumErrors(0) {	}

	STDMETHODIMP CArchiveExtractCallback::SetTotal(UInt64 /* size */)
	{
		return S_OK;
	}

	STDMETHODIMP CArchiveExtractCallback::SetCompleted(const UInt64 * /* completeValue */)
	{
		return S_OK;
	}

	STDMETHODIMP CArchiveExtractCallback::GetStream(UInt32 index,
		ISequentialOutStream **outStream, Int32 askExtractMode)
	{
		return askExtractMode == NArchive::NExtract::NAskMode::kExtract ? _stream.QueryInterface(IID_ISequentialOutStream, outStream) : S_OK;
	}

	STDMETHODIMP CArchiveExtractCallback::PrepareOperation(Int32 askExtractMode)
	{
		return S_OK;
	}

	STDMETHODIMP CArchiveExtractCallback::SetOperationResult(Int32 operationResult)
	{
		switch (operationResult)
		{
		case NArchive::NExtract::NOperationResult::kOK:
			break;
		default:
		{
			NumErrors++;
			/*switch (operationResult)
			{
			case NArchive::NExtract::NOperationResult::kUnsupportedMethod:
				break;
			case NArchive::NExtract::NOperationResult::kCRCError:
				break;
			case NArchive::NExtract::NOperationResult::kDataError:
				break;
			case NArchive::NExtract::NOperationResult::kUnavailable:
				break;
			case NArchive::NExtract::NOperationResult::kUnexpectedEnd:
				break;
			case NArchive::NExtract::NOperationResult::kDataAfterEnd:
				break;
			case NArchive::NExtract::NOperationResult::kIsNotArc:
				break;
			case NArchive::NExtract::NOperationResult::kHeadersError:
				break;
			}*/
		}
		}

		_stream->CloseFile();
		return S_OK;
	}


	STDMETHODIMP CArchiveExtractCallback::CryptoGetTextPassword(BSTR *password)
	{
		return E_ABORT; // todo
	}
};
#pragma endregion IMP

wstring const ArchiveWrap::RootName(L"Root");

ULONGLONG ArchiveWrap::GetHash(LPCWSTR s, UInt32 id)
{
#define GetHashA 54059 /* a prime */
#define GetHashB 76963 /* another prime */
	ULONGLONG h = id;
	while (*s) {
		h = (h * GetHashA) ^ (s[0] * GetHashB);
		s++;
	}
	return h;
}

ArchiveWrap::~ArchiveWrap()
{
	if (_dll != nullptr)
		::FreeLibrary(_dll);
}

bool ArchiveWrap::Init(LPCWSTR filePath)
{
	_filePath = filePath;

	_dll = ::LoadLibrary(DllName);
	CHECKNULL(_dll);

	Func_CreateObject pco = reinterpret_cast<Func_CreateObject>(::GetProcAddress(_dll, "CreateObject"));
	CHECKNULL(pco);

	pco(&CLSID_Format, &IID_IInArchive, reinterpret_cast<void **>(&_archive));
	CHECKNULL(_archive);

	const UInt64 scanSize = 1 << 23;
	_inStream = new InStream(filePath);
	CMyComPtr<IArchiveOpenCallback> callback = new CArchiveOpenCallback();
	HRESULT ret = _archive->Open(_inStream, &scanSize, callback);
	CHECKHRESULT(ret);

	ret = _archive->GetNumberOfItems(&_fileCount);
	CHECKHRESULT(ret);

	CreateFilesAndFolders();
	return true;
}

const wstring & ArchiveWrap::GetPathProp(UInt32 index)
{
	if (index < _fileCount)
	{
		return _files[index];
	}
	else if (index < _folders.size() + _fileCount)
	{
		return _folders[index - _fileCount];
	}
	else
	{
		return RootName;
	}
}

size_t ArchiveWrap::Read(UInt32 id, byte * data, size_t offset, size_t size)
{
	ULONGLONG hash = GetHash(_filePath.c_str(), id);
	NWindows::NCOM::CPropVariant v;
	HRESULT hr = GetProperty(id, kpidSize, &v);
	if (!SUCCEEDED(hr) || v.vt != VT_UI8) return 0;
	if (!_streamManager.IsAdded(hash))
	{
		auto fun = [id, this](MyCacheStream *s) {
			CMyComPtr<IArchiveExtractCallback> callback = new CArchiveExtractCallback(s);
			return _archive->Extract(&id, 1, FALSE, callback);
		};
		_streamManager.AddStream(hash, v.lVal, fun);
	}
	return _streamManager.Read(hash, data, offset, size);
}

void ArchiveWrap::CreateFilesAndFolders()
{
	for (UInt32 i = 0; i < _fileCount; i++)
	{
		_files.push_back(GetPathFromArchive(i));
	}

	vector<wstring> all(_files);
	sort(all.begin(), all.end());

	for (UInt32 i = 0; i < _fileCount; i++)
	{
		wstring temp(all[i]);
		size_t pos = 0;
		while ((pos = temp.find_last_of(L'\\')) != temp.npos)
		{
			temp = temp.substr(0, pos);
			if (binary_search(all.cbegin(), all.cend(), temp) ||
				find(_folders.cbegin(), _folders.cend(), temp) != _folders.cend())
			{
				break;
			}

			_folders.push_back(temp);
		}
	}
}

STDMETHODIMP ArchiveWrap::GetProperty(UInt32 index, PROPID propID, PROPVARIANT *value)
{
	if (index < _fileCount)
	{
		return _archive->GetProperty(index, propID, value);
	}
	else if (index < _fileCount + _folders.size())
	{
		NWindows::NCOM::PropVariant_Clear(value);
		if (propID == kpidPath)
		{
			value->vt = VT_BSTR;
			value->bstrVal = ::SysAllocString(_folders[index - _fileCount].c_str());
		}
		else if (propID == kpidIsDir)
		{
			value->vt = VT_BOOL;
			value->boolVal = -1/*true*/;
		}
		else if (propID == kpidSize)
		{
			value->vt = VT_UI8;
			value->lVal = 0;
		}
		else
		{
			throw;
		}
	}
	else
	{
		return E_FAIL;
	}
	return S_OK;
}

wstring ArchiveWrap::GetPathFromArchive(UInt32 id)
{
	NWindows::NCOM::CPropVariant v;
	wstring ret;
	HRESULT _ret = _archive->GetProperty(id, kpidPath, &v);
	CHECKHRESULT(_ret);
	if (v.vt == VT_BSTR)
		ret = v.bstrVal;
	return ret;
}