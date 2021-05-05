// Lexilla lexer library
/** @file Lexilla.cxx
 ** Lexer infrastructure.
 ** Provides entry points to shared library.
 **/
// Copyright 2019 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstring>

#include <vector>
#include <initializer_list>

#if _WIN32
#define EXPORT_FUNCTION __declspec(dllexport)
#define CALLING_CONVENTION __stdcall
#else
#define EXPORT_FUNCTION __attribute__((visibility("default")))
#define CALLING_CONVENTION
#endif

#include "ILexer.h"

#include "LexerModule.h"
#include "CatalogueModules.h"

using namespace Scintilla;

//++Autogenerated -- run lexilla/scripts/LexillaGen.py to regenerate
//**\(extern LexerModule \*;\n\)
extern LexerModule lmA68k;
extern LexerModule lmAbaqus;
extern LexerModule lmAda;
extern LexerModule lmAPDL;
extern LexerModule lmAs;
extern LexerModule lmAsm;
extern LexerModule lmAsn1;
extern LexerModule lmASY;
extern LexerModule lmAU3;
extern LexerModule lmAVE;
extern LexerModule lmAVS;
extern LexerModule lmBaan;
extern LexerModule lmBash;
extern LexerModule lmBatch;
extern LexerModule lmBibTeX;
extern LexerModule lmBlitzBasic;
extern LexerModule lmBullant;
extern LexerModule lmCaml;
extern LexerModule lmCIL;
extern LexerModule lmClw;
extern LexerModule lmClwNoCase;
extern LexerModule lmCmake;
extern LexerModule lmCOBOL;
extern LexerModule lmCoffeeScript;
extern LexerModule lmConf;
extern LexerModule lmCPP;
extern LexerModule lmCPPNoCase;
extern LexerModule lmCsound;
extern LexerModule lmCss;
extern LexerModule lmD;
extern LexerModule lmDataflex;
extern LexerModule lmDiff;
extern LexerModule lmDMAP;
extern LexerModule lmDMIS;
extern LexerModule lmECL;
extern LexerModule lmEDIFACT;
extern LexerModule lmEiffel;
extern LexerModule lmEiffelkw;
extern LexerModule lmErlang;
extern LexerModule lmErrorList;
extern LexerModule lmESCRIPT;
extern LexerModule lmF77;
extern LexerModule lmFlagShip;
extern LexerModule lmForth;
extern LexerModule lmFortran;
extern LexerModule lmFreeBasic;
extern LexerModule lmFSharp;
extern LexerModule lmGAP;
extern LexerModule lmGui4Cli;
extern LexerModule lmHaskell;
extern LexerModule lmHollywood;
extern LexerModule lmHTML;
extern LexerModule lmIHex;
extern LexerModule lmIndent;
extern LexerModule lmInno;
extern LexerModule lmJSON;
extern LexerModule lmKix;
extern LexerModule lmKVIrc;
extern LexerModule lmLatex;
extern LexerModule lmLISP;
extern LexerModule lmLiterateHaskell;
extern LexerModule lmLot;
extern LexerModule lmLout;
extern LexerModule lmLua;
extern LexerModule lmMagikSF;
extern LexerModule lmMake;
extern LexerModule lmMarkdown;
extern LexerModule lmMatlab;
extern LexerModule lmMaxima;
extern LexerModule lmMETAPOST;
extern LexerModule lmMMIXAL;
extern LexerModule lmModula;
extern LexerModule lmMSSQL;
extern LexerModule lmMySQL;
extern LexerModule lmNim;
extern LexerModule lmNimrod;
extern LexerModule lmNncrontab;
extern LexerModule lmNsis;
extern LexerModule lmNull;
extern LexerModule lmOctave;
extern LexerModule lmOpal;
extern LexerModule lmOScript;
extern LexerModule lmPascal;
extern LexerModule lmPB;
extern LexerModule lmPerl;
extern LexerModule lmPHPSCRIPT;
extern LexerModule lmPLM;
extern LexerModule lmPO;
extern LexerModule lmPOV;
extern LexerModule lmPowerPro;
extern LexerModule lmPowerShell;
extern LexerModule lmProgress;
extern LexerModule lmProps;
extern LexerModule lmPS;
extern LexerModule lmPureBasic;
extern LexerModule lmPython;
extern LexerModule lmR;
extern LexerModule lmRaku;
extern LexerModule lmREBOL;
extern LexerModule lmRegistry;
extern LexerModule lmRuby;
extern LexerModule lmRust;
extern LexerModule lmSAS;
extern LexerModule lmScriptol;
extern LexerModule lmSmalltalk;
extern LexerModule lmSML;
extern LexerModule lmSorc;
extern LexerModule lmSpecman;
extern LexerModule lmSpice;
extern LexerModule lmSQL;
extern LexerModule lmSrec;
extern LexerModule lmStata;
extern LexerModule lmSTTXT;
extern LexerModule lmTACL;
extern LexerModule lmTADS3;
extern LexerModule lmTAL;
extern LexerModule lmTCL;
extern LexerModule lmTCMD;
extern LexerModule lmTEHex;
extern LexerModule lmTeX;
extern LexerModule lmTxt2tags;
extern LexerModule lmVB;
extern LexerModule lmVBScript;
extern LexerModule lmVerilog;
extern LexerModule lmVHDL;
extern LexerModule lmVisualProlog;
extern LexerModule lmX12;
extern LexerModule lmXML;
extern LexerModule lmYAML;

//--Autogenerated -- end of automatically generated section

namespace {

CatalogueModules catalogueLexilla;

void AddEachLexer() {

	if (catalogueLexilla.Count() > 0) {
		return;
	}

	catalogueLexilla.AddLexerModules({
//++Autogenerated -- run scripts/LexillaGen.py to regenerate
//**\(\t\t&\*,\n\)
		&lmA68k,
		&lmAbaqus,
		&lmAda,
		&lmAPDL,
		&lmAs,
		&lmAsm,
		&lmAsn1,
		&lmASY,
		&lmAU3,
		&lmAVE,
		&lmAVS,
		&lmBaan,
		&lmBash,
		&lmBatch,
		&lmBibTeX,
		&lmBlitzBasic,
		&lmBullant,
		&lmCaml,
		&lmCIL,
		&lmClw,
		&lmClwNoCase,
		&lmCmake,
		&lmCOBOL,
		&lmCoffeeScript,
		&lmConf,
		&lmCPP,
		&lmCPPNoCase,
		&lmCsound,
		&lmCss,
		&lmD,
		&lmDataflex,
		&lmDiff,
		&lmDMAP,
		&lmDMIS,
		&lmECL,
		&lmEDIFACT,
		&lmEiffel,
		&lmEiffelkw,
		&lmErlang,
		&lmErrorList,
		&lmESCRIPT,
		&lmF77,
		&lmFlagShip,
		&lmForth,
		&lmFortran,
		&lmFreeBasic,
		&lmFSharp,
		&lmGAP,
		&lmGui4Cli,
		&lmHaskell,
		&lmHollywood,
		&lmHTML,
		&lmIHex,
		&lmIndent,
		&lmInno,
		&lmJSON,
		&lmKix,
		&lmKVIrc,
		&lmLatex,
		&lmLISP,
		&lmLiterateHaskell,
		&lmLot,
		&lmLout,
		&lmLua,
		&lmMagikSF,
		&lmMake,
		&lmMarkdown,
		&lmMatlab,
		&lmMaxima,
		&lmMETAPOST,
		&lmMMIXAL,
		&lmModula,
		&lmMSSQL,
		&lmMySQL,
		&lmNim,
		&lmNimrod,
		&lmNncrontab,
		&lmNsis,
		&lmNull,
		&lmOctave,
		&lmOpal,
		&lmOScript,
		&lmPascal,
		&lmPB,
		&lmPerl,
		&lmPHPSCRIPT,
		&lmPLM,
		&lmPO,
		&lmPOV,
		&lmPowerPro,
		&lmPowerShell,
		&lmProgress,
		&lmProps,
		&lmPS,
		&lmPureBasic,
		&lmPython,
		&lmR,
		&lmRaku,
		&lmREBOL,
		&lmRegistry,
		&lmRuby,
		&lmRust,
		&lmSAS,
		&lmScriptol,
		&lmSmalltalk,
		&lmSML,
		&lmSorc,
		&lmSpecman,
		&lmSpice,
		&lmSQL,
		&lmSrec,
		&lmStata,
		&lmSTTXT,
		&lmTACL,
		&lmTADS3,
		&lmTAL,
		&lmTCL,
		&lmTCMD,
		&lmTEHex,
		&lmTeX,
		&lmTxt2tags,
		&lmVB,
		&lmVBScript,
		&lmVerilog,
		&lmVHDL,
		&lmVisualProlog,
		&lmX12,
		&lmXML,
		&lmYAML,

//--Autogenerated -- end of automatically generated section
		});

}

}

extern "C" {

EXPORT_FUNCTION int CALLING_CONVENTION GetLexerCount() {
	AddEachLexer();
	return catalogueLexilla.Count();
}

EXPORT_FUNCTION void CALLING_CONVENTION GetLexerName(unsigned int index, char *name, int buflength) {
	AddEachLexer();
	*name = 0;
	const char *lexerName = catalogueLexilla.Name(index);
	if (static_cast<size_t>(buflength) > strlen(lexerName)) {
		strcpy(name, lexerName);
	}
}

EXPORT_FUNCTION LexerFactoryFunction CALLING_CONVENTION GetLexerFactory(unsigned int index) {
	AddEachLexer();
	return catalogueLexilla.Factory(index);
}

EXPORT_FUNCTION ILexer5 * CALLING_CONVENTION CreateLexer(const char *name) {
	AddEachLexer();
	for (unsigned int i = 0; i < catalogueLexilla.Count(); i++) {
		const char *lexerName = catalogueLexilla.Name(i);
		if (0 == strcmp(lexerName, name)) {
			return catalogueLexilla.Create(i);
		}
	}
	return nullptr;
}

EXPORT_FUNCTION const char * CALLING_CONVENTION LexerNameFromID(int identifier) {
	const LexerModule *pModule = catalogueLexilla.Find(identifier);
	if (pModule) {
		return pModule->languageName;
	} else {
		return nullptr;
	}
}

EXPORT_FUNCTION const char * CALLING_CONVENTION GetLibraryPropertyNames() {
	return "";
}

EXPORT_FUNCTION void CALLING_CONVENTION SetLibraryProperty(const char *, const char *) {
	// Null implementation
}

}
