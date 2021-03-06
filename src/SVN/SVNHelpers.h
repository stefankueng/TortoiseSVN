// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2010-2012, 2016, 2021 - TortoiseSVN

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

#pragma once

/*
 * allow compilation even if the following structs have not been
 * defined yet (i.e. in case of missing includes).
 */

// ReSharper disable CppInconsistentNaming
struct apr_pool_t;
struct svn_client_ctx_t;
struct svn_error_t;
struct apr_hash_t;
// ReSharper restore CppInconsistentNaming
class CTSVNPath;

/**
 * \ingroup SVN
 * This class encapsulates an apr_pool taking care of destroying it at end of scope
 * Use this class in preference to doing svn_pool_create and then trying to remember all
 * the svn_pool_destroys which might be needed.
 */
class SVNPool
{
public:
    SVNPool();
    explicit SVNPool(apr_pool_t* parentPool);
    ~SVNPool();

    // Not implemented - we don't want any copying of these objects
    SVNPool(const SVNPool& rhs) = delete;
    SVNPool& operator=(SVNPool& rhs) = delete;

public:
    operator apr_pool_t*() const;

private:
    apr_pool_t* m_pool;
};

//////////////////////////////////////////////////////////////////////////

class SVNHelper
{
public:
    SVNHelper();
    ~SVNHelper();
    SVNHelper(const SVNHelper&) = delete;
    SVNHelper& operator=(SVNHelper&) = delete;

public:
    apr_pool_t*        Pool() const { return m_pool; }
    svn_client_ctx_t*  ClientContext(apr_pool_t* pool) const;
    void               Cancel(bool bCancelled = true) { m_bCancelled = bCancelled; }
    static const char* GetUserAgentString(apr_pool_t* pool);
    static bool        IsVersioned(const CTSVNPath& path, bool mustBeOk);

protected:
    apr_pool_t*       m_pool;
    svn_client_ctx_t* m_ctx;
    apr_hash_t*       m_config;
    bool              m_bCancelled;

    static svn_error_t* cancelFunc(void* cancelBaton);
};
