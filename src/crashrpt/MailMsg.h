///////////////////////////////////////////////////////////////////////////////
//
//  Module: MailMsg.h
//
//    Desc: This class encapsulates the MAPI and CMC mail functions.
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <xcmc.h>          // CMC function defs
#include <mapi.h>          // MAPI function defs

#ifndef TStrStrVector
// STL generates various warnings.
// 4100: unreferenced formal parameter
// 4663: C++ language change: to explicitly specialize class template...
// 4018: signed/unsigned mismatch
// 4245: conversion from <a> to <b>: signed/unsigned mismatch
#pragma warning(push, 3)
#pragma warning(disable: 4100)
#pragma warning(disable: 4663)
#pragma warning(disable: 4018)
#pragma warning(disable: 4245)
#include <vector>
#pragma warning(pop)

typedef std::pair<string,string> TStrStrPair;
typedef std::vector<TStrStrPair> TStrStrVector;
#endif // !defined TStrStrVector

//
// Define CMC entry points
//
typedef CMC_return_code (FAR PASCAL *LPCMCLOGON) \
   (CMC_string, CMC_string, CMC_string, CMC_object_identifier, \
   CMC_ui_id, CMC_uint16, CMC_flags, CMC_session_id FAR*, \
   CMC_extension FAR*);

typedef CMC_return_code (FAR PASCAL *LPCMCSEND) \
   (CMC_session_id, CMC_message FAR*, CMC_flags, \
   CMC_ui_id, CMC_extension FAR*);

typedef CMC_return_code (FAR PASCAL *LPCMCLOGOFF) \
   (CMC_session_id, CMC_ui_id, CMC_flags, CMC_extension FAR*);

typedef CMC_return_code (FAR PASCAL *LPCMCQUERY) \
   (CMC_session_id, CMC_enum, CMC_buffer, CMC_extension FAR*);


////////////////////////////// Class Definitions /////////////////////////////

// ===========================================================================
// CMailMsg
// 
// See the module comment at top of file.
//
class CMailMsg  
{
public:
	CMailMsg();
	virtual ~CMailMsg();

   //-----------------------------------------------------------------------------
   // SetTo
   //    Sets the Email:To address
   //
   // Parameters
   //    sAddress    Address
   //    sName       Optional name
   //
   // Return Values
   //    CMailMsg reference
   //
   // Remarks
   //    Only one To address can be set.  If called more than once
   //    the last address will be used.
   //
   CMailMsg& 
   SetTo(
      string sAddress, 
      string sName = _T("")
      );

   //-----------------------------------------------------------------------------
   // SetCc
   //    Sets the Email:Cc address
   //
   // Parameters
   //    sAddress    Address
   //    sName       Optional name
   //
   // Return Values
   //    CMailMsg reference
   //
   // Remarks
   //    Multiple Cc addresses can be set.
   //
   CMailMsg& 
   SetCc(
      string sAddress, 
      string sName = _T("")
      );

   //-----------------------------------------------------------------------------
   // SetBc
   //    Sets the Email:Bcc address
   //
   // Parameters
   //    sAddress    Address
   //    sName       Optional name
   //
   // Return Values
   //    CMailMsg reference
   //
   // Remarks
   //    Multiple Bcc addresses can be set.
   //
   CMailMsg& 
   SetBc(
      string sAddress, 
      string sName = _T("")
      );

   //-----------------------------------------------------------------------------
   // SetFrom
   //    Sets the Email:From address
   //
   // Parameters
   //    sAddress    Address
   //    sName       Optional name
   //
   // Return Values
   //    CMailMsg reference
   //
   // Remarks
   //    Only one From address can be set.  If called more than once
   //    the last address will be used.
   //
   CMailMsg& 
   SetFrom(
      string sAddress, 
      string sName = _T("")
      );

   //-----------------------------------------------------------------------------
   // SetSubect
   //    Sets the Email:Subject
   //
   // Parameters
   //    sSubject    Subject
   //
   // Return Values
   //    CMailMsg reference
   //
   // Remarks
   //    none
   //
   CMailMsg& 
   SetSubject(
      string sSubject
      ) {m_sSubject = sSubject; return *this;};

   //-----------------------------------------------------------------------------
   // SetMessage
   //    Sets the Email message body
   //
   // Parameters
   //    sMessage    Message body
   //
   // Return Values
   //    CMailMsg reference
   //
   // Remarks
   //    none
   //
   CMailMsg& 
   SetMessage(
      string sMessage
      ) {m_sMessage = sMessage; return *this;};

   //-----------------------------------------------------------------------------
   // AddAttachment
   //    Attaches a file to the email
   //
   // Parameters
   //    sAttachment Fully qualified file name
   //    sTitle      File display name
   //
   // Return Values
   //    CMailMsg reference
   //
   // Remarks
   //    none
   //
   CMailMsg& 
   AddAttachment(
      string sAttachment, 
      string sTitle = _T("")
      );

   //-----------------------------------------------------------------------------
   // Send
   //    Send the email.
   //
   // Parameters
   //    none
   //
   // Return Values
   //    TRUE if successful
   //
   // Remarks
   //    First simple MAPI is used if unsuccessful CMC is used.
   //
   BOOL Send();

protected:

   //-----------------------------------------------------------------------------
   // CMCSend
   //    Send email using CMC functions.
   //
   // Parameters
   //    none
   //
   // Return Values
   //    TRUE if successful
   //
   // Remarks
   //    none
   //
   BOOL CMCSend();

   //-----------------------------------------------------------------------------
   // MAPISend
   //    Send email using MAPI functions.
   //
   // Parameters
   //    none
   //
   // Return Values
   //    -1: cancelled; 0: other failure; 1: success
   //
   // Remarks
   //    none
   //
   int MAPISend();

   //-----------------------------------------------------------------------------
   // Initialize
   //    Initialize MAPI32.dll
   //
   // Parameters
   //    none
   //
   // Return Values
   //    TRUE if successful
   //
   // Remarks
   //    none
   //
   BOOL Initialize();

   //-----------------------------------------------------------------------------
   // Uninitialize
   //    Uninitialize MAPI32.dll
   //
   // Parameters
   //    none
   //
   // Return Values
   //    void
   //
   // Remarks
   //    none
   //
   void Uninitialize();

	/*
	+------------------------------------------------------------------------------
	|
	|	Function:	cResolveName()
	|
	|	Parameters:	[IN]	lpszName = Name of e-mail recipient to resolve.
	|				[OUT]	ppRecips = Pointer to a pointer to an lpMapiRecipDesc
	|
	|	Purpose:	Resolves an e-mail address and returns a pointer to a 
	|				MapiRecipDesc structure filled with the recipient information
	|				contained in the address book.
	|
	|	Note:		ppRecips is allocated off the heap using MAPIAllocateBuffer.
	|				Any user of this method must be sure to release ppRecips when 
	|				done with it using either MAPIFreeBuffer or cFreeBuffer.
	+-------------------------------------------------------------------------------
	*/
   int cResolveName( LHANDLE m_lhSession, const char * lpszName, lpMapiRecipDesc *ppRecip );

   TStrStrVector  m_from;                       // From <address,name>
   TStrStrVector  m_to;                         // To <address,name>
   TStrStrVector  m_cc;                         // Cc <address,name>
   TStrStrVector  m_bcc;                        // Bcc <address,name>
   TStrStrVector  m_attachments;                // Attachment <file,title>
   string        m_sSubject;                   // EMail subject
   string        m_sMessage;                   // EMail message

   HMODULE        m_hMapi;                      // Handle to MAPI32.DLL
   LPCMCQUERY     m_lpCmcQueryConfiguration;    // Cmc func pointer
   LPCMCLOGON     m_lpCmcLogon;                 // Cmc func pointer
   LPCMCSEND      m_lpCmcSend;                  // Cmc func pointer
   LPCMCLOGOFF    m_lpCmcLogoff;                // Cmc func pointer
   LPMAPILOGON    m_lpMapiLogon;                // Mapi func pointer
   LPMAPISENDMAIL m_lpMapiSendMail;             // Mapi func pointer
   LPMAPILOGOFF   m_lpMapiLogoff;               // Mapi func pointer
   LPMAPIRESOLVENAME m_lpMapiResolveName;       // Mapi func pointer
   LPMAPIFREEBUFFER m_lpMapiFreeBuffer;         // Mapi func pointer
   
   BOOL           m_bReady;                     // MAPI is loaded

private:
	bool initIfNeeded();
};
