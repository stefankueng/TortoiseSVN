// MyGraph.h

#pragma once

/////////////////////////////////////////////////////////////////////////////
// MyGraphSeries

class MyGraphSeries : public CObject
{
    friend class MyGraph;

    // Construction.
public:
    MyGraphSeries(const CString& sLabel = L"");
    ~MyGraphSeries() override;

    // Declared but not defined.
private:
    MyGraphSeries(const MyGraphSeries& rhs) = delete;
    MyGraphSeries& operator=(const MyGraphSeries& rhs) = delete;

    // Operations.
public:
    void Clear()
    {
        m_dwaValues.RemoveAll();
        m_oaRegions.RemoveAll();
    }
    void    SetLabel(const CString& sLabel);
    CString GetLabel() const;
    void    SetData(int nGroup, int nValue);
    int     GetData(int nGroup) const;

    // Implementation.
private:
    int     GetMaxDataValue(bool bStackedGraph) const;
    int     GetAverageDataValue() const;
    int     GetNonZeroElementCount() const;
    int     GetDataTotal() const;
    void    SetTipRegion(int nGroup, const CRect& rc);
    void    SetTipRegion(int nGroup, CRgn* prgn);
    int     HitTest(const CPoint& pt, int searchStart) const;
    CString GetTipText(int nGroup, const CString& unitString) const;

    // Data.
private:
    CString              m_sLabel;    // Series label.
    CDWordArray          m_dwaValues; // Values array.
    CArray<CRgn*, CRgn*> m_oaRegions; // Tooltip regions.
};

/////////////////////////////////////////////////////////////////////////////
// MyGraph

class MyGraph : public CStatic
{
    // Enum.
public:
    enum GraphType
    {
        Bar,
        Line,
        PieChart
    }; // Renamed 'Pie' because it hides a GDI function name

    // Construction.
public:
    MyGraph(GraphType eGraphType = MyGraph::PieChart, bool bStackedGraph = false);
    ~MyGraph() override;

    // Declared but not defined.
private:
    MyGraph(const MyGraph& rhs) = delete;
    MyGraph& operator=(const MyGraph& rhs) = delete;

    // Operations.
public:
    void Clear();
    void AddSeries(MyGraphSeries& rMyGraphSeries);
    void SetXAxisLabel(const CString& sLabel);
    void SetYAxisLabel(const CString& sLabel);
    int  AppendGroup(const CString& sLabel);
    void SetLegend(int nGroup, const CString& sLabel);
    void SetGraphType(GraphType eType, bool bStackedGraph);
    void SetGraphTitle(const CString& sTitle);
    int  LookupLabel(const CString& sLabel) const;
    void DrawGraph(CDC& dc);

    // Implementation.
private:
    void DrawTitle(CDC& dc);
    void SetupAxes(CDC& dc);
    void DrawAxes(CDC& dc) const;
    void DrawLegend(CDC& dc);
    void DrawSeriesBar(CDC& dc) const;
    void DrawSeriesLine(CDC& dc) const;
    void DrawSeriesLineStacked(CDC& dc) const;
    void DrawSeriesPie(CDC& dc) const;

    int GetMaxLegendLabelLength(CDC& dc) const;
    int GetMaxSeriesSize() const;
    int GetMaxNonZeroSeriesSize() const;
    int GetMaxDataValue() const;
    int GetAverageDataValue() const;
    int GetNonZeroSeriesCount() const;

    CString GetTipText() const;

    INT_PTR OnToolHitTest(CPoint point, TOOLINFO* pTi) const override;

    CPoint WedgeEndFromDegrees(double degrees, const CPoint& ptCenter,
                               double radius) const;

    static UINT SpinTheMessageLoop(bool bNoDrawing   = false,
                                   bool bOnlyDrawing = false,
                                   UINT uiMsgAllowed = WM_NULL);

    static void     RGBtoHLS(COLORREF crRGB, WORD& wH, WORD& wL, WORD& wS);
    static COLORREF HLStoRGB(WORD wH, WORD wL, WORD wS);
    static WORD     HueToRGB(WORD w1, WORD w2, WORD wH);

protected:
    void PreSubclassWindow() override;

    // Generated message map functions
protected:
    afx_msg void OnPaint();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg BOOL OnNeedText(UINT uiId, NMHDR* pNMHDR, LRESULT* pResult);
    DECLARE_MESSAGE_MAP()

    // Data.
private:
    int                                   m_nXAxisWidth;
    int                                   m_nYAxisHeight;
    int                                   m_nAxisLabelHeight;
    int                                   m_nAxisTickLabelHeight;
    CPoint                                m_ptOrigin;
    CRect                                 m_rcGraph;
    CRect                                 m_rcLegend;
    CRect                                 m_rcTitle;
    CString                               m_sXAxisLabel;
    CString                               m_sYAxisLabel;
    CString                               m_sTitle;
    CDWordArray                           m_dwaColors;
    CStringArray                          m_saLegendLabels;
    CList<MyGraphSeries*, MyGraphSeries*> m_olMyGraphSeries;
    GraphType                             m_eGraphType;
    bool                                  m_bStackedGraph;
};
