// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2010, 2012 - TortoiseSVN

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
#include "SVNConfig.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "SVNGlobal.h"
#pragma warning(push)
#include "svn_error.h"
#include "svn_pools.h"
#include "svn_config.h"
#include "svn_wc.h"
#pragma warning(pop)

SVNConfig* SVNConfig::m_pInstance;


SVNConfig::SVNConfig(void)
    : config(nullptr)
    , patterns(nullptr)
{
    svn_error_t * err;
    parentpool = svn_pool_create(NULL);

    err = svn_config_ensure(NULL, parentpool);
    pool = svn_pool_create (parentpool);
    // set up the configuration
    if (err == 0)
        err = svn_config_get_config (&(config), g_pConfigDir, parentpool);

    if (err != 0)
    {
        svn_error_clear(err);
    }
}

SVNConfig::~SVNConfig(void)
{
    svn_pool_destroy (pool);
    svn_pool_destroy (parentpool);
    delete m_pInstance;
}

BOOL SVNConfig::GetDefaultIgnores()
{
    if (config == nullptr)
        return FALSE;
    svn_error_t * err;
    patterns = NULL;
    err = svn_wc_get_default_ignores (&(patterns), config, pool);
    if (err)
    {
        svn_error_clear(err);
        return FALSE;
    }

    return TRUE;
}

BOOL SVNConfig::MatchIgnorePattern(const CString& name)
{
    if (patterns == NULL)
        return FALSE;
    return svn_wc_match_ignore_list(CUnicodeUtils::GetUTF8(name), patterns, pool);
}

BOOL SVNConfig::KeepLocks()
{
    if (config == nullptr)
        return FALSE;
    svn_boolean_t no_unlock = FALSE;
    svn_config_t * opt = (svn_config_t *)apr_hash_get (config, SVN_CONFIG_CATEGORY_CONFIG,
        APR_HASH_KEY_STRING);
    if (opt)
        svn_error_clear(svn_config_get_bool(opt, &no_unlock, SVN_CONFIG_SECTION_MISCELLANY, SVN_CONFIG_OPTION_NO_UNLOCK, FALSE));
    return no_unlock;
}

bool SVNConfig::SetUpSSH(svn_client_ctx_t * ctx)
{
    bool bRet = false;
    //set up the SVN_SSH param
    CString tsvn_ssh = CRegString(_T("Software\\TortoiseSVN\\SSH"));
    if (tsvn_ssh.IsEmpty() && ctx->config)
    {
        // check whether the ssh client is already set in the Subversion config
        svn_config_t * cfg = (svn_config_t *)apr_hash_get (ctx->config, SVN_CONFIG_CATEGORY_CONFIG,
            APR_HASH_KEY_STRING);
        if (cfg)
        {
            const char * sshValue = NULL;
            svn_config_get(cfg, &sshValue, SVN_CONFIG_SECTION_TUNNELS, "ssh", "");
            if ((sshValue == NULL)||(sshValue[0] == 0))
                tsvn_ssh = _T("\"") + CPathUtils::GetAppDirectory() + _T("TortoisePlink.exe") + _T("\"");
        }
    }
    tsvn_ssh.Replace('\\', '/');
    if (!tsvn_ssh.IsEmpty() && ctx->config)
    {
        svn_config_t * cfg = (svn_config_t *)apr_hash_get (ctx->config, SVN_CONFIG_CATEGORY_CONFIG,
            APR_HASH_KEY_STRING);
        if (cfg)
        {
            svn_config_set(cfg, SVN_CONFIG_SECTION_TUNNELS, "ssh", CUnicodeUtils::GetUTF8(tsvn_ssh));
            bRet = true;
        }
    }
    return bRet;
}
