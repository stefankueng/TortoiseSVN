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
#include "stdafx.h"
#include "StandardLayout.h"

#include "VisibleGraph.h"

#include "StandardLayoutRectList.h"
#include "StandardLayoutNodeList.h"
#include "StandardLayoutConnectionList.h"
#include "StandardLayoutTextList.h"

// initialization

CStandardLayoutNodeInfo::CStandardLayoutNodeInfo()
    : node (NULL)
    , firstSubBranch (NULL)
    , nextInBranch (NULL)
    , previousInBranch (NULL)
    , lastInBranch (NULL)
    , nextBranch (NULL)
    , previousBranch (NULL)
    , lastBranch (NULL)
    , rootID ((index_t)NO_INDEX)
    , subTreeWidth (1)
    , subTreeHeight (1)
    , subTreeWeight (1)
    , branchLength (1)
    , requiresRevision (true)
    , requiresPath (true)
    , requiresGap (true)
    , skipStartPathElements (0)
    , skipTailPathElements (0)
    , requiredSize (0, 0)
    , subTreeShift (0, 0)
    , treeShift (0, 0)
    , rect (0, 0, 0, 0)
    , parentBranch(NULL)
{
}

// sort nodes: make "heaviest" branch the first one

void CStandardLayout::SortNodes()
{
    for (size_t i = 0, count = nodes.size(); i < count; ++i)
        if (nodes[i].firstSubBranch != NULL)
        {
            // find first deepest sub-branch

            CStandardLayoutNodeInfo* firstBranch = nodes[i].firstSubBranch;
            CStandardLayoutNodeInfo* heaviestBranch = firstBranch;
            index_t maxWeight = heaviestBranch->subTreeWeight;

            for ( CStandardLayoutNodeInfo* node = heaviestBranch->nextBranch
                ; node != NULL
                ; node = node->nextBranch)
            {
                if (node->subTreeWeight > maxWeight)
                {
                    maxWeight = node->subTreeWeight;
                    heaviestBranch = node;
                }
            }

            // already the first one?

            if (firstBranch == heaviestBranch)
                continue;

            // update "last" pointers, if necessary

            CStandardLayoutNodeInfo* nextBranch = heaviestBranch->nextBranch;
            if (nextBranch == NULL)
            {
                CStandardLayoutNodeInfo* newLast = heaviestBranch->previousBranch;
                for ( CStandardLayoutNodeInfo* node = firstBranch
                    ; node != NULL
                    ; node = node->nextBranch)
                {
                    node->lastBranch = newLast;
                }
            }

            // mode branch to front

            heaviestBranch->previousBranch->nextBranch = nextBranch;
            if (nextBranch != NULL)
                nextBranch->previousBranch = heaviestBranch->previousBranch;

            heaviestBranch->previousBranch = NULL;
            heaviestBranch->nextBranch = firstBranch;
            firstBranch->previousBranch = heaviestBranch;

            nodes[i].firstSubBranch = heaviestBranch;
        }
}

// layout creation:
// * create a node info object for every node
// * calculate branch and tree sizes (in nodes)

void CStandardLayout::InitializeNodes ( const CVisibleGraphNode* start
                                      , CStandardLayoutNodeInfo* parentBranch)
{
    CStandardLayoutNodeInfo* previousInBranch = NULL;
    CStandardLayoutNodeInfo* lastInBranch = NULL;

    // copy this info to all sub-nodes:

    index_t rootID = nodes[start->GetIndex()].rootID;

    // measure subtree size, calculate branch length and back-link it

    index_t branchLength = 0;
    for ( const CVisibleGraphNode* node = start
        ; node != NULL
        ; node = node->GetNext())
    {
        // get the node, initialize it and update pointers

        CStandardLayoutNodeInfo& nodeInfo = nodes[node->GetIndex()];
        ++branchLength;

        assert (nodeInfo.node == NULL);
        nodeInfo.node = node;
        nodeInfo.rootID = rootID;
        nodeInfo.previousInBranch = previousInBranch;
        nodeInfo.parentBranch = parentBranch;
        if (previousInBranch != NULL)
            previousInBranch->nextInBranch = &nodeInfo;

        previousInBranch = &nodeInfo;
        lastInBranch = &nodeInfo;

        if (node->GetFirstCopyTarget())
        {
            CStandardLayoutNodeInfo* previousBranch = NULL;
            CStandardLayoutNodeInfo* lastBranch = NULL;

            // measure sub-branches and back-link them

            for ( const CVisibleGraphNode::CCopyTarget*
                    target = node->GetFirstCopyTarget()
                ; target != NULL
                ; target = target->next())
            {
                // get sub-branch node, initialize it and update pointers

                const CVisibleGraphNode* subNode = target->value();
                CStandardLayoutNodeInfo& subNodeInfo = nodes[subNode->GetIndex()];
                subNodeInfo.rootID = rootID;

                if (previousBranch != NULL)
                {
                    previousBranch->nextBranch = &subNodeInfo;
                    subNodeInfo.previousBranch = previousBranch;
                }
                else
                {
                    // mark the first branch

                    nodeInfo.firstSubBranch = &subNodeInfo;
                }

                previousBranch = &subNodeInfo;
                lastBranch = &subNodeInfo;

                // add branch

                InitializeNodes (subNode, &nodeInfo);

                // accumulate branch into sub-tree

                nodeInfo.subTreeWidth += subNodeInfo.subTreeWidth;
                nodeInfo.subTreeWeight += subNodeInfo.subTreeWeight;
                if (nodeInfo.subTreeHeight <= subNodeInfo.subTreeHeight)
                    nodeInfo.subTreeHeight = subNodeInfo.subTreeHeight+1;
            }

            // link sub-branches forward

            for ( const CVisibleGraphNode::CCopyTarget*
                    target = node->GetFirstCopyTarget()
                ; target != NULL
                ; target = target->next())
            {
                nodes[target->value()->GetIndex()].lastBranch = lastBranch;
            }
        }
    }

    // write branch lengths, adjust sub-tree heights and link forward

    for ( const CVisibleGraphNode* node = start
        ; node != NULL
        ; node = node->GetNext())
    {
        CStandardLayoutNodeInfo& nodeInfo = nodes[node->GetIndex()];
        nodeInfo.branchLength = branchLength;
        nodeInfo.lastInBranch = lastInBranch;

        if (nodeInfo.subTreeHeight < branchLength)
            nodeInfo.subTreeHeight = branchLength;
    }
}

void CStandardLayout::InitializeNodes()
{
    nodes.resize (graph->GetNodeCount());

    for (size_t i = 0, count = graph->GetRootCount(); i < count; ++i)
    {
        const CVisibleGraphNode* root = graph->GetRoot(i);
        nodes[root->GetIndex()].rootID = static_cast<index_t>(i);

        InitializeNodes (root, NULL);
    }

    // every node info must actually refer to a node

    for (size_t i = 0, count = nodes.size(); i < count; ++i)
        assert (nodes[i].node != NULL);

    // adjust sub-tree weights

    for (size_t i = nodes.size(); i > 0; --i)
    {
        CStandardLayoutNodeInfo& node = nodes[i-1];

        index_t weight = node.nextInBranch != NULL
                       ? node.nextInBranch->subTreeWeight+1
                       : 1;

        for ( CStandardLayoutNodeInfo* branch = node.firstSubBranch
            ; branch != NULL
            ; branch = branch->nextBranch)
            weight += branch->subTreeWeight;

        node.subTreeWeight = weight;
    }

    // prevent degenerated branches from growing too far to the right

    SortNodes();
}

// scan tree for connections between non-overlapping nodes

void CStandardLayout::CreateConnections()
{
    // there can't be more connections than nodes

    connections.reserve (nodes.size());
    for (index_t i = 0, count = (index_t)nodes.size(); i < count; ++i)
    {
        const CVisibleGraphNode* node = nodes[i].node;
        const CVisibleGraphNode* previousNode = node->GetSource();

        // skip roots

        if (previousNode == NULL)
            continue;

        // source rect

        const CRect& previousRect = nodes[previousNode->GetIndex()].rect;
        CRect rect = nodes[i].rect;

        // no line because nodes touch or overlap?

        rect.InflateRect (1, 1, 1, 1);
        if (TRUE == CRect().IntersectRect (rect, previousRect))
            continue;

        // an actual connection

        connections.push_back (std::make_pair (previousNode->GetIndex(), i));
    }
}

// scan tree for connections longer than 0

void CStandardLayout::CreateTexts()
{
    // there can be at most two texts per node

    texts.reserve (2*nodes.size());

    // cover all node rects
    // (connections and texts will lie within these bounds)

    for (index_t i = 0, count = (index_t)nodes.size(); i < count; ++i)
    {
        const CStandardLayoutNodeInfo& info = nodes[i];

        if (info.requiresRevision)
            texts.push_back (STextInfo (i, 0));

        if (info.requiresPath)
        {
            size_t visibleElementCount = info.node->GetPath().GetDepth()
                                       - info.skipStartPathElements
                                       - info.skipTailPathElements;
            for (index_t k = (index_t)visibleElementCount; k > 0; --k)
                texts.push_back (STextInfo (i, k));
        }
    }
}

// just iterate over all nodes

inline bool SortRectByLeft (const CRect& lhs, const CRect& rhs)
{
    return lhs.left < rhs.left;
};

void CStandardLayout::CloseTreeBoundingRectGaps()
{
    std::sort (trees.begin(), trees.end(), &SortRectByLeft);

    for (size_t i = 1, count = trees.size(); i < count; ++i)
    {
        LONG diff = (trees[i].left - trees[i-1].right + 1) / 2;
        trees[i-1].right += diff;
        trees[i].left -= diff;
    }
}

void CStandardLayout::CalculateTreeBoundingRects()
{
    // initialize with empty rect

    trees.resize (graph->GetRootCount());
    std::fill_n (trees.begin(), graph->GetRootCount(), CRect(0, 0, 0, 0));

    for (size_t i = 0, count = nodes.size(); i < count; ++i)
    {
        const CStandardLayoutNodeInfo& nodeInfo = nodes[i];
        const CSize& size = nodeInfo.requiredSize;

        CRect rect = nodeInfo.rect;
        rect.right = max (rect.left + size.cx, rect.right);
        rect.bottom = max (rect.top + size.cy, rect.bottom);

        CRect& bounds = trees[nodeInfo.rootID];
        if (bounds.Width() == 0)
            bounds = rect;
        else
            bounds |= rect;
    }
}

// just iterate over all nodes

void CStandardLayout::CalculateBoundingRect()
{
    // special case: empty graph

    if (nodes.empty())
    {
        boundingRect = CRect();
        return;
    }

    // cover all node rects
    // (connections and texts will lie within these bounds)

    boundingRect = trees[0];
    for (size_t i = 1, count = trees.size(); i < count; ++i)
        boundingRect |= trees[i];
}

// construction / destruction

CStandardLayout::CStandardLayout ( const CCachedLogInfo* cache
                                 , const CVisibleGraph* graph
                                 , const CFullHistory::SWCInfo& wcInfo)
    : cache (cache)
    , wcInfo (wcInfo)
    , graph (graph)
{
    InitializeNodes();
}

CStandardLayout::~CStandardLayout(void)
{
}

// call this after executing the format options

void CStandardLayout::Finalize()
{
    CreateConnections();
    CreateTexts();

    CalculateTreeBoundingRects();
    CloseTreeBoundingRectGaps();
    CalculateBoundingRect();
}

/// implement IRevisionGraphLayout

CRect CStandardLayout::GetRect() const
{
    return boundingRect;
}

const ILayoutRectList* CStandardLayout::GetTrees() const
{
    return new CStandardLayoutRectList (trees);
}

const ILayoutNodeList* CStandardLayout::GetNodes() const
{
    return new CStandardLayoutNodeList (nodes, cache, wcInfo);
}

const ILayoutConnectionList* CStandardLayout::GetConnections() const
{
    return new CStandardLayoutConnectionList (nodes, connections);
}

const ILayoutTextList* CStandardLayout::GetTexts() const
{
    return new CStandardLayoutTextList (nodes, texts);
}

/// implement IStandardLayoutNodeAccess

index_t CStandardLayout::GetNodeCount() const
{
    return static_cast<index_t>(nodes.size());
}

CStandardLayoutNodeInfo* CStandardLayout::GetNode (index_t index)
{
    return &nodes[index];
}

