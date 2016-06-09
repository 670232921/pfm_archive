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

wstring const ArchiveWrap::RootName(L"Root");

ArchiveWrap::~ArchiveWrap()
{
	if (_dll != nullptr)
		::FreeLibrary(_dll);
}

bool ArchiveWrap::Init(LPCWSTR filePath)
{
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
		CleanVAR(value);
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

void ArchiveWrap::CleanVAR(PROPVARIANT *prop)
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

wstring ArchiveWrap::GetPathFromArchive(UInt32 id)
{
	PROPVARIANT v;
	wstring ret;
	HRESULT _ret = _archive->GetProperty(id, kpidPath, &v);
	CHECKHRESULT(_ret);
	if (v.vt == VT_BSTR)
		ret = v.bstrVal;
	ArchiveWrap::CleanVAR(&v);
	return ret;
}