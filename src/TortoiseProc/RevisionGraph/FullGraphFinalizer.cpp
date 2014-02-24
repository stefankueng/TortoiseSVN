// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2010, 2012, 2014 - TortoiseSVN

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
#include "FullGraphFinalizer.h"
#include "FullHistory.h"
#include "FullGraph.h"
#include "CachedLogInfo.h"
#include "registry.h"
#include "UnicodeUtils.h"
#include "Access/CopyFollowingLogIterator.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CFullGraphFinalizer::CFullGraphFinalizer
    ( const CFullHistory& history
    , CFullGraph& graph)
    : history (history)
    , graph (graph)
{
    // initialize path classificator

    CRegStdString trunkPattern (L"Software\\TortoiseSVN\\RevisionGraph\\TrunkPattern", L"trunk");
    CRegStdString branchesPattern (L"Software\\TortoiseSVN\\RevisionGraph\\BranchPattern", L"branches");
    CRegStdString tagsPattern (L"Software\\TortoiseSVN\\RevisionGraph\\TagsPattern", L"tags");

    const CPathDictionary& paths = history.GetCache()->GetLogInfo().GetPaths();
    pathClassification.reset
        (new CPathClassificator ( paths
                                , CUnicodeUtils::StdGetUTF8 (trunkPattern)
                                , CUnicodeUtils::StdGetUTF8 (branchesPattern)
                                , CUnicodeUtils::StdGetUTF8 (tagsPattern)));
}

CFullGraphFinalizer::~CFullGraphFinalizer(void)
{
}

void CFullGraphFinalizer::Run()
{
    // nothing to do for empty graphs

    if (graph.GetRoot() == NULL)
        return;

    // init internal structures

    InitWCRevs();

    // say "renamed" for "Deleted"/"Added" entries

    FindRenames (graph.GetRoot());

    // classify all nodes (needs to fully passes):
    // classify nodes on by one

    ForwardClassification (graph.GetRoot());

    // propagate classifation back along copy history

    BackwardClassification (graph.GetRoot());
}

void CFullGraphFinalizer::InitWCRevs()
{
    // collect revisions to show

    std::vector<revision_t> revisions;

    revisions.push_back (history.GetWCInfo().minCommit);
    revisions.push_back (history.GetWCInfo().maxCommit);
    revisions.push_back (history.GetWCInfo().minAtRev);
    revisions.push_back (history.GetWCInfo().maxAtRev);

    std::sort (revisions.begin(), revisions.end());
    revisions.erase ( std::unique_copy ( revisions.begin()
                                       , revisions.end()
                                       , revisions.begin())
                    , revisions.end());

    // assign paths

    CDictionaryBasedTempPath path = *history.GetWCPath();
    revision_t pathRevision = history.GetPegRevision();

    while (   !revisions.empty()
           && (revisions.back() >= pathRevision)
           && path.IsValid())
    {
        wcRevs.insert ( wcRevs.begin()
                      , std::make_pair (revisions.back(), path.GetBasePath()));
        revisions.pop_back();
    }

    while (!revisions.empty())
    {
        revision_t revision = revisions.back();
        revisions.pop_back();

        // efficiently follow path changes only

        const CCachedLogInfo* cache = history.GetCache();
        const CRevisionIndex& revisionIndices = cache->GetRevisions();
        const CRevisionInfoContainer& info = cache->GetLogInfo();

        while (revision < pathRevision)
        {
            index_t index = revisionIndices[pathRevision];
            if (   (index != NO_INDEX)
                && (info.GetSumChanges (index) & CRevisionInfoContainer::HAS_COPY_FROM)
                && (info.GetRootPath (index).IsSameOrParentOf (path.GetBasePath())))
            {
                CCopyFollowingLogIterator iterator (cache, pathRevision, path);
                iterator.Advance();

                pathRevision = iterator.GetRevision();
                path = iterator.GetPath();
            }
            else
            {
                --pathRevision;
            }
        }

        if (path.IsValid())
            wcRevs.insert ( wcRevs.begin()
                          , std::make_pair (revision, path.GetBasePath()));
    }
}

void CFullGraphFinalizer::FindRenames (CFullGraphNode* node)
{
    // say "renamed" for "Deleted"/"Added" entries

    for ( CFullGraphNode * next = node->GetNext()
        ; node != NULL
        ; node = next, next = (next == NULL ? NULL : next->GetNext()))
    {
        if (   (next != NULL)
            && (next->GetClassification().Is (CNodeClassification::IS_DELETED)))
        {
            // this line will be deleted.
            // will it be continued exactly once under a different name?

            CFullGraphNode* renameTarget = NULL;
            CFullGraphNode::CCopyTarget** renameCopy = NULL;

            for ( CFullGraphNode::CCopyTarget** copy = &node->GetFirstCopyTarget()
                ; *copy != NULL
                ; copy = &(*copy)->next())
            {
                CFullGraphNode * target = (*copy)->value();
                assert (target->GetClassification().Is (CNodeClassification::IS_COPY_TARGET));

                if (target->GetRevision() == next->GetRevision())
                {
                    // that actually looks like a rename

                    if (renameTarget != NULL)
                    {
                        // there is more than one copy target
                        // -> display all individual deletion and additions

                        renameTarget = NULL;
                        break;
                    }
                    else
                    {
                        // remember the (potential) rename target

                        renameTarget = target;
                        renameCopy = copy;
                    }
                }
            }

            // did we find a unambigous rename target?

            if (renameTarget != NULL)
            {
                // optimize graph

                graph.Replace ( node->GetNext()
                              , *renameCopy
                              , CNodeClassification::IS_RENAMED);

                // "next" has just been destroyed

                next = node->GetNext();
            }
        }

        // recourse

        for ( const CFullGraphNode::CCopyTarget* copy = node->GetFirstCopyTarget()
            ; copy != NULL
            ; copy = copy->next())
        {
            FindRenames (copy->value());
        }
    }
}

// mark nodes according to local properties

void CFullGraphFinalizer::MarkRoot (CFullGraphNode* node)
{
    if (node == graph.GetRoot())
        node->AddClassification (CNodeClassification::IS_FIRST);
}

void CFullGraphFinalizer::MarkCopySource (CFullGraphNode* node) const
{
    if (node->GetFirstCopyTarget() != NULL)
        node->AddClassification (CNodeClassification::IS_COPY_SOURCE);
}

void CFullGraphFinalizer::MarkWCRevisions (CFullGraphNode* node)
{
    if (wcRevs.empty())
        return;

    // if this the same revision and path as the WC?

    if (   (node->GetRevision() == wcRevs.front().first)
        && (node->GetPath().GetBasePath().Intersects (wcRevs.front().second)))
    {
        node->AddClassification (CNodeClassification::IS_WORKINGCOPY);
        wcRevs.erase (wcRevs.begin());
    }

    // maybe, we need to insert more nodes

    while (!wcRevs.empty())
    {
        revision_t wcRev = wcRevs.front().first;
        const CDictionaryBasedPath& path = wcRevs.front().second;

        // maybe, we have to insert the WC node first?
        // (the sub-tree may not have been modified in that revision)

        if (// between this ...
               (node->GetRevision() < wcRev)
            // ... and the next node (if there is one) ...
            && (   (node->GetNext() == NULL)
                || (node->GetNext()->GetRevision() > wcRev))
            // ... on the right path ...
            && node->GetPath().GetBasePath().Intersects (path)
            // ... and not deleted, yet
            && !node->GetClassification().Is (CNodeClassification::IS_DELETED))
        {
            // WC rev lies behind this node and before the next one

            graph.Add ( node->GetPath()
                      , wcRev
                      , CNodeClassification::IS_WORKINGCOPY
                      , node);

            wcRevs.erase (wcRevs.begin());
        }
        else
        {
            return;
        }
    }
}

void CFullGraphFinalizer::MarkHead (CFullGraphNode* node) const
{
    // scan all "latest" nodes
    // (they must be either HEADs or special nodes)

    if (   (node->GetNext() != NULL)
        || (node->GetClassification().IsAnyOf
               (CNodeClassification::SUBTREE_DELETED)))
        return;

    // look for the latest change
    // (there may be several "copy-source-only" nodes trailing HEAD

    const DWORD forbidden = CNodeClassification::IS_OPERATION_MASK;
    while (   (node->GetClassification().Matches (0, forbidden))
           && (node->GetPrevious() != NULL))
        node = node->GetPrevious();

    node->AddClassification (CNodeClassification::IS_LAST);
}

void CFullGraphFinalizer::AddWCModification (CFullGraphNode* node)
{
    if (!history.GetWCInfo().modified)
        return;

    // is this the BASE node for our WC?

    if (node->GetClassification().Matches
            ( CNodeClassification::IS_WORKINGCOPY
            , CNodeClassification::IS_MODIFIED_WC))
    {
        if (node->GetRevision() == history.GetWCInfo().maxAtRev)
        {
            // add the modification node

            CNodeClassification classification
                = CNodeClassification::IS_MODIFIED_WC
                | CNodeClassification::IS_WORKINGCOPY
                | (node->GetNext() == NULL
                     ? 0
                     : CNodeClassification::IS_COPY_TARGET);
            graph.Add (node->GetPath(), node->GetRevision()+1, classification, node);
        }
    }
}

// classify nodes on by one

void CFullGraphFinalizer::ForwardClassification (CFullGraphNode* node)
{
    do
    {
        // add local classification

        MarkRoot (node);
        MarkCopySource (node);
        MarkWCRevisions (node);
        MarkHead (node);
        AddWCModification (node);

        // add path-based classification

        node->AddClassification ((*pathClassification)[node->GetPath()]);

        // recourse

        for ( const CFullGraphNode::CCopyTarget* copy = node->GetFirstCopyTarget()
            ; copy != NULL
            ; copy = copy->next())
        {
            ForwardClassification (copy->value());
        }

        node = node->GetNext();
    }
    while (node != NULL);
}

// propagate classifation back along copy history

DWORD CFullGraphFinalizer::BackwardClassification (CFullGraphNode* node)
{
    // start at the end of this chain

    assert (node->GetPrevious()== NULL);

    while (node->GetNext())
        node = node->GetNext();

    // classify this branch

    DWORD branchClassification = 0;

    do
    {
        // set classification on copies first

        DWORD commonCopyClassfication = (DWORD)(-1);  // flags set in all copyies
        DWORD aggregatedCopyClassification = 0;      // flags set in at least one copy

        for ( const CFullGraphNode::CCopyTarget* copy = node->GetFirstCopyTarget()
            ; copy != NULL
            ; copy = copy->next())
        {
            DWORD classification = BackwardClassification (copy->value());
            commonCopyClassfication &= classification;
            aggregatedCopyClassification |= classification;
        }

        // construct the common classification

        DWORD classification // aggregate changes along the branch
            =   branchClassification
              & ~CNodeClassification::ALL_COPIES_MASK;

        classification
            |=  (node->GetClassification().GetFlags() * CNodeClassification::PATH_ONLY_SHIFT)
              & CNodeClassification::PATH_ONLY_MASK;

        classification      // add what applies to all branches
            |=   commonCopyClassfication & branchClassification
               & CNodeClassification::ALL_COPIES_MASK;

        classification      // any change to this node applies to all copies as well
            |=  (node->GetClassification().GetFlags() * CNodeClassification::ALL_COPIES_SHIFT)
              & commonCopyClassfication
              & CNodeClassification::ALL_COPIES_MASK;

        classification      // add changes that occur in *any* sub-tree
            |=  (aggregatedCopyClassification * CNodeClassification::COPIES_TO_SHIFT)
              & CNodeClassification::COPIES_TO_MASK;

        // store and return the flags

        DWORD nodeClassification
            =   classification
              & (  CNodeClassification::ALL_COPIES_MASK
                 + CNodeClassification::COPIES_TO_MASK
                 + CNodeClassification::PATH_ONLY_MASK);

        node->AddClassification (nodeClassification);

        // current path classification

        branchClassification = classification | node->GetClassification().GetFlags();
        node = node->GetPrevious();
    }
    while (node != NULL);

    // done

    return branchClassification;
}
