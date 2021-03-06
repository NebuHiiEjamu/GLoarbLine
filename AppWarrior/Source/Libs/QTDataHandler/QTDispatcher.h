// =====================================================================
//  QTDispatcher.h                       (C) Hotline Communications 2000
// =====================================================================
// A set of functions to respond to QuickTime's requests

#ifndef _H_QTDISPATCHER_
#define _H_QTDISPATCHER_


//-----------------------------------------------------------------------
// Includes
#include <Components.h>
#include <QuickTimeComponents.h>

HL_Begin_Namespace_BigRedH

class CDataHandlerConnection;

// Note: all below functions are called through the main dispatcher
// All of them have to be defined using pascal calling convention

pascal ComponentResult  DataHandlerOpen                       (CDataHandlerConnection* handler, ComponentInstance self);
pascal ComponentResult  DataHandlerClose                      (CDataHandlerConnection* handler, ComponentInstance self);
pascal ComponentResult  DataHandlerCanDo                      (CDataHandlerConnection* handler, short selector);
pascal ComponentResult  DataHandlerVersion                    (CDataHandlerConnection* handler);

pascal ComponentResult  DataHandlerCanUseDataRef              (CDataHandlerConnection* handler, Handle dataref, long * flags);
pascal ComponentResult  DataHandlerSetDataRef                 (CDataHandlerConnection* handler, Handle dataref);
pascal ComponentResult  DataHandlerGetFileSize                (CDataHandlerConnection* handler, long *filesize);
pascal ComponentResult  DataHandlerGetAvailableFileSize       (CDataHandlerConnection* handler, long *filesize);
pascal ComponentResult  DataHandlerGetFileName                (CDataHandlerConnection* handler, StringPtr fileName);
pascal ComponentResult  DataHandlerGetMimeType                (CDataHandlerConnection* handler, StringPtr mimeType);
pascal ComponentResult  DataHandlerGetFileType                (CDataHandlerConnection* handler, OSType *fileType);
pascal ComponentResult  DataHandlerOpenForRead                (CDataHandlerConnection* handler);
pascal ComponentResult  DataHandlerCloseForRead               (CDataHandlerConnection* handler);
pascal ComponentResult  DataHandlerGetScheduleAheadTime       (CDataHandlerConnection* handler, long * milliseconds);
pascal ComponentResult  DataHandlerCompareDataRef             (CDataHandlerConnection* handler, Handle dataref, Boolean * equal);
pascal ComponentResult  DataHandlerDataTask                   (CDataHandlerConnection* handler);
pascal ComponentResult  DataHandlerScheduleData               (CDataHandlerConnection* handler, Ptr placeToPutData, long fileOffset, long dataSize, long refCon,
																DataHSchedulePtr scheduleRecPtr,  DataHCompletionUPP completionRtn);
pascal ComponentResult  DataHandlerFinishData                 (CDataHandlerConnection* handler, Ptr placeToPutData, Boolean cancel);
pascal ComponentResult  DataHandlerFlushData                  (CDataHandlerConnection* handler);

HL_End_Namespace_BigRedH


#endif //_H_QTDISPATCHER_
