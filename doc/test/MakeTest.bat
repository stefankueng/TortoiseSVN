@echo off
:: There is no easy way to avoid using an absolute path here.
:: To avoid editing a versioned file you should create a text file
:: called DocPath.txt which contains the path to the TSVN doc folder.
:: It must use forward slashes and URL escaping if necessary - no spaces!
if not exist DocPath.txt echo c:/TortoiseSVN/doc > DocPath.txt
for /f %%p in (DocPath.txt) do @set docpath=%%p

if exist temp rd /s/q temp
md temp
cd temp
md repos
md doc
md docs
md repos2
md ext
svnadmin create repos --fs-type fsfs
svnadmin create repos2 --fs-type fsfs
svn co -q file:///%docpath%/test/temp/repos doc
if errorlevel 1 goto checkout_fail
svn co -q file:///%docpath%/test/temp/repos docs
svn co -q file:///%docpath%/test/temp/repos2 ext

:: This is used to add content to the 'external' repository.
cd ext
:: Copy some files from the docs to create content
copy ..\..\..\source\en\TortoiseSVN\tsvn_server\server*.xml > nul
svn add server*.xml
svn ci -q -m "Document the server" .
cd ..
rd /s/q ext

cd doc
:: Copy some files from the docs to create content
type nul > ..\targets
for %%f in (add blame checkout) do (
	@copy ..\..\..\source\en\TortoiseSVN\tsvn_dug\dug_%%f.xml+..\..\footnote1.txt dug_%%f.xml> nul
	@echo dug_%%f.xml >> ..\targets
)
svn add --no-auto-props --targets ..\targets
svn ci -q -m "Document some commands" .
type nul > ..\targets
for %%f in (commit export ignore) do (
	@copy ..\..\..\source\en\TortoiseSVN\tsvn_dug\dug_%%f.xml+..\..\footnote1.txt dug_%%f.xml> nul
	@echo dug_%%f.xml >> ..\targets
)
svn add --no-auto-props --targets ..\targets
svn ci -q -m "Document commands group 2" .
type nul > ..\targets
for %%f in (relocate revert showlog) do (
	@copy ..\..\..\source\en\TortoiseSVN\tsvn_dug\dug_%%f.xml+..\..\footnote1.txt dug_%%f.xml> nul
	@echo dug_%%f.xml >> ..\targets
)
svn add --targets ..\targets
svn ci -q -m "Document some more commands" .

:: Add a reference to the external repository
svn ps -q svn:externals "ext file:///%docpath%/test/temp/repos2/" .
svn ci -q -m "create externals reference" .
svn up -q

copy ..\..\subwcrev1.txt subwcrev.txt > nul
svn add --no-auto-props subwcrev.txt
svn ci -q -m "Document the SubWCRev program" .
svn up -q ../docs

:: Now make some changes that will cause conflicts later.
:: Change detection is broken when timestamps are the same.
:: Force a current timestamp by using type instead of copy.
type ..\..\subwcrev2.txt > subwcrev.txt
for %%f in (ignore showlog) do (
	@copy ..\..\..\source\en\TortoiseSVN\tsvn_dug\dug_%%f.xml+..\..\footnote2.txt dug_%%f.xml> nul
)
svn ci -q -m "Clarify the description of SubWCRev" .

:: Add yet more files
type nul > ..\targets
for %%f in (branchtag conflicts general) do (
	@copy ..\..\..\source\en\TortoiseSVN\tsvn_dug\dug_%%f.xml > nul
	@echo dug_%%f.xml >> ..\targets
)
type ..\..\footnote1.txt >> dug_ignore.xml
svn add --no-auto-props --targets ..\targets
svn ci -q -m "Document final commands" .

:: Update to just before the latest commit
svn up -q -r6

:: Modify some files
for %%f in (add blame relocate) do (
	@copy ..\..\..\source\en\TortoiseSVN\tsvn_dug\dug_%%f.xml+..\..\footnote2.txt dug_%%f.xml> nul
)
:: Add an unversioned file
echo Read Me > readme.txt
:: Add a nested WC
md nested
svn co -q file:///%docpath%/test/temp/repos2 nested
:: Change a file property
svn ps -q local:property "This is a local property" dug_revert.xml
:: Lock a file
svn lock -m "Major update" dug_add.xml > NUL

cd ..\docs
:: Make changes which conflict with what we did earlier.
copy /y ..\..\subwcrev3.txt subwcrev.txt > nul
for %%f in (checkout commit export ignore showlog) do (
	@copy ..\..\..\source\en\TortoiseSVN\tsvn_dug\dug_%%f.xml+..\..\footnote3.txt dug_%%f.xml> nul
)
svn diff . > ..\docs.patch
svn up -q

cd ..
del targets
goto end
:checkout_fail
echo Perhaps you have not set the correct unix-style path in DocPath.txt
:end
cd ..
set docpath=
