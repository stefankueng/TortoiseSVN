// ResText.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ResModule.h"

#include <string>
#include <vector>
#include "shlwapi.h"
#pragma comment(lib, "shlwapi.lib")

typedef std::basic_string<wchar_t> wstring;
#ifdef UNICODE
#	define stdstring wstring
#else
#	define stdstring std::string
#endif

int _tmain(int argc, _TCHAR* argv[])
{
	bool bShowHelp = true;
	bool bQuiet = false;
	bool bNoUpdate = false;
	//parse the command line
	std::vector<stdstring> arguments;
	std::vector<stdstring> switches;
	for (int i=1; i<argc; ++i)
	{
		if ((argv[i][0] == '-')||(argv[i][0] == '/'))
		{
			stdstring str = stdstring(&argv[i][1]);
			switches.push_back(str);
		}
		else
		{
			stdstring str = stdstring(&argv[i][0]);
			arguments.push_back(str);
		}
	}

	for (std::vector<stdstring>::iterator I = switches.begin(); I != switches.end(); ++I)
	{
		if (_tcscmp(I->c_str(), _T("?"))==0)
			bShowHelp = true;
		if (_tcscmp(I->c_str(), _T("help"))==0)
			bShowHelp = true;
		if (_tcscmp(I->c_str(), _T("quiet"))==0)
			bQuiet = true;
		if (_tcscmp(I->c_str(), _T("noupdate"))==0)
			bNoUpdate = true;
	}
	std::vector<stdstring>::iterator arg = arguments.begin();

	if (arg != arguments.end())
	{
		if (_tcscmp(arg->c_str(), _T("extract"))==0)
		{
			stdstring sDllFile;
			stdstring sPoFile;
			++arg;
			
			std::vector<std::wstring> filelist = arguments;
			filelist.erase(filelist.begin());
			sPoFile = stdstring((--filelist.end())->c_str());
			filelist.erase(--filelist.end());
			
			CResModule module;
			module.SetQuiet(bQuiet);
			if (!module.ExtractResources(filelist, sPoFile.c_str(), bNoUpdate))
				return -1;
			bShowHelp = false;
		}
		else if (_tcscmp(arg->c_str(), _T("apply"))==0)
		{
			stdstring sSrcDllFile;
			stdstring sDstDllFile;
			stdstring sPoFile;
			++arg;
			if (!PathFileExists(arg->c_str()))
			{
				_ftprintf(stderr, _T("the resource dll <%s> does not exist!\n"), arg->c_str());
				return -1;
			}
			sSrcDllFile = stdstring(arg->c_str());
			++arg;
			sDstDllFile = stdstring(arg->c_str());
			++arg;
			if (!PathFileExists(arg->c_str()))
			{
				_ftprintf(stderr, _T("the po-file <%s> does not exist!\n"), arg->c_str());
				return -1;
			}
			sPoFile = stdstring(arg->c_str());
			CResModule module;
			module.SetQuiet(bQuiet);
			if (!module.CreateTranslatedResources(sSrcDllFile.c_str(), sDstDllFile.c_str(), sPoFile.c_str()))
				return -1;
			bShowHelp = false;
		}
	}

	if (bShowHelp)
	{
		_tcprintf(_T("usage:\n"));
		_tcprintf(_T("\n"));
		_tcprintf(_T("ResText extract <resource.dll> [<resource.dll> ...] <po-file> [-quiet] [-noupdate]\n"));
		_tcprintf(_T("Extracts all strings from the resource dll and writes them to the po-file\n"));
		_tcprintf(_T("-quiet: don't print progress messages\n"));
		_tcprintf(_T("-noupdate: overwrite the po-file\n"));
		_tcprintf(_T("\n"));
		_tcprintf(_T("ResText apply <src resource.dll> <dst resource.dll> <po-file> [-quiet]\n"));
		_tcprintf(_T("Replaces all strings in the dst resource.dll with the po-file translations\n"));
		_tcprintf(_T("-quiet: don't print progress messages\n"));
		_tcprintf(_T("\n"));
	}

	return 0;
}

