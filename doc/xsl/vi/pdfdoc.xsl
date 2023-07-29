<xsl:stylesheet
 xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
 xmlns:fo="http://www.w3.org/1999/XSL/Format"
 version="1.0">

  <xsl:import href="../pdfdoc.xsl"/>

  <xsl:param name="l10n.gentext.language" select="'vi'"/>
  <xsl:param name="body.font.family" select="'Times New Roman'"/>
  <xsl:param name="title.font.family" select="'Arial'"/>
  <xsl:param name="sans.font.family" select="'Arial'"/>
  <xsl:param name="monospace.font.family" select="'Courier New'"/>

  <xsl:param name="hyphenate">false</xsl:param>

</xsl:stylesheet>
