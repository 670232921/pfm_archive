// 7zTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../../../7z1600-src/CPP/Common/MyWindows.h"

#include "../../../7z1600-src/CPP/Common/Defs.h"
#include "../../../7z1600-src/CPP/Common/MyInitGuid.h"
#include "../../../7z1600-src/CPP/Common/MyCom.h"

#include "../../../7z1600-src//CPP//7zip/IPassword.h"
#include "../../../7z1600-src//CPP//7zip/Common/FileStreams.h"

#include "../../../7z1600-src/CPP/Windows/PropVariantConv.h"

#include <iostream>
#include <fstream>
#include "..\..\..\7z1600-src\CPP\7zip\Archive\IArchive.h"
using namespace std;

#define CHECKNULL(x) if (x == nullptr) { cout << "nullptr error :" << __LINE__; getchar();return-1;}
#define CHECKHRESULT(x) if (x != S_OK) { cout << "hresult error :" << __LINE__; getchar();return-1;}
#define CHECKBOOL(x) if (!x) { cout << "bool error :" << __LINE__; getchar();return-1;}


DEFINE_GUID(CLSID_CFormat7z,
	0x23170F69, 0x40C1, 0x278A, 0x10, 0x00, 0x00, 0x01, 0x10, 0x07, 0x00, 0x00);
#define CLSID_Format CLSID_CFormat7z

class InStream : public IInStream, public CMyUnknownImp
{
	ifstream _ifs;
	FILE *_f = nullptr;
	size_t len = 0;
public:
	InStream(LPCCH filename)
	{
		_ifs.open(filename, ios::binary);
		if (!_ifs.is_open())
		{
			throw L"open failed";
		}
		/*_f = _wfopen(filename, L"r");
			auto a = errno;
		if (!_f)
		{
			throw L"open failed";
		}
		fseek(_f, 0, 2);
		len = ftell(_f);
		fseek(_f, 0, 0);*/
	}

	/*~InStream()
	{
		fclose(_f);
	}*/

	STDMETHODIMP Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition)
	{
		//SZ_SEEK_END
		_ifs.seekg(offset, seekOrigin == SZ_SEEK_SET ? _ifs.beg : (seekOrigin == SZ_SEEK_CUR ? _ifs.cur : _ifs.end));
		if (newPosition) *newPosition = _ifs.tellg();
		return S_OK;

		/*auto ret = fseek(_f, offset, seekOrigin);
		if (newPosition) *newPosition = (ret == 0 ? ftell(_f) : 0);
		return ret;*/
	}

	STDMETHODIMP Read(void *data, UInt32 size, UInt32 *processedSize)
	{
		_ifs.read((char*)data, size);
		if (processedSize) *processedSize = _ifs.gcount();
		return S_OK;
		/*cout << "\n" << "read " << size << " from " << ftell(_f) << " in"<<len;
		auto ret = fread(data, 1, size, _f);
		if (processedSize) *processedSize = ret;
		return S_OK;*/
	}
	MY_QUERYINTERFACE_BEGIN2(IInStream)
	MY_QUERYINTERFACE_END
	MY_ADDREF_RELEASE
};

//class CArchiveOpenCallback :
//	public IArchiveOpenCallback,
//	public ICryptoGetTextPassword,
//	public CMyUnknownImp
//{
//public:
//	MY_UNKNOWN_IMP1(ICryptoGetTextPassword)
//
//	STDMETHODIMP CArchiveOpenCallback::SetTotal(const UInt64 * /* files */, const UInt64 * /* bytes */)
//	{
//		return S_OK;
//	}
//
//	STDMETHODIMP CArchiveOpenCallback::SetCompleted(const UInt64 * /* files */, const UInt64 * /* bytes */)
//	{
//		return S_OK;
//	}
//
//	STDMETHODIMP CArchiveOpenCallback::CryptoGetTextPassword(BSTR *password)
//	{
//		*password = ::SysAllocString(L"123");
//		return S_OK;
//	}
//
//	CArchiveOpenCallback() {}
//};

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

	bool PasswordIsDefined;
	UString Password;

	CArchiveOpenCallback() : PasswordIsDefined(false) {}
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
	if (!PasswordIsDefined)
	{
		// You can ask real password here from user
		// Password = GetPassword(OutStream);
		// PasswordIsDefined = true;
		//PrintError("Password is not defined");
		return E_ABORT;
	}
	return StringToBstr(Password, password);
}

int main()
{
	HMODULE mo = ::LoadLibraryW(L"7z.dll");
	CHECKNULL(mo);

	Func_CreateObject pco = (Func_CreateObject)::GetProcAddress(mo, "CreateObject");
	CHECKNULL(pco);

	CMyComPtr<IInArchive> iina;
	CHECKHRESULT(pco(&CLSID_Format, &IID_IInArchive, (void **)&iina));

	//InStream is(L"C:\\Users\\ccc\\Documents\\Visual Studio 2015\\Projects\\pfm\\pfm_archive\\Debug\\test.7z");
	//CInFileStream * fileStream = new CInFileStream();
	InStream * fileStream = new InStream("C:\\Users\\ccc\\Documents\\Visual Studio 2015\\Projects\\pfm\\pfm_archive\\Debug\\test.7z");
	CMyComPtr<IInStream> is = fileStream;
	//fileStream->Open(L"C:\\Users\\ccc\\Documents\\Visual Studio 2015\\Projects\\pfm\\pfm_archive\\Debug\\test.7z");
    CArchiveOpenCallback* opencallback = new CArchiveOpenCallback();
	CMyComPtr<IArchiveOpenCallback> op = opencallback;
	const UInt64 scanSize = 1 << 23;
	CHECKHRESULT(iina->Open(is, &scanSize, op));

	UInt32 npro = 0, nitem = 0;
	CHECKHRESULT(iina->GetNumberOfProperties(&npro));
	CHECKHRESULT(iina->GetNumberOfItems(&nitem));
	for (int i = 0; i < npro; i++)
	{
		BSTR bstr;
		PROPID pid;
		VARTYPE vt;
		CHECKHRESULT(iina->GetPropertyInfo(i, &bstr, &pid, &vt));
		cout << bstr << "  " << pid << "  " << vt << "\n";
	}
	for (int i = 0; i < nitem; i++)
	{
		for (int j = 0; j < npro; j++)
		{
			BSTR bstr;
			PROPID pid;
			VARTYPE vt;
			CHECKHRESULT(iina->GetPropertyInfo(j, &bstr, &pid, &vt));
			PROPVARIANT var;
			CHECKHRESULT(iina->GetProperty(i, pid, &var));
			if (vt == VT_BSTR  && var.vt != 0)
			wcout << ( var.bstrVal ) << L"\n";
		}
		cout << "=====================================\n";
	}
	cout << "end"; getchar();
    return 0;
}

