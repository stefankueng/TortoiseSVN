<xsl:stylesheet 
 xmlns:xsl="http://www.w3.org/1999/XSL/Transform" 
 xmlns:fo="http://www.w3.org/1999/XSL/Format" 
 version="1.0"> 
 
<xsl:import href="./db_pdfdoc.xsl"/> 
<xsl:import href="./defaults.xsl"/> 
<xsl:param name="paper.type" select="'A4'"></xsl:param> 
<xsl:param name="double.sided" select="1"></xsl:param> 
<xsl:param name="variablelist.as.blocks" select="1"></xsl:param> 
<xsl:param name="symbol.font.family" select="'Symbol,ZapfDingbats'"></xsl:param> 
<xsl:param name="draft.mode" select="no"/> 
 
<xsl:param name="formal.title.placement"> 
  figure after 
  example after 
  equation after 
  table after 
  procedure after 
</xsl:param> 
 
<xsl:template match="menuchoice">
  <fo:inline font-family="Helvetica">
    <xsl:call-template name="process.menuchoice"/>
  </fo:inline>
</xsl:template>

<xsl:template match="guilabel">
  <fo:inline font-family="Helvetica">
    <xsl:call-template name="inline.charseq"/>
  </fo:inline>
</xsl:template>

<xsl:template match="guibutton">
  <fo:inline font-family="Helvetica">
    <xsl:call-template name="inline.charseq"/>
  </fo:inline>
</xsl:template>

<xsl:template match="keysym">
  <fo:inline font-family="Symbol">
    <xsl:call-template name="inline.charseq"/>
  </fo:inline>
</xsl:template>
	
<xsl:template match="thead">
  <xsl:variable name="tgroup" select="parent::*"/>

  <fo:table-header
    background-color="#7099C5">
    <xsl:apply-templates select="row[1]">
      <xsl:with-param name="spans">
        <xsl:call-template name="blank.spans">
          <xsl:with-param name="cols" select="../@cols"/>
        </xsl:call-template>
      </xsl:with-param>
    </xsl:apply-templates>
  </fo:table-header>
</xsl:template>

<xsl:template match="tfoot">
  <xsl:variable name="tgroup" select="parent::*"/>

  <fo:table-footer
    background-color="#7099C5">
    <xsl:apply-templates select="row[1]">
      <xsl:with-param name="spans">
        <xsl:call-template name="blank.spans">
          <xsl:with-param name="cols" select="../@cols"/>
        </xsl:call-template>
      </xsl:with-param>
    </xsl:apply-templates>
  </fo:table-footer>
</xsl:template>

<xsl:template match="tbody">
  <xsl:variable name="tgroup" select="parent::*"/>

  <fo:table-body
  background-color="#f0f0ff">
			<xsl:apply-templates select="row[1]">
      <xsl:with-param name="spans">
        <xsl:call-template name="blank.spans">
          <xsl:with-param name="cols" select="../@cols"/>
        </xsl:call-template>
      </xsl:with-param>
    </xsl:apply-templates>
  </fo:table-body>
</xsl:template>
 
</xsl:stylesheet> 
