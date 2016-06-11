#include "pfmHeader.h"

int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
//int wmain(int argc, const wchar_t*const* argv)
{
	if (pCmdLine == 0 || pCmdLine[0] == 0) return 0;

	wchar_t path[MAX_PATH + 1] = { 0 };
	int s = 0, e = lstrlenW(pCmdLine);
	if (pCmdLine[s] == L'\"' && pCmdLine[e - 1] == L'\"')
	{
		s++;
		e--;
	}
	memcpy(path, pCmdLine + s, e * sizeof(wchar_t));

	int err = 0;
	PfmApi* pfm = 0;
	PfmMount* mount = 0;
	FD_T toFormatterRead = FD_INVALID;
	FD_T fromFormatterWrite = FD_INVALID;
	PfmMountCreateParams mcp;
	PfmMarshallerServeParams msp;
	//PfmFormatterDispatch* volume = GetPfmFormatterDispatch(L"C:\\Users\\ccc\\Documents\\Visual Studio 2015\\Projects\\pfm\\pfm_archive\\Debug\\3.zip");
	PfmFormatterDispatch* volume = GetPfmFormatterDispatch(path);
	PfmMarshaller* marshaller = 0;

	mcp.toFormatterWrite = FD_INVALID;
	mcp.fromFormatterRead = FD_INVALID;
	msp.dispatch = volume;
	msp.formatterName = "Archive";
	msp.toFormatterRead = FD_INVALID;
	msp.fromFormatterWrite = FD_INVALID;

	wchar_t name[1024] = { 0 };
	for (int i = lstrlenW(path) - 1;; i--)
	{
		if (path[i] == L'\\')
		{
			//lstrcpyW(name, argv[1][i + 1])
			mcp.mountSourceName = path + i + 1;
			break;
		}
	}
	DWORD drivers = GetLogicalDrives();
	if (drivers != 0)
	{
		for (int i = 8; i < 26; i++)
		{
			if (!(drivers & 1 << i))
			{
				mcp.driveLetter = L'A' + i;
				break;
			}
		}
	}

	if (!err)
	{
		err = PfmApiFactory(&pfm);
		if (err)
		{
			printf("ERROR: %i Unable to open PFM Api.\n", err);
		}
	}

	if (!err)
	{
		err = PfmMarshallerFactory(&marshaller);
		if (err)
		{
			printf("ERROR: %i Unable to create marshaller.\n", err);
		}
	}
	if (!err)
	{
		// Communication between the driver and file system is done
		// over a pair of simple pipes. Application is responsible
		// for creating the pipes.
		err = create_pipe(&msp.toFormatterRead, &mcp.toFormatterWrite);
		if (!err)
		{
			err = create_pipe(&mcp.fromFormatterRead, &msp.fromFormatterWrite);
		}
	}
	if (!err)
	{
		// Various mount options are available through mountFlags,
		// visibleProcessId, and ownerSid.
		err = pfm->MountCreate(&mcp, &mount);
		if (err)
		{
			printf("ERROR: %i Unable to create mount.\n", err);
		}
	}
	// Close driver end pipe handles now. Driver has duplicated what
	// it needs. If these handles are not closed then pipes will not
	// break if driver disconnects, leaving us stuck in the
	// marshaller.
	close_fd(mcp.toFormatterWrite);
	close_fd(mcp.fromFormatterRead);

	if (!err)
	{
		// If tracemon is installed and running then diagnostic
		// messsages can be viewed in the "hellofs" channel.
		marshaller->SetTrace(L"hellofs");
		// Also send diagnostic messages to stdout. This can slow
		// things down quite a bit.
		marshaller->SetStatus(stdout_fd());

		// The marshaller uses alertable I/O, so process can be
		// exited via ctrl+c.
		printf("Press CTRL+C to exit.\n");
		// The marshaller serve function will return at unmount or
		// if driver disconnects.
		marshaller->ServeDispatch(&msp);
	}

	close_fd(msp.toFormatterRead);
	close_fd(msp.fromFormatterWrite);
	if (mount)
	{
		mount->Release();
	}
	if (pfm)
	{
		pfm->Release();
	}
	if (marshaller)
	{
		marshaller->Release();
		marshaller = 0;
	}
	PfmApiUnload();

	delete volume;
	return err;
}