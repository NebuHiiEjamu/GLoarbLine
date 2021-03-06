// =====================================================================
//  CDataHandlerComponent                (C) Hotline Communications 2000
// =====================================================================
// 
//  A class that wraps working with QuickTime from the datahandling 
//  component's side
// =====================================================================

#ifndef _H_DATAHANDLERCOMPONENT_
#define _H_DATAHANDLERCOMPONENT_


//-----------------------------------------------------------------------
// Includes
#include <Components.h>
#include <QuickTimeComponents.h>
#include "CDataHandlerConnection.h"
#include "CDataProvider.h"
#include "CMutex.h"


HL_Begin_Namespace_BigRedH


// QuickTime datahandling component-related class
class CDataHandlerComponent
{
	public:
		static CDataHandlerComponent&	Instance();

		virtual		~CDataHandlerComponent();

	protected:

		CDataHandlerComponent(); 

	private:

		static std::auto_ptr<CDataHandlerComponent>	sInstance; // Singleton

		std::list<CDataHandlerConnection*>	mConnections; // list of connections
		std::map<CString, CDataProvider*>	mProviders; // list of providers

		// private constants
		enum EDataHandlerConsts {
			eTestDHldrComponentSpec		= 15,
			eTestDHldrComponentVersion	= 3,
			eTimeAheadMs				= 5000
		};
	public:
		// for debugging
		char* GetSelectorName(int number); 
		CMutex mEnter;
	public:
		// Creates new connection object
		CDataHandlerConnection* NewConnection();

	    // Destroys a connection object
		void DeleteConnection(CDataHandlerConnection* inConnection);

		// do we support particular selector
		bool CanDo(short inSelector);

		// what is the current version of the component?
		UInt32 GetVersion();

		// can we use that dataref?
		bool CanUseDataRef(Handle inDataref);

		// can we use that dataref?
		bool IsReading( Str255 inTempFileName );
		void CancelReading( Str255 inTempFileName );

		// get a provider for the given dataref
		CDataProvider*	GetProvider(const CString& URL);

		// delete a provider
		void DeleteProvider(CDataProvider*);

		// how long ahead should QuickTime read before start playing?
		UInt32 GetAheadTime();
};


HL_End_Namespace_BigRedH

#endif //_H_DATAHANDLERCOMPONENT_
