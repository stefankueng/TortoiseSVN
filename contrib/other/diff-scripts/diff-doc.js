var objArgs,num,sBaseDoc,sNewDoc,objScript,word,destination;
// WdCompareTarget
var wdCompareTargetSelected = 0;
var wdCompareTargetCurrent = 1;
var wdCompareTargetNew = 2;
// WdViewType
var wdMasterView = 5;
var wdNormalView = 1;
var wdOutlineView = 2;

objArgs = WScript.Arguments;
num = objArgs.length;
if (num < 2)
{
   WScript.Echo("Usage: [CScript | WScript] diff-doc.js base.doc new.doc");
   WScript.Quit(1);
}

sBaseDoc = objArgs(0);
sNewDoc = objArgs(1);

objScript = new ActiveXObject("Scripting.FileSystemObject");
if ( ! objScript.FileExists(sBaseDoc))
{
    WScript.Echo("File " + sBaseDoc + " does not exist.  Cannot compare the documents.");
    WScript.Quit(1);
}
if ( ! objScript.FileExists(sNewDoc))
{
    WScript.Echo("File " + sNewDoc +" does not exist.  Cannot compare the documents.");
    WScript.Quit(1);
}

objScript = null;

try
{
   word = WScript.CreateObject("Word.Application");
}
catch(e)
{
   WScript.Echo("You must have Microsoft Word installed to perform this operation.");
   WScript.Quit(1);
}

word.visible = true;

// Open the new document
destination = word.Documents.Open(sNewDoc);

if(((destination.ActiveWindow.View.Type == wdOutlineView) || (destination.ActiveWindow.View.Type == wdMasterView)) && (destination.Subdocuments.Count == 0))
{
    destination.ActiveWindow.View.Type = wdNormalView;
}


// Compare to the base document
destination.Compare(sBaseDoc, "", wdCompareTargetNew, true, true);
    
// Show the comparison result
if (Number(word.Version) < 12)
{
	word.ActiveDocument.Windows(1).Visible = 1;
}
    
// Mark the comparison document as saved to prevent the annoying
// "Save as" dialog from appearing.
word.ActiveDocument.Saved = 1;
    
// Close the first document
if (Number(word.Version) >= 10)
{
    destination.Close();
}
