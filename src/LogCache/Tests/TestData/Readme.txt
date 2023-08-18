This is a pre-cooked repository cache for the test in ..\CachedLogInfoTests.cpp

* To update src-LogCache-all
  - Clear TortoiseSVN log cache files (%APPDATA%\TortoiseSVN\logcache)
  - Open TortoiseSVN log for https://svn.code.sf.net/p/tortoisesvn/code/trunk/src/LogCache
    TortoiseProc.exe /command:log /path:https://svn.code.sf.net/p/tortoisesvn/code/trunk/src/LogCache /startrev:27194 /endrev:1
  - Click "Show All"
  - Close TortoiseSVN
  - Replace src-LogCache-all with %APPDATA%\TortoiseSVN\logcache\https___svn.code.sf.net_p_tortoisesvn_code[guid]
