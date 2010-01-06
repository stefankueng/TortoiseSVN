// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2010 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "StdAfx.h"
#include "TortoiseProc.h"
#include "UnicodeUtils.h"
#include "ProjectProperties.h"
#include "SVNProperties.h"
#include "TSVNPath.h"
#include <regex>

using namespace std;

#define LOG_REVISIONREGEX _T("\\b(r\\d+)|\\b(revisions?(\\(s\\))?\\s#?\\d+([, ]+(and\\s?)?\\d+)*)|\\b(revs?\\.?\\s?\\d+([, ]+(and\\s?)?\\d+)*)")


ProjectProperties::ProjectProperties(void)
    : regExNeedUpdate (true)
	, bNumber (TRUE)
	, bWarnIfNoIssue (FALSE)
	, nLogWidthMarker (0)
	, nMinLogSize (0)
	, nMinLockMsgSize (0)
	, bFileListInEnglish (TRUE)
	, bAppend (TRUE)
	, lProjectLanguage (0)
{
}

ProjectProperties::~ProjectProperties(void)
{
}


BOOL ProjectProperties::ReadPropsPathList(const CTSVNPathList& pathList)
{
	for(int nPath = 0; nPath < pathList.GetCount(); nPath++)
	{
		if (ReadProps(pathList[nPath]))
		{
			return TRUE;
		}
	}
	return FALSE;
}

BOOL ProjectProperties::ReadProps(CTSVNPath path)
{
    regExNeedUpdate = true;

	BOOL bFoundBugtraqLabel = FALSE;
	BOOL bFoundBugtraqMessage = FALSE;
	BOOL bFoundBugtraqNumber = FALSE;
	BOOL bFoundBugtraqLogRe = FALSE;
	BOOL bFoundBugtraqURL = FALSE;
	BOOL bFoundBugtraqWarnIssue = FALSE;
	BOOL bFoundBugtraqAppend = FALSE;
	BOOL bFoundLogWidth = FALSE;
	BOOL bFoundLogTemplate = FALSE;
	BOOL bFoundMinLogSize = FALSE;
	BOOL bFoundMinLockMsgSize = FALSE;
	BOOL bFoundFileListEnglish = FALSE;
	BOOL bFoundProjectLanguage = FALSE;
	BOOL bFoundUserFileProps = FALSE;
	BOOL bFoundUserDirProps = FALSE;
	BOOL bFoundWebViewRev = FALSE;
	BOOL bFoundWebViewPathRev = FALSE;
	BOOL bFoundAutoProps = FALSE;
	BOOL bFoundLogSummary = FALSE;
	BOOL bFoundBugtraqProviderUuid = FALSE;
	BOOL bFoundBugtraqProviderParams = FALSE;
	BOOL bFoundLogRevRegex = FALSE;

	if (!path.IsDirectory())
		path = path.GetContainingDirectory();
		
	for (;;)
	{
		SVNProperties props(path, SVNRev::REV_WC, false);
		for (int i=0; i<props.GetCount(); ++i)
		{
			std::string sPropName = props.GetItemName(i);
			CString sPropVal = CUnicodeUtils::GetUnicode(((char *)props.GetItemValue(i).c_str()));
			if ((!bFoundBugtraqLabel)&&(sPropName.compare (BUGTRAQPROPNAME_LABEL)==0))
			{
				sLabel = sPropVal;
				bFoundBugtraqLabel = TRUE;
			}
			if ((!bFoundBugtraqMessage)&&(sPropName.compare(BUGTRAQPROPNAME_MESSAGE)==0))
			{
				sMessage = sPropVal;
				bFoundBugtraqMessage = TRUE;
			}
			if ((!bFoundBugtraqNumber)&&(sPropName.compare(BUGTRAQPROPNAME_NUMBER)==0))
			{
				CString val;
				val = sPropVal;
				val = val.Trim(_T(" \n\r\t"));
				if ((val.CompareNoCase(_T("false"))==0)||(val.CompareNoCase(_T("no"))==0))
					bNumber = FALSE;
				else
					bNumber = TRUE;
				bFoundBugtraqNumber = TRUE;
			}
			if ((!bFoundBugtraqLogRe)&&(sPropName.compare(BUGTRAQPROPNAME_LOGREGEX)==0))
			{
				sCheckRe = sPropVal;
				if (sCheckRe.Find('\n')>=0)
				{
					sBugIDRe = sCheckRe.Mid(sCheckRe.Find('\n')).Trim();
					sCheckRe = sCheckRe.Left(sCheckRe.Find('\n')).Trim();
				}
				if (!sCheckRe.IsEmpty())
				{
					sCheckRe = sCheckRe.Trim();
				}
				bFoundBugtraqLogRe = TRUE;
			}
			if ((!bFoundBugtraqURL)&&(sPropName.compare(BUGTRAQPROPNAME_URL)==0))
			{
				sUrl = sPropVal;
				bFoundBugtraqURL = TRUE;
			}
			if ((!bFoundBugtraqWarnIssue)&&(sPropName.compare(BUGTRAQPROPNAME_WARNIFNOISSUE)==0))
			{
				CString val;
				val = sPropVal;
				val = val.Trim(_T(" \n\r\t"));
				if ((val.CompareNoCase(_T("true"))==0)||(val.CompareNoCase(_T("yes"))==0))
					bWarnIfNoIssue = TRUE;
				else
					bWarnIfNoIssue = FALSE;
				bFoundBugtraqWarnIssue = TRUE;
			}
			if ((!bFoundBugtraqAppend)&&(sPropName.compare(BUGTRAQPROPNAME_APPEND)==0))
			{
				CString val;
				val = sPropVal;
				val = val.Trim(_T(" \n\r\t"));
				if ((val.CompareNoCase(_T("true"))==0)||(val.CompareNoCase(_T("yes"))==0))
					bAppend = TRUE;
				else
					bAppend = FALSE;
				bFoundBugtraqAppend = TRUE;
			}
			if ((!bFoundBugtraqProviderUuid)&&(sPropName.compare(BUGTRAQPROPNAME_PROVIDERUUID)==0))
			{
				sProviderUuid = sPropVal;
				bFoundBugtraqProviderUuid = TRUE;
			}
			if ((!bFoundBugtraqProviderParams)&&(sPropName.compare(BUGTRAQPROPNAME_PROVIDERPARAMS)==0))
			{
				sProviderParams = sPropVal;
				bFoundBugtraqProviderParams = TRUE;
			}
			if ((!bFoundLogWidth)&&(sPropName.compare(PROJECTPROPNAME_LOGWIDTHLINE)==0))
			{
				CString val;
				val = sPropVal;
				if (!val.IsEmpty())
				{
					nLogWidthMarker = _ttoi(val);
				}
				bFoundLogWidth = TRUE;
			}
			if ((!bFoundLogTemplate)&&(sPropName.compare(PROJECTPROPNAME_LOGTEMPLATE)==0))
			{
				sLogTemplate = sPropVal;
				sLogTemplate.Replace(_T("\r"), _T(""));
				sLogTemplate.Replace(_T("\n"), _T("\r\n"));
				bFoundLogTemplate = TRUE;
			}
			if ((!bFoundMinLogSize)&&(sPropName.compare(PROJECTPROPNAME_LOGMINSIZE)==0))
			{
				CString val;
				val = sPropVal;
				if (!val.IsEmpty())
				{
					nMinLogSize = _ttoi(val);
				}
				bFoundMinLogSize = TRUE;
			}
			if ((!bFoundMinLockMsgSize)&&(sPropName.compare(PROJECTPROPNAME_LOCKMSGMINSIZE)==0))
			{
				CString val;
				val = sPropVal;
				if (!val.IsEmpty())
				{
					nMinLockMsgSize = _ttoi(val);
				}
				bFoundMinLockMsgSize = TRUE;
			}
			if ((!bFoundFileListEnglish)&&(sPropName.compare(PROJECTPROPNAME_LOGFILELISTLANG)==0))
			{
				CString val;
				val = sPropVal;
				val = val.Trim(_T(" \n\r\t"));
				if ((val.CompareNoCase(_T("false"))==0)||(val.CompareNoCase(_T("no"))==0))
					bFileListInEnglish = TRUE;
				else
					bFileListInEnglish = FALSE;
				bFoundFileListEnglish = TRUE;
			}
			if ((!bFoundProjectLanguage)&&(sPropName.compare(PROJECTPROPNAME_PROJECTLANGUAGE)==0))
			{
				CString val;
				val = sPropVal;
				if (!val.IsEmpty())
				{
					LPTSTR strEnd;
					lProjectLanguage = _tcstol(val, &strEnd, 0);
				}
				bFoundProjectLanguage = TRUE;
			}
			if ((!bFoundUserFileProps)&&(sPropName.compare(PROJECTPROPNAME_USERFILEPROPERTY)==0))
			{
				sFPPath = sPropVal;
				sFPPath.Replace(_T("\r\n"), _T("\n"));
				bFoundUserFileProps = TRUE;
			}
			if ((!bFoundUserDirProps)&&(sPropName.compare(PROJECTPROPNAME_USERDIRPROPERTY)==0))
			{
				sDPPath = sPropVal;
				sDPPath.Replace(_T("\r\n"), _T("\n"));
				bFoundUserDirProps = TRUE;
			}
			if ((!bFoundAutoProps)&&(sPropName.compare(PROJECTPROPNAME_AUTOPROPS)==0))
			{
				sAutoProps = sPropVal;
				sAutoProps.Replace(_T("\r\n"), _T("\n"));
				bFoundAutoProps = TRUE;
			}
			if ((!bFoundWebViewRev)&&(sPropName.compare(PROJECTPROPNAME_WEBVIEWER_REV)==0))
			{
				sWebViewerRev = sPropVal;
				bFoundWebViewRev = TRUE;
			}
			if ((!bFoundWebViewPathRev)&&(sPropName.compare(PROJECTPROPNAME_WEBVIEWER_PATHREV)==0))
			{
				sWebViewerPathRev = sPropVal;
				bFoundWebViewPathRev = TRUE;
			}
			if ((!bFoundLogSummary)&&(sPropName.compare(PROJECTPROPNAME_LOGSUMMARY)==0))
			{
				sLogSummaryRe = sPropVal;
				bFoundLogSummary = TRUE;
			}
			if ((!bFoundLogRevRegex)&&(sPropName.compare(PROJECTPROPNAME_LOGREVREGEX)==0))
			{
				sLogRevRegex = sPropVal;
				bFoundLogRevRegex = TRUE;
			}
		}
		if (PathIsRoot(path.GetWinPath()))
			return FALSE;
		propsPath = path;
		path = path.GetContainingDirectory();
		if ((!path.HasAdminDir())||(path.IsEmpty()))
		{
			if (bFoundBugtraqLabel | bFoundBugtraqMessage | bFoundBugtraqNumber
				| bFoundBugtraqURL | bFoundBugtraqWarnIssue | bFoundLogWidth
				| bFoundLogTemplate | bFoundBugtraqLogRe | bFoundMinLockMsgSize
				| bFoundUserFileProps | bFoundUserDirProps | bFoundAutoProps
				| bFoundWebViewRev | bFoundWebViewPathRev | bFoundLogSummary | bFoundLogRevRegex
				| bFoundBugtraqProviderUuid | bFoundBugtraqProviderParams)
			{
				if (!bFoundLogRevRegex)
					sLogRevRegex = LOG_REVISIONREGEX;
				return TRUE;
			}
			propsPath.Reset();
			return FALSE;
		}
	}
	//return FALSE;		//never reached
}

CString ProjectProperties::GetBugIDFromLog(CString& msg)
{
	CString sBugID;

	if (!sMessage.IsEmpty())
	{
		CString sBugLine;
		CString sFirstPart;
		CString sLastPart;
		BOOL bTop = FALSE;
		if (sMessage.Find(_T("%BUGID%"))<0)
			return sBugID;
		sFirstPart = sMessage.Left(sMessage.Find(_T("%BUGID%")));
		sLastPart = sMessage.Mid(sMessage.Find(_T("%BUGID%"))+7);
		msg.TrimRight('\n');
		if (msg.ReverseFind('\n')>=0)
		{
			if (bAppend)
				sBugLine = msg.Mid(msg.ReverseFind('\n')+1);
			else
			{
				sBugLine = msg.Left(msg.Find('\n'));
				bTop = TRUE;
			}
		}
		else
		{
			if (bNumber)
			{
				// find out if the message consists only of numbers
				bool bOnlyNumbers = true;
				for (int i=0; i<msg.GetLength(); ++i)
				{
					if (!_istdigit(msg[i]))
					{
						bOnlyNumbers = false;
						break;
					}
				}
				if (bOnlyNumbers)
					sBugLine = msg;
			}
			else
				sBugLine = msg;
		}
		if (sBugLine.IsEmpty() && (msg.ReverseFind('\n') < 0))
		{
			sBugLine = msg.Mid(msg.ReverseFind('\n')+1);
		}
		if (sBugLine.Left(sFirstPart.GetLength()).Compare(sFirstPart)!=0)
			sBugLine.Empty();
		if (sBugLine.Right(sLastPart.GetLength()).Compare(sLastPart)!=0)
			sBugLine.Empty();
		if (sBugLine.IsEmpty())
		{
			if (msg.Find('\n')>=0)
				sBugLine = msg.Left(msg.Find('\n'));
			if (sBugLine.Left(sFirstPart.GetLength()).Compare(sFirstPart)!=0)
				sBugLine.Empty();
			if (sBugLine.Right(sLastPart.GetLength()).Compare(sLastPart)!=0)
				sBugLine.Empty();
			bTop = TRUE;
		}
		if (sBugLine.IsEmpty())
			return sBugID;
		sBugID = sBugLine.Mid(sFirstPart.GetLength(), sBugLine.GetLength() - sFirstPart.GetLength() - sLastPart.GetLength());
		if (bTop)
		{
			msg = msg.Mid(sBugLine.GetLength());
			msg.TrimLeft('\n');
		}
		else
		{
			msg = msg.Left(msg.GetLength()-sBugLine.GetLength());
			msg.TrimRight('\n');
		}
	}
	return sBugID;
}

void ProjectProperties::AutoUpdateRegex()
{
    if (regExNeedUpdate)
    {
		regCheck = tr1::wregex (sCheckRe);
		regBugID = tr1::wregex (sBugIDRe);

        regExNeedUpdate = false;
    }
}

BOOL ProjectProperties::FindBugID(const CString& msg, CWnd * pWnd)
{
	size_t offset1 = 0;
	size_t offset2 = 0;
	bool bFound = false;

	if (sUrl.IsEmpty())
		return FALSE;

	// first use the checkre string to find bug ID's in the message
	if (!sCheckRe.IsEmpty())
	{
		if (!sBugIDRe.IsEmpty())
		{

			// match with two regex strings (without grouping!)
			try
			{
                AutoUpdateRegex();
				const tr1::wsregex_iterator end;
				wstring s = msg;
				for (tr1::wsregex_iterator it(s.begin(), s.end(), regCheck); it != end; ++it)
				{
					// (*it)[0] is the matched string
					wstring matchedString = (*it)[0];
					ptrdiff_t matchpos = it->position(0);
					for (tr1::wsregex_iterator it2(matchedString.begin(), matchedString.end(), regBugID); it2 != end; ++it2)
					{
						ATLTRACE(_T("matched id : %s\n"), (*it2)[0].str().c_str());
						ptrdiff_t matchposID = it2->position(0);
						CHARRANGE range = {(LONG)(matchpos+matchposID), (LONG)(matchpos+matchposID+(*it2)[0].str().size())};
						pWnd->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);
						SetLinkCharFormat(pWnd);
						bFound = true;
					}
				}
			}
			catch (exception) {}
		}
		else
		{
			try
			{
                AutoUpdateRegex();
				const tr1::wsregex_iterator end;
				wstring s = msg;
				for (tr1::wsregex_iterator it(s.begin(), s.end(), regCheck); it != end; ++it)
				{
					const tr1::wsmatch match = *it;
					// we define group 1 as the whole issue text and
					// group 2 as the bug ID
					if (match.size() >= 2)
					{
						ATLTRACE(_T("matched id : %s\n"), wstring(match[1]).c_str());
						CHARRANGE range = {(LONG)(match[1].first-s.begin()), (LONG)(match[1].second-s.begin())};
						pWnd->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);
						SetLinkCharFormat(pWnd);
						bFound = true;
					}
				}
			}
			catch (exception) {}
		}
	}
	else if ((!bFound)&&(!sMessage.IsEmpty()))
	{
		CString sBugLine;
		CString sFirstPart;
		CString sLastPart;
		BOOL bTop = FALSE;
		if (sMessage.Find(_T("%BUGID%"))<0)
			return FALSE;
		sFirstPart = sMessage.Left(sMessage.Find(_T("%BUGID%")));
		sLastPart = sMessage.Mid(sMessage.Find(_T("%BUGID%"))+7);
		CString sMsg = msg;
		sMsg.TrimRight('\n');
		if (sMsg.ReverseFind('\n')>=0)
		{
			if (bAppend)
				sBugLine = sMsg.Mid(sMsg.ReverseFind('\n')+1);
			else
			{
				sBugLine = sMsg.Left(sMsg.Find('\n'));
				bTop = TRUE;
			}
		}
		else
			sBugLine = sMsg;
		if (sBugLine.Left(sFirstPart.GetLength()).Compare(sFirstPart)!=0)
			sBugLine.Empty();
		if (sBugLine.Right(sLastPart.GetLength()).Compare(sLastPart)!=0)
			sBugLine.Empty();
		if (sBugLine.IsEmpty())
		{
			if (sMsg.Find('\n')>=0)
				sBugLine = sMsg.Left(sMsg.Find('\n'));
			if (sBugLine.Left(sFirstPart.GetLength()).Compare(sFirstPart)!=0)
				sBugLine.Empty();
			if (sBugLine.Right(sLastPart.GetLength()).Compare(sLastPart)!=0)
				sBugLine.Empty();
			bTop = TRUE;
		}
		if (sBugLine.IsEmpty())
			return FALSE;
		CString sBugIDPart = sBugLine.Mid(sFirstPart.GetLength(), sBugLine.GetLength() - sFirstPart.GetLength() - sLastPart.GetLength());
		if (sBugIDPart.IsEmpty())
			return FALSE;
		//the bug id part can contain several bug id's, separated by commas
		if (!bTop)
			offset1 = sMsg.GetLength() - sBugLine.GetLength() + sFirstPart.GetLength();
		else
			offset1 = sFirstPart.GetLength();
		sBugIDPart.Trim(_T(","));
		while (sBugIDPart.Find(',')>=0)
		{
			offset2 = offset1 + sBugIDPart.Find(',');
			CHARRANGE range = {(LONG)offset1, (LONG)offset2};
			pWnd->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);
			SetLinkCharFormat(pWnd);
			sBugIDPart = sBugIDPart.Mid(sBugIDPart.Find(',')+1);
			offset1 = offset2 + 1;
		}
		offset2 = offset1 + sBugIDPart.GetLength();
		CHARRANGE range = {(LONG)offset1, (LONG)offset2};
		pWnd->SendMessage(EM_EXSETSEL, NULL, (LPARAM)&range);
		SetLinkCharFormat(pWnd);
		return TRUE;
	}
	return FALSE;
}

std::set<CString> ProjectProperties::FindBugIDs(const CString& msg)
{
	size_t offset1 = 0;
	size_t offset2 = 0;
	bool bFound = false;
	std::set<CString> bugIDs;

	// first use the checkre string to find bug ID's in the message
	if (!sCheckRe.IsEmpty())
	{
		if (!sBugIDRe.IsEmpty())
		{
			// match with two regex strings (without grouping!)
			try
			{
                AutoUpdateRegex();
				const tr1::wsregex_iterator end;
				wstring s = msg;
				for (tr1::wsregex_iterator it(s.begin(), s.end(), regCheck); it != end; ++it)
				{
					// (*it)[0] is the matched string
					wstring matchedString = (*it)[0];
					for (tr1::wsregex_iterator it2(matchedString.begin(), matchedString.end(), regBugID); it2 != end; ++it2)
					{
						ATLTRACE(_T("matched id : %s\n"), (*it2)[0].str().c_str());
						bugIDs.insert(CString((*it2)[0].str().c_str()));
					}
				}
			}
			catch (exception) {}
		}
		else
		{
			try
			{
                AutoUpdateRegex();
				const tr1::wsregex_iterator end;
				wstring s = msg;
				for (tr1::wsregex_iterator it(s.begin(), s.end(), regCheck); it != end; ++it)
				{
					const tr1::wsmatch match = *it;
					// we define group 1 as the whole issue text and
					// group 2 as the bug ID
					if (match.size() >= 2)
					{
						ATLTRACE(_T("matched id : %s\n"), wstring(match[1]).c_str());
						bugIDs.insert(CString(wstring(match[1]).c_str()));
					}
				}
			}
			catch (exception) {}
		}
	}
	else if ((!bFound)&&(!sMessage.IsEmpty()))
	{
		CString sBugLine;
		CString sFirstPart;
		CString sLastPart;
		BOOL bTop = FALSE;
		if (sMessage.Find(_T("%BUGID%"))<0)
			return bugIDs;
		sFirstPart = sMessage.Left(sMessage.Find(_T("%BUGID%")));
		sLastPart = sMessage.Mid(sMessage.Find(_T("%BUGID%"))+7);
		CString sMsg = msg;
		sMsg.TrimRight('\n');
		if (sMsg.ReverseFind('\n')>=0)
		{
			if (bAppend)
				sBugLine = sMsg.Mid(sMsg.ReverseFind('\n')+1);
			else
			{
				sBugLine = sMsg.Left(sMsg.Find('\n'));
				bTop = TRUE;
			}
		}
		else
			sBugLine = sMsg;
		if (sBugLine.Left(sFirstPart.GetLength()).Compare(sFirstPart)!=0)
			sBugLine.Empty();
		if (sBugLine.Right(sLastPart.GetLength()).Compare(sLastPart)!=0)
			sBugLine.Empty();
		if (sBugLine.IsEmpty())
		{
			if (sMsg.Find('\n')>=0)
				sBugLine = sMsg.Left(sMsg.Find('\n'));
			if (sBugLine.Left(sFirstPart.GetLength()).Compare(sFirstPart)!=0)
				sBugLine.Empty();
			if (sBugLine.Right(sLastPart.GetLength()).Compare(sLastPart)!=0)
				sBugLine.Empty();
			bTop = TRUE;
		}
		if (sBugLine.IsEmpty())
			return bugIDs;
		CString sBugIDPart = sBugLine.Mid(sFirstPart.GetLength(), sBugLine.GetLength() - sFirstPart.GetLength() - sLastPart.GetLength());
		if (sBugIDPart.IsEmpty())
			return bugIDs;
		//the bug id part can contain several bug id's, separated by commas
		if (!bTop)
			offset1 = sMsg.GetLength() - sBugLine.GetLength() + sFirstPart.GetLength();
		else
			offset1 = sFirstPart.GetLength();
		sBugIDPart.Trim(_T(","));
		while (sBugIDPart.Find(',')>=0)
		{
			offset2 = offset1 + sBugIDPart.Find(',');
			CHARRANGE range = {(LONG)offset1, (LONG)offset2};
			bugIDs.insert(msg.Mid(range.cpMin, range.cpMax-range.cpMin));
			sBugIDPart = sBugIDPart.Mid(sBugIDPart.Find(',')+1);
			offset1 = offset2 + 1;
		}
		offset2 = offset1 + sBugIDPart.GetLength();
		CHARRANGE range = {(LONG)offset1, (LONG)offset2};
		bugIDs.insert(msg.Mid(range.cpMin, range.cpMax-range.cpMin));
	}

	return bugIDs;
}

CString ProjectProperties::FindBugID(const CString& msg)
{
	CString sRet;
	if (!sCheckRe.IsEmpty() || (sMessage.Find(_T("%BUGID%")) >= 0))
    {
	    std::set<CString> bugIDs = FindBugIDs(msg);

	    for (std::set<CString>::iterator it = bugIDs.begin(); it != bugIDs.end(); ++it)
	    {
		    sRet += *it;
		    sRet += _T(" ");
	    }
	    sRet.Trim();
    }

	return sRet;
}

CString ProjectProperties::GetBugIDUrl(const CString& sBugID)
{
	CString ret;
	if (sUrl.IsEmpty())
		return ret;
	if (!sMessage.IsEmpty() || !sCheckRe.IsEmpty())
	{
		ret = sUrl;
		ret.Replace(_T("%BUGID%"), sBugID);
	}
	return ret;
}

BOOL ProjectProperties::CheckBugID(const CString& sID)
{
	if (bNumber)
	{
		// check if the revision actually _is_ a number
		// or a list of numbers separated by colons
		TCHAR c = 0;
		int len = sID.GetLength();
		for (int i=0; i<len; ++i)
		{
			c = sID.GetAt(i);
			if ((c < '0')&&(c != ',')&&(c != ' '))
			{
				return FALSE;
			}
			if (c > '9')
				return FALSE;
		}
	}
	return TRUE;
}

BOOL ProjectProperties::HasBugID(const CString& sMessage)
{
	if (!sCheckRe.IsEmpty())
	{
		try
		{
            AutoUpdateRegex();
			return tr1::regex_search((LPCTSTR)sMessage, regCheck);
		}
		catch (exception) {}
	}
	return FALSE;
}

void ProjectProperties::InsertAutoProps(svn_config_t *cfg)
{
	// every line is an autoprop
	CString sPropsData = sAutoProps;
	bool bEnableAutoProps = false;
	while (!sPropsData.IsEmpty())
	{
		int pos = sPropsData.Find('\n');
		CString sLine = pos >= 0 ? sPropsData.Left(pos) : sPropsData;
		sLine.Trim();
		if (!sLine.IsEmpty())
		{
			// format is '*.something = property=value;property=value;....'
			// find first '=' char
			int equalpos = sLine.Find('=');
			if ((equalpos >= 0)&&(sLine[0] != '#'))
			{
				CString key = sLine.Left(equalpos);
				CString value = sLine.Mid(equalpos);
				key.Trim(_T(" ="));
				value.Trim(_T(" ="));
				svn_config_set(cfg, SVN_CONFIG_SECTION_AUTO_PROPS, (LPCSTR)CUnicodeUtils::GetUTF8(key), (LPCSTR)CUnicodeUtils::GetUTF8(value));
				bEnableAutoProps = true;
			}
		}
		if (pos >= 0)
			sPropsData = sPropsData.Mid(pos).Trim();
		else
			sPropsData.Empty();
	}
	if (bEnableAutoProps)
		svn_config_set(cfg, SVN_CONFIG_SECTION_MISCELLANY, SVN_CONFIG_OPTION_ENABLE_AUTO_PROPS, "yes");
}

bool ProjectProperties::AddAutoProps(const CTSVNPath& path)
{
	if (!path.IsDirectory())
		return true;	// no error, but nothing to do

	bool bRet = true;

	char buf[1024] = {0};
	SVNProperties props(path, SVNRev::REV_WC, false);
	if (!sLabel.IsEmpty())
		bRet = props.Add(BUGTRAQPROPNAME_LABEL, WideToUTF8((LPCTSTR)sLabel)) && bRet;
	if (!sMessage.IsEmpty())
		bRet = props.Add(BUGTRAQPROPNAME_MESSAGE, WideToUTF8((LPCTSTR)sMessage)) && bRet;
	if (!bNumber)
		bRet = props.Add(BUGTRAQPROPNAME_NUMBER, "false") && bRet;
	if (!sCheckRe.IsEmpty())
		bRet = props.Add(BUGTRAQPROPNAME_LOGREGEX, WideToUTF8((LPCTSTR)(sCheckRe + _T("\n") + sBugIDRe))) && bRet;
	if (!sUrl.IsEmpty())
		bRet = props.Add(BUGTRAQPROPNAME_URL, WideToUTF8((LPCTSTR)sUrl)) && bRet;
	if (bWarnIfNoIssue)
		bRet = props.Add(BUGTRAQPROPNAME_WARNIFNOISSUE, "true") && bRet;
	if (!bAppend)
		bRet = props.Add(BUGTRAQPROPNAME_APPEND, "false") && bRet;
	if (!sProviderUuid.IsEmpty())
		bRet = props.Add(BUGTRAQPROPNAME_PROVIDERUUID, WideToUTF8((LPCTSTR)sProviderUuid)) && bRet;
	if (!sProviderParams.IsEmpty())
		bRet = props.Add(BUGTRAQPROPNAME_PROVIDERPARAMS, WideToUTF8((LPCTSTR)sProviderParams)) && bRet;
	if (nLogWidthMarker)
	{
		sprintf_s(buf, sizeof(buf), "%ld", nLogWidthMarker);
		bRet = props.Add(PROJECTPROPNAME_LOGWIDTHLINE, buf) && bRet;
	}
	if (!sLogTemplate.IsEmpty())
		bRet = props.Add(PROJECTPROPNAME_LOGTEMPLATE, WideToUTF8((LPCTSTR)sLogTemplate)) && bRet;
	if (nMinLogSize)
	{
		sprintf_s(buf, sizeof(buf), "%ld", nMinLogSize);
		bRet = props.Add(PROJECTPROPNAME_LOGMINSIZE, buf) && bRet;
	}
	if (nMinLockMsgSize)
	{
		sprintf_s(buf, sizeof(buf), "%ld", nMinLockMsgSize);
		bRet = props.Add(PROJECTPROPNAME_LOCKMSGMINSIZE, buf) && bRet;
	}
	if (!bFileListInEnglish)
		bRet = props.Add(PROJECTPROPNAME_LOGFILELISTLANG, "false") && bRet;
	if (!sLogSummaryRe.IsEmpty())
		bRet = props.Add(PROJECTPROPNAME_LOGSUMMARY, WideToUTF8((LPCTSTR)sLogSummaryRe)) && bRet;
	if (!sLogRevRegex.IsEmpty())
		bRet = props.Add(PROJECTPROPNAME_LOGREVREGEX, WideToUTF8((LPCTSTR)sLogRevRegex)) && bRet;
	if (lProjectLanguage)
	{
		sprintf_s(buf, sizeof(buf), "%ld", lProjectLanguage);
		bRet = props.Add(PROJECTPROPNAME_PROJECTLANGUAGE, buf) && bRet;
	}
	if (!sFPPath.IsEmpty())
		bRet = props.Add(PROJECTPROPNAME_USERFILEPROPERTY, WideToUTF8((LPCTSTR)sFPPath)) && bRet;
	if (!sDPPath.IsEmpty())
		bRet = props.Add(PROJECTPROPNAME_USERDIRPROPERTY, WideToUTF8((LPCTSTR)sDPPath)) && bRet;
	if (!sWebViewerRev.IsEmpty())
		bRet = props.Add(PROJECTPROPNAME_WEBVIEWER_REV, WideToUTF8((LPCTSTR)sWebViewerRev)) && bRet;
	if (!sWebViewerPathRev.IsEmpty())
		bRet = props.Add(PROJECTPROPNAME_WEBVIEWER_PATHREV, WideToUTF8((LPCTSTR)sWebViewerPathRev)) && bRet;
	if (!sAutoProps.IsEmpty())
		bRet = props.Add(PROJECTPROPNAME_AUTOPROPS, WideToUTF8((LPCTSTR)sAutoProps)) && bRet;
	return bRet;
}

CString ProjectProperties::GetLogSummary(const CString& sMessage)
{
	CString sRet;

	if (!sLogSummaryRe.IsEmpty())
	{
		try
		{
			const tr1::wregex regSum(sLogSummaryRe);
			const tr1::wsregex_iterator end;
			wstring s = sMessage;
			for (tr1::wsregex_iterator it(s.begin(), s.end(), regSum); it != end; ++it)
			{
				const tr1::wsmatch match = *it;
				// we define the first group as the summary text
				if ((*it).size() >= 1)
				{
					ATLTRACE(_T("matched summary : %s\n"), wstring(match[0]).c_str());
					sRet += (CString(wstring(match[1]).c_str()));
				}
			}
		}
		catch (exception) {}
	}
	sRet.Trim();

	return sRet;
}

CString ProjectProperties::MakeShortMessage(const CString& message)
{
	bool bFoundShort = true;
	CString sShortMessage = GetLogSummary(message);
	if (sShortMessage.IsEmpty())
	{
		bFoundShort = false;
		sShortMessage = message;
	}
	// Remove newlines and tabs 'cause those are not shown nicely in the list control
	sShortMessage.Replace(_T("\r"), _T(""));
	sShortMessage.Replace(_T("\t"), _T(" "));
	
	// Suppose the first empty line separates 'summary' from the rest of the message.
	int found = sShortMessage.Find(_T("\n\n"));
	// To avoid too short 'short' messages 
	// (e.g. if the message looks something like "Bugfix:\n\n*done this\n*done that")
	// only use the empty newline as a separator if it comes after at least 15 chars.
	if ((!bFoundShort)&&(found >= 15))
	{
		sShortMessage = sShortMessage.Left(found);
	}
	sShortMessage.Replace('\n', ' ');
	return sShortMessage;
}

void ProjectProperties::SetLinkCharFormat(CWnd* window)
{
	CHARFORMAT2 format;
	SecureZeroMemory(&format, sizeof(CHARFORMAT2));
	format.cbSize = sizeof(CHARFORMAT2);
	format.dwMask = CFM_LINK;
	format.dwEffects = CFE_LINK;
	window->SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&format);
}

#ifdef DEBUG
static class PropTest
{
public:
	PropTest()
	{
		CString msg = _T("this is a test logmessage: issue 222\nIssue #456, #678, 901  #456");
		CString sUrl = _T("http://tortoisesvn.tigris.org/issues/show_bug.cgi?id=%BUGID%");
		CString sCheckRe = _T("[Ii]ssue #?(\\d+)(,? ?#?(\\d+))+");
		CString sBugIDRe = _T("(\\d+)");
		ProjectProperties props;
		props.sCheckRe = _T("PAF-[0-9]+");
		props.sUrl = _T("http://tortoisesvn.tigris.org/issues/show_bug.cgi?id=%BUGID%");
		CString sRet = props.FindBugID(_T("This is a test for PAF-88"));
		ATLASSERT(sRet.IsEmpty());
		props.sCheckRe = _T("[Ii]ssue #?(\\d+)");
        props.regExNeedUpdate = true;
		sRet = props.FindBugID(_T("Testing issue #99"));
		sRet.Trim();
		ATLASSERT(sRet.Compare(_T("99"))==0);
		props.sCheckRe = _T("[Ii]ssues?:?(\\s*(,|and)?\\s*#\\d+)+");
		props.sBugIDRe = _T("(\\d+)");
		props.sUrl = _T("http://tortoisesvn.tigris.org/issues/show_bug.cgi?id=%BUGID%");
        props.regExNeedUpdate = true;
		sRet = props.FindBugID(_T("This is a test for Issue #7463,#666"));
		ATLASSERT(props.HasBugID(_T("This is a test for Issue #7463,#666")));
		ATLASSERT(!props.HasBugID(_T("This is a test for Issue 7463,666")));
		sRet.Trim();
		ATLASSERT(sRet.Compare(_T("666 7463"))==0);
		props.sCheckRe = _T("^\\[(\\d+)\\].*");
		props.sUrl = _T("http://tortoisesvn.tigris.org/issues/show_bug.cgi?id=%BUGID%");
        props.regExNeedUpdate = true;
		sRet = props.FindBugID(_T("[000815] some stupid programming error fixed"));
		sRet.Trim();
		ATLASSERT(sRet.Compare(_T("000815"))==0);
		props.sCheckRe = _T("\\[\\[(\\d+)\\]\\]\\]");
		props.sUrl = _T("http://tortoisesvn.tigris.org/issues/show_bug.cgi?id=%BUGID%");
        props.regExNeedUpdate = true;
		sRet = props.FindBugID(_T("test test [[000815]]] some stupid programming error fixed"));
		sRet.Trim();
		ATLASSERT(sRet.Compare(_T("000815"))==0);
		ATLASSERT(props.HasBugID(_T("test test [[000815]]] some stupid programming error fixed")));
		ATLASSERT(!props.HasBugID(_T("test test [000815]] some stupid programming error fixed")));
		props.sLogSummaryRe = _T("\\[SUMMARY\\]:(.*)");
		ATLASSERT(props.GetLogSummary(_T("[SUMMARY]: this is my summary")).Compare(_T("this is my summary"))==0);
	}
} PropTest;
#endif

