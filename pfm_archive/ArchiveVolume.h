#pragma once
#include "../samples/common/portability.c"
#include "../include/pfmapi.h"
#define CCALL PT_CCALL


struct ArchiveVolume : PfmFormatterDispatch
{
	PfmMarshaller* marshaller;
	int64_t folderOpenId;
	int64_t fileOpenId;

	ArchiveVolume(void);
	~ArchiveVolume(void);

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
};