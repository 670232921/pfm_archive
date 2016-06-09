#pragma once
#include "ArchiveWrap.h"
#include "pfmHeader.h"

#include <map>
#define CCALL PT_CCALL
typedef unsigned int UInt32;

class ArchiveVolume : public PfmFormatterDispatch
{
public:
	ArchiveVolume(LPCWSTR filePath);

	bool Aviliable() { return _aviliable; }

	// PfmFormatterDispatch
	void CCALL Open(PfmMarshallerOpenOp* op, void* formatterUse);
	void CCALL Replace(PfmMarshallerReplaceOp* op, void* formatterUse);
	void CCALL Move(PfmMarshallerMoveOp* op, void* formatterUse);
	void CCALL MoveReplace(PfmMarshallerMoveReplaceOp* op, void* formatterUse);
	void CCALL Delete(PfmMarshallerDeleteOp* op, void* formatterUse);
	void CCALL Close(PfmMarshallerCloseOp* op, void* formatterUse);
	void CCALL FlushFile(PfmMarshallerFlushFileOp* op, void* formatterUse);
	void CCALL List(PfmMarshallerListOp* op, void* formatterUse);
	void CCALL ListEnd(PfmMarshallerListEndOp* op, void* formatterUse);
	void CCALL Read(PfmMarshallerReadOp* op, void* formatterUse);
	void CCALL Write(PfmMarshallerWriteOp* op, void* formatterUse);
	void CCALL SetSize(PfmMarshallerSetSizeOp* op, void* formatterUse);
	void CCALL Capacity(PfmMarshallerCapacityOp* op, void* formatterUse);
	void CCALL FlushMedia(PfmMarshallerFlushMediaOp* op, void* formatterUse);
	void CCALL Control(PfmMarshallerControlOp* op, void* formatterUse);
	void CCALL MediaInfo(PfmMarshallerMediaInfoOp* op, void* formatterUse);
	void CCALL Access(PfmMarshallerAccessOp* op, void* formatterUse);
	void CCALL ReadXattr(PfmMarshallerReadXattrOp* op, void* formatterUse);
	void CCALL WriteXattr(PfmMarshallerWriteXattrOp* op, void* formatterUse);

private:

	HRESULT _ret = 0;
	std::wstring _fileName;
	size_t _size = 0;
	std::map<int64_t, UInt32> _openIDArchiveIDMap;
	ArchiveWrap _archive;
	bool _aviliable = false;


	PfmAttribs GetFileAttribute(UInt32 id);
	UInt32 GetArchiveID(int64_t oid) { return _openIDArchiveIDMap[oid]; }
	UInt32 GetArchiveID(const PfmNamePart *parts, size_t count);
	int64_t GetOpenID(UInt32 id);
	inline void SaveID(int64_t oid, UInt32 id) { _openIDArchiveIDMap[oid] = id; }
	inline bool IsOpened(int64_t oid) { return _openIDArchiveIDMap.find(oid) != _openIDArchiveIDMap.end(); }
	bool IsOpenedId(UInt32 id);
	size_t List(UInt32 id, PfmMarshallerListOp* op);

	std::wstring GetPathPro(UInt32 id);
	std::wstring GetEndName(UInt32 id);
	std::wstring GetEndName(std::wstring path);
};