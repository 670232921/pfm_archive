#include "ArchiveVolume.h"

static PfmOpenAttribs zeroOpenAttribs = {};
static PfmAttribs zeroAttribs = {};
static PfmMediaInfo zeroMediaInfo = {};

static const wchar_t helloFileName[] = L"readme.txt";
static const char helloData[] = "Hello world.\r\n";
static const size_t helloDataSize = sizeof(helloData) - sizeof(helloData[0]);
static const int64_t helloRootId = 2;
static const int64_t helloFileId = 3;

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
	const wchar_t* endName = 0;

	if (!namePartCount)
	{
		if (!folderOpenId)
		{
			folderOpenId = newExistingOpenId;
		}
		existed = true;
		openAttribs.openId = folderOpenId;
		openAttribs.openSequence = 1;
		openAttribs.accessLevel = pfmAccessLevelReadData;
		openAttribs.attribs.fileType = pfmFileTypeFolder;
		openAttribs.attribs.fileId = helloRootId;
	}
	else if (namePartCount == 1)
	{
		if (sswcmpf(nameParts[0].name, helloFileName) != 0)
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
		}
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
	else if (openId == folderOpenId)
	{
		openAttribs.openId = folderOpenId;
		openAttribs.openSequence = 1;
		openAttribs.accessLevel = pfmAccessLevelReadData;
		openAttribs.attribs.fileType = pfmFileTypeFolder;
		openAttribs.attribs.fileId = helloRootId;
	}
	else if (openId == fileOpenId)
	{
		openAttribs.openId = fileOpenId;
		openAttribs.openSequence = 1;
		openAttribs.accessLevel = pfmAccessLevelReadData;
		openAttribs.attribs.fileType = pfmFileTypeFile;
		openAttribs.attribs.fileId = helloFileId;
		openAttribs.attribs.fileSize = helloDataSize;
	}
	else
	{
		perr = pfmErrorNotFound;
	}

	op->Complete(perr, &openAttribs, 0);
}

void CCALL ArchiveVolume::List(PfmMarshallerListOp* op, void* formatterUse)
{
	int64_t openId = op->OpenId();
	int perr = 0;
	PfmAttribs attribs = zeroAttribs;

	if (openId != folderOpenId)
	{
		perr = pfmErrorAccessDenied;
	}
	else
	{
		attribs.fileType = pfmFileTypeFile;
		attribs.fileId = helloFileId;
		attribs.fileSize = helloDataSize;
		op->Add(&attribs, helloFileName);
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

	if (openId != fileOpenId)
	{
		perr = pfmErrorAccessDenied;
	}
	else
	{
		actualSize = requestedSize;
		if (fileOffset > helloDataSize)
		{
			actualSize = 0;
		}
		else if (fileOffset + requestedSize > helloDataSize)
		{
			actualSize = static_cast<size_t>(helloDataSize - fileOffset);
		}
		memcpy(data, helloData, actualSize);
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
	op->Complete(pfmErrorSuccess, helloDataSize/*totalCapacity*/, 0/*availableCapacity*/);
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
}

void CCALL ArchiveVolume::Access(PfmMarshallerAccessOp* op, void* formatterUse)
{
	int64_t openId = op->OpenId();
	int perr = 0;
	PfmOpenAttribs openAttribs = zeroOpenAttribs;

	if (openId == folderOpenId)
	{
		openAttribs.openId = folderOpenId;
		openAttribs.openSequence = 1;
		openAttribs.accessLevel = pfmAccessLevelReadData;
		openAttribs.attribs.fileType = pfmFileTypeFolder;
		openAttribs.attribs.fileId = helloRootId;
		perr = 0;
	}
	else if (openId == fileOpenId)
	{
		openAttribs.openId = fileOpenId;
		openAttribs.openSequence = 1;
		openAttribs.accessLevel = pfmAccessLevelReadData;
		openAttribs.attribs.fileType = pfmFileTypeFile;
		openAttribs.attribs.fileId = helloFileId;
		openAttribs.attribs.fileSize = helloDataSize;
		perr = 0;
	}
	else
	{
		perr = pfmErrorNotFound;
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

ArchiveVolume::ArchiveVolume(void)
{
	marshaller = 0;
	folderOpenId = 0;
	fileOpenId = 0;
}

ArchiveVolume::~ArchiveVolume(void)
{
	ASSERT(!marshaller);
}