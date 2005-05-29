@echo off
SETLOCAL ENABLEDELAYEDEXPANSION
..\..\bin\release\bin\SubWCRev.exe ..\.. Setup.wxs Setup_good.wxs
if NOT EXIST Setup_good.wxs (copy Setup.wxs Setup_good.wxs)
..\..\bin\release\bin\SubWCRev.exe ..\.. makemsisub.in makemsisub.bat
if NOT EXIST makemsisub.bat (copy makemsisub.in makemsisub.bat)
rem include the spell checker and thesaurus if they're available
set ENSPELL=0
set ENTHES=0
if EXIST ..\..\..\Common\Spell_Thes\en_US.aff (set ENSPELL=1)
if EXIST ..\..\..\Common\Spell_Thes\th_en_US_v2.idx (set ENTHES=1)
rem don't include the crashreport dll if the build is not done by myself.
rem Since I don't have the debug symbols of those builds, crashreports sent
rem to me are unusable.
set MYBUILD=0
if EXIST ..\..\MYBUILD (set MYBUILD=1)
call makemsisub.bat
del makemsisub.bat