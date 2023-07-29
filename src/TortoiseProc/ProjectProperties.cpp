﻿// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2018, 2020-2021 - TortoiseSVN

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
#include "stdafx.h"
#include "UnicodeUtils.h"
#include "ProjectProperties.h"
#include "SVNProperties.h"
#include "SVN.h"
#include "SVNHelpers.h"
#include "TSVNPath.h"
#include "AppUtils.h"
#include <regex>
#include <Shlwapi.h>

#define LOG_REVISIONREGEX L"\\b(r\\d+)|\\b(revisions?(\\(s\\))?\\s#?\\d+([, ]+(and\\s?)?\\d+)*)|\\b(revs?\\.?\\s?\\d+([, ]+(and\\s?)?\\d+)*)"

const CString sLog_revisionregex = LOG_REVISIONREGEX;

struct NumCompare
{
    bool operator()(const CString& lhs, const CString& rhs) const
    {
        return StrCmpLogicalW(lhs, rhs) < 0;
    }
};

ProjectProperties::ProjectProperties()
    : bNumber(TRUE)
    , bWarnIfNoIssue(FALSE)
    , bAppend(TRUE)
    , nLogWidthMarker(0)
    , nMinLogSize(0)
    , nMinLockMsgSize(0)
    , bFileListInEnglish(TRUE)
    , lProjectLanguage(0)
    , bMergeLogTemplateMsgTitleBottom(FALSE)
    , regExNeedUpdate(true)
    , nBugIdPos(-1)
    , m_bFound(false)
    , m_bPropsRead(false)
{
}

ProjectProperties::~ProjectProperties()
{
}

BOOL ProjectProperties::ReadPropsPathList(const CTSVNPathList& pathList)
{
    for (int nPath = 0; nPath < pathList.GetCount(); nPath++)
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
    m_bPropsRead    = true;

    if (!path.IsUrl())
    {
        if (!path.IsDirectory())
            path = path.GetContainingDirectory();

        while (!SVNHelper::IsVersioned(path, false) && !path.IsEmpty())
            path = path.GetContainingDirectory();
    }

    SVN           svn;
    SVNProperties props(path, path.IsUrl() ? SVNRev::REV_HEAD : SVNRev::REV_WC, false, true);
    for (int i = 0; i < props.GetCount(); ++i)
    {
        std::string sPropName = props.GetItemName(i);
        CString     sPropVal  = CUnicodeUtils::GetUnicode(const_cast<char*>(props.GetItemValue(i).c_str()));
        if (CheckStringProp(sMessage, sPropName, sPropVal, BUGTRAQPROPNAME_MESSAGE))
            nBugIdPos = sMessage.Find(L"%BUGID%");
        if (sPropName.compare(BUGTRAQPROPNAME_NUMBER) == 0)
        {
            CString val;
            val = sPropVal;
            val = val.Trim(L" \n\r\t");
            if ((val.CompareNoCase(L"false") == 0) || (val.CompareNoCase(L"no") == 0))
                bNumber = FALSE;
            else
                bNumber = TRUE;
        }
        if (CheckStringProp(sCheckRe, sPropName, sPropVal, BUGTRAQPROPNAME_LOGREGEX))
        {
            if (sCheckRe.Find('\n') >= 0)
            {
                sBugIDRe = sCheckRe.Mid(sCheckRe.Find('\n')).Trim();
                sCheckRe = sCheckRe.Left(sCheckRe.Find('\n')).Trim();
            }
            if (!sCheckRe.IsEmpty())
            {
                sCheckRe = sCheckRe.Trim();
            }
        }
        CheckStringProp(sLabel, sPropName, sPropVal, BUGTRAQPROPNAME_LABEL);
        CheckStringProp(sUrl, sPropName, sPropVal, BUGTRAQPROPNAME_URL);
        if (sPropName.compare(BUGTRAQPROPNAME_WARNIFNOISSUE) == 0)
        {
            CString val;
            val = sPropVal;
            val = val.Trim(L" \n\r\t");
            if ((val.CompareNoCase(L"true") == 0) || (val.CompareNoCase(L"yes") == 0))
                bWarnIfNoIssue = TRUE;
            else
                bWarnIfNoIssue = FALSE;
        }
        if (sPropName.compare(BUGTRAQPROPNAME_APPEND) == 0)
        {
            CString val;
            val = sPropVal;
            val = val.Trim(L" \n\r\t");
            if ((val.CompareNoCase(L"true") == 0) || (val.CompareNoCase(L"yes") == 0))
                bAppend = TRUE;
            else
                bAppend = FALSE;
        }
        CheckStringProp(sProviderUuid, sPropName, sPropVal, BUGTRAQPROPNAME_PROVIDERUUID);
        CheckStringProp(sProviderUuid64, sPropName, sPropVal, BUGTRAQPROPNAME_PROVIDERUUID64);
        CheckStringProp(sProviderParams, sPropName, sPropVal, BUGTRAQPROPNAME_PROVIDERPARAMS);
        if (sPropName.compare(PROJECTPROPNAME_LOGWIDTHLINE) == 0)
        {
            CString val;
            val = sPropVal;
            if (!val.IsEmpty())
            {
                nLogWidthMarker = _wtoi(val);
            }
        }
        CheckStringProp(sLogTemplate, sPropName, sPropVal, PROJECTPROPNAME_LOGTEMPLATE);
        CheckStringProp(sLogTemplateCommit, sPropName, sPropVal, PROJECTPROPNAME_LOGTEMPLATECOMMIT);
        CheckStringProp(sLogTemplateBranch, sPropName, sPropVal, PROJECTPROPNAME_LOGTEMPLATEBRANCH);
        CheckStringProp(sLogTemplateImport, sPropName, sPropVal, PROJECTPROPNAME_LOGTEMPLATEIMPORT);
        CheckStringProp(sLogTemplateDelete, sPropName, sPropVal, PROJECTPROPNAME_LOGTEMPLATEDEL);
        CheckStringProp(sLogTemplateMove, sPropName, sPropVal, PROJECTPROPNAME_LOGTEMPLATEMOVE);
        CheckStringProp(sLogTemplateMkDir, sPropName, sPropVal, PROJECTPROPNAME_LOGTEMPLATEMKDIR);
        CheckStringProp(sLogTemplatePropset, sPropName, sPropVal, PROJECTPROPNAME_LOGTEMPLATEPROPSET);
        CheckStringProp(sLogTemplateLock, sPropName, sPropVal, PROJECTPROPNAME_LOGTEMPLATELOCK);
        if (sPropName.compare(PROJECTPROPNAME_LOGMINSIZE) == 0)
        {
            CString val;
            val = sPropVal;
            if (!val.IsEmpty())
            {
                nMinLogSize = _wtoi(val);
            }
        }
        if (sPropName.compare(PROJECTPROPNAME_LOCKMSGMINSIZE) == 0)
        {
            CString val;
            val = sPropVal;
            if (!val.IsEmpty())
            {
                nMinLockMsgSize = _wtoi(val);
            }
        }
        if (sPropName.compare(PROJECTPROPNAME_LOGFILELISTLANG) == 0)
        {
            CString val;
            val = sPropVal;
            val = val.Trim(L" \n\r\t");
            if ((val.CompareNoCase(L"false") == 0) || (val.CompareNoCase(L"no") == 0))
                bFileListInEnglish = FALSE;
            else
                bFileListInEnglish = TRUE;
        }
        if (sPropName.compare(PROJECTPROPNAME_PROJECTLANGUAGE) == 0)
        {
            CString val;
            val = sPropVal;
            if (!val.IsEmpty())
            {
                LPWSTR strEnd;
                lProjectLanguage = wcstol(val, &strEnd, 0);
            }
        }
        CheckStringProp(m_sFpPath, sPropName, sPropVal, PROJECTPROPNAME_USERFILEPROPERTY);
        CheckStringProp(m_sDpPath, sPropName, sPropVal, PROJECTPROPNAME_USERDIRPROPERTY);
        CheckStringProp(sAutoProps, sPropName, sPropVal, PROJECTPROPNAME_AUTOPROPS);
        CheckStringProp(sWebViewerRev, sPropName, sPropVal, PROJECTPROPNAME_WEBVIEWER_REV);
        CheckStringProp(sWebViewerPathRev, sPropName, sPropVal, PROJECTPROPNAME_WEBVIEWER_PATHREV);
        CheckStringProp(sLogSummaryRe, sPropName, sPropVal, PROJECTPROPNAME_LOGSUMMARY);
        CheckStringProp(sLogRevRegex, sPropName, sPropVal, PROJECTPROPNAME_LOGREVREGEX);
        CheckStringProp(sStartCommitHook, sPropName, sPropVal, PROJECTPROPNAME_STARTCOMMITHOOK);
        CheckStringProp(sCheckCommitHook, sPropName, sPropVal, PROJECTPROPNAME_CHECKCOMMITHOOK);
        CheckStringProp(sPreCommitHook, sPropName, sPropVal, PROJECTPROPNAME_PRECOMMITHOOK);
        CheckStringProp(sManualPreCommitHook, sPropName, sPropVal, PROJECTPROPNAME_MANUALPRECOMMITHOOK);
        CheckStringProp(sPostCommitHook, sPropName, sPropVal, PROJECTPROPNAME_POSTCOMMITHOOK);
        CheckStringProp(sStartUpdateHook, sPropName, sPropVal, PROJECTPROPNAME_STARTUPDATEHOOK);
        CheckStringProp(sPreUpdateHook, sPropName, sPropVal, PROJECTPROPNAME_PREUPDATEHOOK);
        CheckStringProp(sPostUpdateHook, sPropName, sPropVal, PROJECTPROPNAME_POSTUPDATEHOOK);
        CheckStringProp(sPreLockHook, sPropName, sPropVal, PROJECTPROPNAME_PRELOCKHOOK);
        CheckStringProp(sPostLockHook, sPropName, sPropVal, PROJECTPROPNAME_POSTLOCKHOOK);

        CheckStringProp(sMergeLogTemplateTitle, sPropName, sPropVal, PROJECTPROPNAME_MERGELOGTEMPLATETITLE);
        CheckStringProp(sMergeLogTemplateReverseTitle, sPropName, sPropVal, PROJECTPROPNAME_MERGELOGTEMPLATEREVERSETITLE);
        CheckStringProp(sMergeLogTemplateMsg, sPropName, sPropVal, PROJECTPROPNAME_MERGELOGTEMPLATEMSG);
        if (sPropName.compare(PROJECTPROPNAME_MERGELOGTEMPLATETITLEBOTTOM) == 0)
        {
            CString val;
            val = sPropVal;
            val = val.Trim(L" \n\r\t");
            if ((val.CompareNoCase(L"true") == 0) || (val.CompareNoCase(L"yes") == 0))
                bMergeLogTemplateMsgTitleBottom = TRUE;
            else
                bMergeLogTemplateMsgTitleBottom = FALSE;
        }
    }

    propsPath = path;
    if (sLogRevRegex.IsEmpty())
        sLogRevRegex = LOG_REVISIONREGEX;
    if (m_bFound)
    {
        sRepositoryRootUrl = svn.GetRepositoryRoot(propsPath);
        sRepositoryPathUrl = svn.GetURLFromPath(propsPath);

        return TRUE;
    }
    propsPath.Reset();
    return FALSE;
}

bool ProjectProperties::CheckStringProp(CString& s, const std::string& propname, const CString& propval, LPCSTR prop)
{
    if (propname.compare(prop) == 0)
    {
        if (s.IsEmpty())
        {
            s = propval;
            s.Remove('\r');
            s.Replace(L"\r\n", L"\n");
            m_bFound = true;
            return true;
        }
    }
    return false;
}

CString ProjectProperties::GetBugIDFromLog(CString& msg) const
{
    CString sBugID;

    if (!sMessage.IsEmpty())
    {
        CString sBugLine;
        CString sFirstPart;
        CString sLastPart;
        BOOL    bTop = FALSE;
        if (nBugIdPos < 0)
            return sBugID;
        sFirstPart = sMessage.Left(nBugIdPos);
        sLastPart  = sMessage.Mid(nBugIdPos + 7);
        msg.TrimRight('\n');
        if (msg.ReverseFind('\n') >= 0)
        {
            if (bAppend)
                sBugLine = msg.Mid(msg.ReverseFind('\n') + 1);
            else
            {
                sBugLine = msg.Left(msg.Find('\n'));
                bTop     = TRUE;
            }
        }
        else
        {
            if (bNumber)
            {
                // find out if the message consists only of numbers
                bool bOnlyNumbers = true;
                for (int i = 0; i < msg.GetLength(); ++i)
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
            sBugLine = msg.Mid(msg.ReverseFind('\n') + 1);
        }
        if (sBugLine.Left(sFirstPart.GetLength()).Compare(sFirstPart) != 0)
            sBugLine.Empty();
        if (sBugLine.Right(sLastPart.GetLength()).Compare(sLastPart) != 0)
            sBugLine.Empty();
        if (sBugLine.IsEmpty())
        {
            if (msg.Find('\n') >= 0)
                sBugLine = msg.Left(msg.Find('\n'));
            if (sBugLine.Left(sFirstPart.GetLength()).Compare(sFirstPart) != 0)
                sBugLine.Empty();
            if (sBugLine.Right(sLastPart.GetLength()).Compare(sLastPart) != 0)
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
            msg = msg.Left(msg.GetLength() - sBugLine.GetLength());
            msg.TrimRight('\n');
        }
    }
    return sBugID;
}

void ProjectProperties::AutoUpdateRegex()
{
    if (regExNeedUpdate)
    {
        try
        {
            regCheck = std::wregex(sCheckRe);
            regBugID = std::wregex(sBugIDRe);
        }
        catch (std::exception&)
        {
        }

        regExNeedUpdate = false;
    }
}

std::vector<CHARRANGE> ProjectProperties::FindBugIDPositions(const CString& msg)
{
    std::vector<CHARRANGE> result;

    // first use the checkre string to find bug ID's in the message
    if (!sCheckRe.IsEmpty())
    {
        if (!sBugIDRe.IsEmpty())
        {
            // match with two regex strings (without grouping!)
            try
            {
                AutoUpdateRegex();
                const std::wsregex_iterator end;
                std::wstring                s = static_cast<LPCWSTR>(msg);
                for (std::wsregex_iterator it(s.begin(), s.end(), regCheck); it != end; ++it)
                {
                    // (*it)[0] is the matched string
                    std::wstring matchedString = (*it)[0];
                    ptrdiff_t    matchpos      = it->position(0);
                    for (std::wsregex_iterator it2(matchedString.begin(), matchedString.end(), regBugID); it2 != end; ++it2)
                    {
                        ATLTRACE(L"matched id : %s\n", (*it2)[0].str().c_str());
                        ptrdiff_t matchposID = it2->position(0);
                        CHARRANGE range      = {static_cast<LONG>(matchpos + matchposID), static_cast<LONG>(matchpos + matchposID + (*it2)[0].str().size())};
                        result.push_back(range);
                    }
                }
            }
            catch (std::exception&)
            {
            }
        }
        else
        {
            try
            {
                AutoUpdateRegex();
                const std::wsregex_iterator end;
                std::wstring                s = static_cast<LPCWSTR>(msg);
                for (std::wsregex_iterator it(s.begin(), s.end(), regCheck); it != end; ++it)
                {
                    const std::wsmatch match = *it;
                    // we define group 1 as the whole issue text and
                    // group 2 as the bug ID
                    if (match.size() >= 2)
                    {
                        ATLTRACE(L"matched id : %s\n", std::wstring(match[1]).c_str());
                        CHARRANGE range = {static_cast<LONG>(match[1].first - s.begin()), static_cast<LONG>(match[1].second - s.begin())};
                        result.push_back(range);
                    }
                }
            }
            catch (std::exception&)
            {
            }
        }
    }
    else if (result.empty() && (!sMessage.IsEmpty()))
    {
        CString sBugLine;
        CString sFirstPart;
        CString sLastPart;
        BOOL    bTop = FALSE;
        if (nBugIdPos < 0)
            return result;

        sFirstPart   = sMessage.Left(nBugIdPos);
        sLastPart    = sMessage.Mid(nBugIdPos + 7);
        CString sMsg = msg;
        sMsg.TrimRight('\n');
        if (sMsg.ReverseFind('\n') >= 0)
        {
            if (bAppend)
                sBugLine = sMsg.Mid(sMsg.ReverseFind('\n') + 1);
            else
            {
                sBugLine = sMsg.Left(sMsg.Find('\n'));
                bTop     = TRUE;
            }
        }
        else
            sBugLine = sMsg;
        if (sBugLine.Left(sFirstPart.GetLength()).Compare(sFirstPart) != 0)
            sBugLine.Empty();
        if (sBugLine.Right(sLastPart.GetLength()).Compare(sLastPart) != 0)
            sBugLine.Empty();
        if (sBugLine.IsEmpty())
        {
            if (sMsg.Find('\n') >= 0)
                sBugLine = sMsg.Left(sMsg.Find('\n'));
            if (sBugLine.Left(sFirstPart.GetLength()).Compare(sFirstPart) != 0)
                sBugLine.Empty();
            if (sBugLine.Right(sLastPart.GetLength()).Compare(sLastPart) != 0)
                sBugLine.Empty();
            bTop = TRUE;
        }
        if (sBugLine.IsEmpty())
            return result;

        CString sBugIDPart = sBugLine.Mid(sFirstPart.GetLength(), sBugLine.GetLength() - sFirstPart.GetLength() - sLastPart.GetLength());
        if (sBugIDPart.IsEmpty())
            return result;

        //the bug id part can contain several bug id's, separated by commas
        size_t offset1 = 0;
        size_t offset2 = 0;
        if (!bTop)
            offset1 = sMsg.GetLength() - sBugLine.GetLength() + sFirstPart.GetLength();
        else
            offset1 = sFirstPart.GetLength();
        sBugIDPart.Trim(L",");
        while (sBugIDPart.Find(',') >= 0)
        {
            offset2         = offset1 + sBugIDPart.Find(',');
            CHARRANGE range = {static_cast<LONG>(offset1), static_cast<LONG>(offset2)};
            result.push_back(range);
            sBugIDPart = sBugIDPart.Mid(sBugIDPart.Find(',') + 1);
            offset1    = offset2 + 1;
        }
        offset2         = offset1 + sBugIDPart.GetLength();
        CHARRANGE range = {static_cast<LONG>(offset1), static_cast<LONG>(offset2)};
        result.push_back(range);
    }

    return result;
}

BOOL ProjectProperties::FindBugID(const CString& msg, CWnd* pWnd)
{
    std::vector<CHARRANGE> positions = FindBugIDPositions(msg);
    CAppUtils::SetCharFormat(pWnd, CFM_LINK, CFE_LINK, positions);

    return positions.empty() ? FALSE : TRUE;
}

std::set<CString> ProjectProperties::FindBugIDs(const CString& msg)
{
    std::vector<CHARRANGE> positions = FindBugIDPositions(msg);
    std::set<CString>      bugIDs;

    for (auto iter = positions.begin(), end = positions.end(); iter != end; ++iter)
    {
        bugIDs.insert(msg.Mid(iter->cpMin, iter->cpMax - iter->cpMin));
    }

    return bugIDs;
}

CString ProjectProperties::FindBugID(const CString& msg)
{
    CString sRet;
    if (!sCheckRe.IsEmpty() || (nBugIdPos >= 0))
    {
        std::vector<CHARRANGE>        positions = FindBugIDPositions(msg);
        std::set<CString, NumCompare> bugIDs;
        for (auto iter = positions.begin(), end = positions.end(); iter != end; ++iter)
        {
            bugIDs.insert(msg.Mid(iter->cpMin, iter->cpMax - iter->cpMin));
        }

        for (std::set<CString, NumCompare>::iterator it = bugIDs.begin(); it != bugIDs.end(); ++it)
        {
            sRet += *it;
            sRet += L" ";
        }
        sRet.Trim();
    }

    return sRet;
}

bool ProjectProperties::MightContainABugID() const
{
    return !sCheckRe.IsEmpty() || (nBugIdPos >= 0);
}

CString ProjectProperties::GetBugIDUrl(const CString& sBugID) const
{
    CString ret = sUrl;
    if (sUrl.IsEmpty())
        return ret;
    if (!sMessage.IsEmpty() || !sCheckRe.IsEmpty())
        ReplaceBugIDPlaceholder(ret, sBugID);
    return ret;
}

void ProjectProperties::ReplaceBugIDPlaceholder(CString& url, const CString& sBugID)
{
    CString parameter;
    DWORD   size = INTERNET_MAX_URL_LENGTH;
    UrlEscape(sBugID, CStrBuf(parameter, size + 1), &size, URL_ESCAPE_SEGMENT_ONLY | URL_ESCAPE_PERCENT | URL_ESCAPE_AS_UTF8);
    // UrlEscape does not escape + and =, starting with Win8 the URL_ESCAPE_ASCII_URI_COMPONENT flag could be used and the following two lines would not be necessary
    parameter.Replace(L"!", L"%21");
    parameter.Replace(L"$", L"%24");
    parameter.Replace(L"'", L"%27");
    parameter.Replace(L"(", L"%28");
    parameter.Replace(L")", L"%29");
    parameter.Replace(L"*", L"%2A");
    parameter.Replace(L"+", L"%2B");
    parameter.Replace(L",", L"%2C");
    parameter.Replace(L":", L"%3A");
    parameter.Replace(L";", L"%3B");
    parameter.Replace(L"=", L"%3D");
    parameter.Replace(L"@", L"%40");
    url.Replace(L"%BUGID%", parameter);
}

BOOL ProjectProperties::CheckBugID(const CString& sID) const
{
    if (bNumber)
    {
        // check if the revision actually _is_ a number
        // or a list of numbers separated by colons
        int len = sID.GetLength();
        for (int i = 0; i < len; ++i)
        {
            wchar_t c = sID.GetAt(i);
            if ((c < '0') && (c != ',') && (c != ' '))
            {
                return FALSE;
            }
            if (c > '9')
                return FALSE;
        }
    }
    return TRUE;
}

BOOL ProjectProperties::HasBugID(const CString& sMsg)
{
    if (!sCheckRe.IsEmpty())
    {
        try
        {
            AutoUpdateRegex();
            return std::regex_search(static_cast<LPCWSTR>(sMsg), regCheck);
        }
        catch (std::exception&)
        {
        }
    }
    return FALSE;
}

void ProjectProperties::InsertAutoProps(svn_config_t* cfg) const
{
    // every line is an autoprop
    CString sPropsData       = sAutoProps;
    bool    bEnableAutoProps = false;
    while (!sPropsData.IsEmpty())
    {
        int     pos   = sPropsData.Find('\n');
        CString sLine = pos >= 0 ? sPropsData.Left(pos) : sPropsData;
        sLine.Trim();
        if (!sLine.IsEmpty())
        {
            // format is '*.something = property=value;property=value;....'
            // find first '=' char
            int equalpos = sLine.Find('=');
            if ((equalpos >= 0) && (sLine[0] != '#'))
            {
                CString key   = sLine.Left(equalpos);
                CString value = sLine.Mid(equalpos);
                key.Trim(L" =");
                value.Trim(L" =");
                svn_config_set(cfg, SVN_CONFIG_SECTION_AUTO_PROPS, static_cast<LPCSTR>(CUnicodeUtils::GetUTF8(key)), static_cast<LPCSTR>(CUnicodeUtils::GetUTF8(value)));
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

CString ProjectProperties::GetLogSummary(const CString& sMsg) const
{
    CString sRet;

    if (!sLogSummaryRe.IsEmpty())
    {
        try
        {
            const std::wregex           regSum(sLogSummaryRe);
            const std::wsregex_iterator end;
            std::wstring                s = static_cast<LPCWSTR>(sMsg);
            for (std::wsregex_iterator it(s.begin(), s.end(), regSum); it != end; ++it)
            {
                const std::wsmatch match = *it;
                // we define the first group as the summary text
                if ((*it).size() >= 1)
                {
                    ATLTRACE(L"matched summary : %s\n", std::wstring(match[0]).c_str());
                    sRet += (CString(std::wstring(match[1]).c_str()));
                }
            }
        }
        catch (std::exception&)
        {
        }
    }
    sRet.Trim();

    return sRet;
}

CString ProjectProperties::MakeShortMessage(const CString& message) const
{
    enum
    {
        MAX_SHORT_MESSAGE_LENGTH = 240
    };

    bool    bFoundShort   = true;
    CString sShortMessage = GetLogSummary(message);
    if (sShortMessage.IsEmpty())
    {
        bFoundShort   = false;
        sShortMessage = message;
    }
    // Remove newlines and tabs 'cause those are not shown nicely in the list control
    sShortMessage.Remove('\r');
    sShortMessage.Replace('\t', ' ');

    if (!bFoundShort)
    {
        // Suppose the first empty line separates 'summary' from the rest of the message.
        int found = sShortMessage.Find(L"\n\n");
        // To avoid too short 'short' messages
        // (e.g. if the message looks something like "Bugfix:\n\n*done this\n*done that")
        // only use the empty newline as a separator if it comes after at least 15 chars.
        if (found >= 15)
            sShortMessage = sShortMessage.Left(found);

        // still too long? -> truncate after about 2 lines
        if (sShortMessage.GetLength() > MAX_SHORT_MESSAGE_LENGTH)
            sShortMessage = sShortMessage.Left(MAX_SHORT_MESSAGE_LENGTH) + L"...";
    }

    sShortMessage.Replace('\n', ' ');
    return sShortMessage;
}

const CString& ProjectProperties::GetLogMsgTemplate(const CStringA& prop) const
{
    if (prop.Compare(PROJECTPROPNAME_LOGTEMPLATECOMMIT) == 0)
        return sLogTemplateCommit.IsEmpty() ? sLogTemplate : sLogTemplateCommit;
    if (prop.Compare(PROJECTPROPNAME_LOGTEMPLATEBRANCH) == 0)
        return sLogTemplateBranch.IsEmpty() ? sLogTemplate : sLogTemplateBranch;
    if (prop.Compare(PROJECTPROPNAME_LOGTEMPLATEIMPORT) == 0)
        return sLogTemplateImport.IsEmpty() ? sLogTemplate : sLogTemplateImport;
    if (prop.Compare(PROJECTPROPNAME_LOGTEMPLATEDEL) == 0)
        return sLogTemplateDelete.IsEmpty() ? sLogTemplate : sLogTemplateDelete;
    if (prop.Compare(PROJECTPROPNAME_LOGTEMPLATEMOVE) == 0)
        return sLogTemplateMove.IsEmpty() ? sLogTemplate : sLogTemplateMove;
    if (prop.Compare(PROJECTPROPNAME_LOGTEMPLATEMKDIR) == 0)
        return sLogTemplateMkDir.IsEmpty() ? sLogTemplate : sLogTemplateMkDir;
    if (prop.Compare(PROJECTPROPNAME_LOGTEMPLATEPROPSET) == 0)
        return sLogTemplatePropset.IsEmpty() ? sLogTemplate : sLogTemplatePropset;
    if (prop.Compare(PROJECTPROPNAME_LOGTEMPLATELOCK) == 0)
        return sLogTemplateLock; // we didn't use sLogTemplate before for lock messages, so we don't do that now either

    return sLogTemplate;
}

const CString& ProjectProperties::GetLogRevRegex() const
{
    if (!sLogRevRegex.IsEmpty())
        return sLogRevRegex;
    return sLog_revisionregex;
}

void ProjectProperties::SaveToIni(CSimpleIni& inifile, const CString& section, const CString& prefix /*= L"pp"*/) const
{
    assert(inifile.IsMultiLine());
    inifile.SetValue(section, prefix + L"sLabel", sLabel);
    inifile.SetValue(section, prefix + L"sMessage", sMessage);
    inifile.SetValue(section, prefix + L"bNumber", bNumber ? L"1" : L"0");
    inifile.SetValue(section, prefix + L"sUrl", sUrl);
    inifile.SetValue(section, prefix + L"bWarnIfNoIssue", bWarnIfNoIssue ? L"1" : L"0");
    inifile.SetValue(section, prefix + L"bAppend", bAppend ? L"1" : L"0");
    inifile.SetValue(section, prefix + L"sProviderParams", sProviderParams);
    inifile.SetValue(section, prefix + L"nLogWidthMarker", std::to_wstring(nLogWidthMarker).c_str());
    inifile.SetValue(section, prefix + L"nMinLogSize", std::to_wstring(nMinLogSize).c_str());
    inifile.SetValue(section, prefix + L"nMinLockMsgSize", std::to_wstring(nMinLockMsgSize).c_str());
    inifile.SetValue(section, prefix + L"bFileListInEnglish", bFileListInEnglish ? L"1" : L"0");
    inifile.SetValue(section, prefix + L"lProjectLanguage", std::to_wstring(lProjectLanguage).c_str());
    inifile.SetValue(section, prefix + L"sFPPath", m_sFpPath);
    inifile.SetValue(section, prefix + L"sDPPath", m_sDpPath);
    inifile.SetValue(section, prefix + L"sWebViewerRev", sWebViewerRev);
    inifile.SetValue(section, prefix + L"sWebViewerPathRev", sWebViewerPathRev);
    inifile.SetValue(section, prefix + L"sLogSummaryRe", sLogSummaryRe);
    inifile.SetValue(section, prefix + L"sStartCommitHook", sStartCommitHook);
    inifile.SetValue(section, prefix + L"sCheckCommitHook", sCheckCommitHook);
    inifile.SetValue(section, prefix + L"sPreCommitHook", sPreCommitHook);
    inifile.SetValue(section, prefix + L"sManualPreCommitHook", sManualPreCommitHook);
    inifile.SetValue(section, prefix + L"sPostCommitHook", sPostCommitHook);
    inifile.SetValue(section, prefix + L"sStartUpdateHook", sStartUpdateHook);
    inifile.SetValue(section, prefix + L"sPreUpdateHook", sPreUpdateHook);
    inifile.SetValue(section, prefix + L"sPostUpdateHook", sPostUpdateHook);
    inifile.SetValue(section, prefix + L"sPreConnectHook", sPreConnectHook);
    inifile.SetValue(section, prefix + L"sPreLockHook", sPreLockHook);
    inifile.SetValue(section, prefix + L"sPostLockHook", sPostLockHook);
    inifile.SetValue(section, prefix + L"sRepositoryRootUrl", sRepositoryRootUrl);
    inifile.SetValue(section, prefix + L"sRepositoryPathUrl", sRepositoryPathUrl);
    inifile.SetValue(section, prefix + L"sMergeLogTemplateTitle", sMergeLogTemplateTitle);
    inifile.SetValue(section, prefix + L"sMergeLogTemplateReverseTitle", sMergeLogTemplateReverseTitle);
    inifile.SetValue(section, prefix + L"sMergeLogTemplateMsg", sMergeLogTemplateMsg);
    inifile.SetValue(section, prefix + L"bMergeLogTemplateMsgTitleBottom", bMergeLogTemplateMsgTitleBottom ? L"1" : L"0");
    inifile.SetValue(section, prefix + L"sAutoProps", sAutoProps);
    inifile.SetValue(section, prefix + L"sProviderUuid", sProviderUuid);
    inifile.SetValue(section, prefix + L"sProviderUuid64", sProviderUuid64);
    inifile.SetValue(section, prefix + L"sCheckRe", sCheckRe);
    inifile.SetValue(section, prefix + L"sBugIDRe", sBugIDRe);
    inifile.SetValue(section, prefix + L"sLogTemplate", sLogTemplate);
    inifile.SetValue(section, prefix + L"sLogTemplateCommit", sLogTemplateCommit);
    inifile.SetValue(section, prefix + L"sLogTemplateBranch", sLogTemplateBranch);
    inifile.SetValue(section, prefix + L"sLogTemplateImport", sLogTemplateImport);
    inifile.SetValue(section, prefix + L"sLogTemplateDelete", sLogTemplateDelete);
    inifile.SetValue(section, prefix + L"sLogTemplateMove", sLogTemplateMove);
    inifile.SetValue(section, prefix + L"sLogTemplateMkDir", sLogTemplateMkDir);
    inifile.SetValue(section, prefix + L"sLogTemplatePropset", sLogTemplatePropset);
    inifile.SetValue(section, prefix + L"sLogTemplateLock", sLogTemplateLock);
    inifile.SetValue(section, prefix + L"sLogRevRegex", sLogRevRegex);
    inifile.SetValue(section, prefix + L"nBugIdPos", std::to_wstring(nBugIdPos).c_str());
    inifile.SetValue(section, prefix + L"m_bFound", m_bFound ? L"1" : L"0");
    inifile.SetValue(section, prefix + L"m_bPropsRead", m_bPropsRead ? L"1" : L"0");
}

void ProjectProperties::LoadFromIni(CSimpleIni& inifile, const CString& section, const CString& prefix /*= L"pp"*/)
{
    assert(inifile.IsMultiLine());
    sLabel                          = inifile.GetValue(section, prefix + L"sLabel", L"");
    sMessage                        = inifile.GetValue(section, prefix + L"sMessage", L"");
    bNumber                         = _wtoi(inifile.GetValue(section, prefix + L"bNumber", L"1"));
    sUrl                            = inifile.GetValue(section, prefix + L"sUrl", L"");
    bWarnIfNoIssue                  = _wtoi(inifile.GetValue(section, prefix + L"bWarnIfNoIssue", L"0"));
    bAppend                         = _wtoi(inifile.GetValue(section, prefix + L"bAppend", L"1"));
    sProviderParams                 = inifile.GetValue(section, prefix + L"sProviderParams", L"");
    nLogWidthMarker                 = _wtoi(inifile.GetValue(section, prefix + L"nLogWidthMarker", L"0"));
    nMinLogSize                     = _wtoi(inifile.GetValue(section, prefix + L"nMinLogSize", L"0"));
    nMinLockMsgSize                 = _wtoi(inifile.GetValue(section, prefix + L"nMinLockMsgSize", L"0"));
    bFileListInEnglish              = _wtoi(inifile.GetValue(section, prefix + L"bFileListInEnglish", L"1"));
    lProjectLanguage                = _wtoi(inifile.GetValue(section, prefix + L"lProjectLanguage", L"0"));
    m_sFpPath                       = inifile.GetValue(section, prefix + L"sFPPath", L"");
    m_sDpPath                       = inifile.GetValue(section, prefix + L"sDPPath", L"");
    sWebViewerRev                   = inifile.GetValue(section, prefix + L"sWebViewerRev", L"");
    sWebViewerPathRev               = inifile.GetValue(section, prefix + L"sWebViewerPathRev", L"");
    sLogSummaryRe                   = inifile.GetValue(section, prefix + L"sLogSummaryRe", L"");
    sStartCommitHook                = inifile.GetValue(section, prefix + L"sStartCommitHook", L"");
    sCheckCommitHook                = inifile.GetValue(section, prefix + L"sCheckCommitHook", L"");
    sPreCommitHook                  = inifile.GetValue(section, prefix + L"sPreCommitHook", L"");
    sManualPreCommitHook            = inifile.GetValue(section, prefix + L"sManualPreCommitHook", L"");
    sPostCommitHook                 = inifile.GetValue(section, prefix + L"sPostCommitHook", L"");
    sStartUpdateHook                = inifile.GetValue(section, prefix + L"sStartUpdateHook", L"");
    sPreUpdateHook                  = inifile.GetValue(section, prefix + L"sPreUpdateHook", L"");
    sPostUpdateHook                 = inifile.GetValue(section, prefix + L"sPostUpdateHook", L"");
    sPreConnectHook                 = inifile.GetValue(section, prefix + L"sPreConnectHook", L"");
    sPreLockHook                    = inifile.GetValue(section, prefix + L"sPreLockHook", L"");
    sPostLockHook                   = inifile.GetValue(section, prefix + L"sPostLockHook", L"");
    sRepositoryRootUrl              = inifile.GetValue(section, prefix + L"sRepositoryRootUrl", L"");
    sRepositoryPathUrl              = inifile.GetValue(section, prefix + L"sRepositoryPathUrl", L"");
    sMergeLogTemplateTitle          = inifile.GetValue(section, prefix + L"sMergeLogTemplateTitle", L"");
    sMergeLogTemplateReverseTitle   = inifile.GetValue(section, prefix + L"sMergeLogTemplateReverseTitle", L"");
    sMergeLogTemplateMsg            = inifile.GetValue(section, prefix + L"sMergeLogTemplateMsg", L"");
    bMergeLogTemplateMsgTitleBottom = _wtoi(inifile.GetValue(section, prefix + L"bMergeLogTemplateMsgTitleBottom", L"0"));
    sAutoProps                      = inifile.GetValue(section, prefix + L"sAutoProps", L"");
    sProviderUuid                   = inifile.GetValue(section, prefix + L"sProviderUuid", L"");
    sProviderUuid64                 = inifile.GetValue(section, prefix + L"sProviderUuid64", L"");
    sCheckRe                        = inifile.GetValue(section, prefix + L"sCheckRe", L"");
    sBugIDRe                        = inifile.GetValue(section, prefix + L"sBugIDRe", L"");
    sLogTemplate                    = inifile.GetValue(section, prefix + L"sLogTemplate", L"");
    sLogTemplateCommit              = inifile.GetValue(section, prefix + L"sLogTemplateCommit", L"");
    sLogTemplateBranch              = inifile.GetValue(section, prefix + L"sLogTemplateBranch", L"");
    sLogTemplateImport              = inifile.GetValue(section, prefix + L"sLogTemplateImport", L"");
    sLogTemplateDelete              = inifile.GetValue(section, prefix + L"sLogTemplateDelete", L"");
    sLogTemplateMove                = inifile.GetValue(section, prefix + L"sLogTemplateMove", L"");
    sLogTemplateMkDir               = inifile.GetValue(section, prefix + L"sLogTemplateMkDir", L"");
    sLogTemplatePropset             = inifile.GetValue(section, prefix + L"sLogTemplatePropset", L"");
    sLogTemplateLock                = inifile.GetValue(section, prefix + L"sLogTemplateLock", L"");
    sLogRevRegex                    = inifile.GetValue(section, prefix + L"sLogRevRegex", LOG_REVISIONREGEX);
    nBugIdPos                       = _wtoi(inifile.GetValue(section, prefix + L"nBugIdPos", L"-1"));
    m_bFound                        = _wtoi(inifile.GetValue(section, prefix + L"m_bFound", L"0")) != 0;
    m_bPropsRead                    = _wtoi(inifile.GetValue(section, prefix + L"m_bPropsRead", L"0")) != 0;
}

#ifdef DEBUG
[[maybe_unused]] static class PropTest
{
public:
    PropTest()
    {
        CString           msg      = L"this is a test logmessage: issue 222\nIssue #456, #678, 901  #456";
        CString           sUrl     = L"http://tortoisesvn.tigris.org/issues/show_bug.cgi?id=%BUGID%";
        CString           sCheckRe = L"[Ii]ssue #?(\\d+)(,? ?#?(\\d+))+";
        CString           sBugIDRe = L"(\\d+)";
        ProjectProperties props;
        props.sCheckRe = L"PAF-[0-9]+";
        props.sUrl     = L"http://tortoisesvn.tigris.org/issues/show_bug.cgi?id=%BUGID%";
        CString sRet   = props.FindBugID(L"This is a test for PAF-88");
        ATLASSERT(sRet.IsEmpty());
        props.sCheckRe        = L"[Ii]ssue #?(\\d+)";
        props.regExNeedUpdate = true;
        sRet                  = props.FindBugID(L"Testing issue #99");
        sRet.Trim();
        ATLASSERT(sRet.Compare(L"99") == 0);
        props.sCheckRe        = L"[Ii]ssues?:?(\\s*(,|and)?\\s*#\\d+)+";
        props.sBugIDRe        = L"(\\d+)";
        props.sUrl            = L"http://tortoisesvn.tigris.org/issues/show_bug.cgi?id=%BUGID%";
        props.regExNeedUpdate = true;
        sRet                  = props.FindBugID(L"This is a test for Issue #7463,#666");
        ATLASSERT(props.HasBugID(L"This is a test for Issue #7463,#666"));
        ATLASSERT(!props.HasBugID(L"This is a test for Issue 7463,666"));
        sRet.Trim();
        ATLASSERT(sRet.Compare(L"666 7463") == 0);
        sRet = props.FindBugID(L"This is a test for Issue #850,#1234,#1345");
        sRet.Trim();
        ATLASSERT(sRet.Compare(L"850 1234 1345") == 0);
        props.sCheckRe        = L"^\\[(\\d+)\\].*";
        props.sUrl            = L"http://tortoisesvn.tigris.org/issues/show_bug.cgi?id=%BUGID%";
        props.regExNeedUpdate = true;
        sRet                  = props.FindBugID(L"[000815] some stupid programming error fixed");
        sRet.Trim();
        ATLASSERT(sRet.Compare(L"000815") == 0);
        props.sCheckRe        = L"\\[\\[(\\d+)\\]\\]\\]";
        props.sUrl            = L"http://tortoisesvn.tigris.org/issues/show_bug.cgi?id=%BUGID%";
        props.regExNeedUpdate = true;
        sRet                  = props.FindBugID(L"test test [[000815]]] some stupid programming error fixed");
        sRet.Trim();
        ATLASSERT(sRet.Compare(L"000815") == 0);
        ATLASSERT(props.HasBugID(L"test test [[000815]]] some stupid programming error fixed"));
        ATLASSERT(!props.HasBugID(L"test test [000815]] some stupid programming error fixed"));
        props.sLogSummaryRe = L"\\[SUMMARY\\]:(.*)";
        ATLASSERT(props.GetLogSummary(L"[SUMMARY]: this is my summary").Compare(L"this is my summary") == 0);
    }
} propTest;
#endif
