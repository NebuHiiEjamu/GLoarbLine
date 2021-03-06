/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */
//
//  Windows-specific implementation of QuickTime component's dispatcher

//-----------------------------------------------------------------------
// includes

#include "aw.h"
#undef PRAGMA_ONCE // to avoid macro redefinition warning
#include <Components.h> // Mac's components 
#include "QTDispatcher.h" 
#include "CDataHandlerComponent.h"
#include "USyncroHelpers.h"

HL_Using_Namespace_BigRedH

/////////////////////////////////////////////////////////////////////////
// main dispatcher function
extern "C" pascal ComponentResult HL_Handler(ComponentParameters *params, Handle storage)
{
	StMutex lock(CDataHandlerComponent::Instance().mEnter);

	ComponentFunctionUPP hlProc = nil;
	ComponentResult err = noErr;

#if DEBUG
		if (params->what != kDataHTaskSelect && 
			params->what != kDataHGetAvailableFileSizeSelect)
		UDebug::Message("(%x):%s\n", (int)storage, 
			CDataHandlerComponent::Instance().GetSelectorName(params->what));
#endif

        if (params->what < 0) {
            switch(params->what) {
                // common to all components selectors
                case kComponentOpenSelect:       hlProc = (ComponentFunctionUPP)DataHandlerOpen; break;
                case kComponentCloseSelect:      hlProc = (ComponentFunctionUPP)DataHandlerClose; break;
                case kComponentCanDoSelect:      hlProc = (ComponentFunctionUPP)DataHandlerCanDo; break;
                case kComponentVersionSelect:    hlProc = (ComponentFunctionUPP)DataHandlerVersion; break;
            }
        }
        else {
            switch (params->what) {
                // insert datahandling-specific dispatch here
                case kDataHCanUseDataRefSelect: hlProc = (ComponentFunctionUPP)DataHandlerCanUseDataRef; break;
                case kDataHSetDataRefSelect:    hlProc = (ComponentFunctionUPP)DataHandlerSetDataRef; break;
                case kDataHGetFileNameSelect:   hlProc = (ComponentFunctionUPP)DataHandlerGetFileName; break;
				case kDataHGetMIMETypeSelect:   hlProc = (ComponentFunctionUPP)DataHandlerGetMimeType; break;
				case kDataHGetMacOSFileTypeSelect:   hlProc = (ComponentFunctionUPP)DataHandlerGetFileType; break;

                case kDataHGetFileSizeSelect:   hlProc = (ComponentFunctionUPP)DataHandlerGetFileSize; break;
				case kDataHGetAvailableFileSizeSelect:
												hlProc = (ComponentFunctionUPP)DataHandlerGetAvailableFileSize; break;

                case kDataHScheduleDataSelect:  hlProc = (ComponentFunctionUPP)DataHandlerScheduleData; break;
                case kDataHFinishDataSelect:    hlProc = (ComponentFunctionUPP)DataHandlerFinishData; break;
                case kDataHGetScheduleAheadTimeSelect: hlProc = (ComponentFunctionUPP)DataHandlerGetScheduleAheadTime; break;
                case kDataHOpenForReadSelect:   hlProc = (ComponentFunctionUPP)DataHandlerOpenForRead; break;
                case kDataHCloseForReadSelect:  hlProc = (ComponentFunctionUPP)DataHandlerCloseForRead; break;
                case kDataHCompareDataRefSelect:hlProc = (ComponentFunctionUPP)DataHandlerCompareDataRef; break;
                case kDataHTaskSelect:          hlProc = (ComponentFunctionUPP)DataHandlerDataTask; break;
				case kDataHFlushDataSelect:		hlProc = (ComponentFunctionUPP)DataHandlerFlushData; break;

                default:  err = badComponentSelector; // not implemented
            }
        }

        if (hlProc)
			err = CallComponentFunctionWithStorage(
				(Handle)storage, params, hlProc);

        return err;
}
