/* (c)2003 Hotsprings Inc. Licensed under GPL - see LICENSE in HotlineSources diresctory */

#include "AW.h"
#include "UAWError.h"

HL_Begin_Namespace_BigRedH

static struct ErrorCodeTranslationTable
{
	OS_ErrorCode mOS_Code;	
	AW_ErrorCode mAW_Code;	
} 
	gErrorCodeTranslationTable[] =
{
	// none
	{ kNoOSError,			UAWError::eNone							},
	
	// memory
	{ OS_ErrorCode(8),		UAWError::eMemoryNotEnough				},
	{ OS_ErrorCode(14),		UAWError::eMemoryNotEnough				},

	// file system
	{ OS_ErrorCode(2),		UAWError::eFileNotFound					},
	{ OS_ErrorCode(3),		UAWError::eNoSuchItem					},
	{ OS_ErrorCode(5),		UAWError::eAccessDenied					},
	{ OS_ErrorCode(15),		UAWError::eNoSuchDisk					},
	{ OS_ErrorCode(17),		UAWError::eDifferentDisks				},
	{ OS_ErrorCode(19),		UAWError::eItemLocked					},
	{ OS_ErrorCode(32),		UAWError::eFileInUse					},
	{ OS_ErrorCode(38),		UAWError::eEndOfFile					},
	{ OS_ErrorCode(52),		UAWError::eItemAlreadyExists			},
	{ OS_ErrorCode(80),		UAWError::eItemAlreadyExists			},
	{ OS_ErrorCode(108),	UAWError::eDiskLocked					},
	{ OS_ErrorCode(112),	UAWError::eNotEnoughSpace				},
	{ OS_ErrorCode(123),	UAWError::eBadName						},
	{ OS_ErrorCode(161),	UAWError::eBadName						},
	{ OS_ErrorCode(183),	UAWError::eItemAlreadyExists			},
	{ OS_ErrorCode(206),	UAWError::eBadName						},
	{ OS_ErrorCode(1393),	UAWError::eDiskDamaged					},

	// network
	{ OS_ErrorCode(1236),	UAWError::eNetworkConnectionAborted		},
	{ OS_ErrorCode(10048),	UAWError::eNetworkAddressInUse			},
	{ OS_ErrorCode(10049),	UAWError::eNetworkAddressInUse			},
	{ OS_ErrorCode(10050),	UAWError::eNetworkHostUnreachable		},
	{ OS_ErrorCode(10051),	UAWError::eNetworkHostUnreachable		},
	{ OS_ErrorCode(10052),	UAWError::eNetworkHostUnreachable		},
	{ OS_ErrorCode(10053),	UAWError::eNetworkConnectionAborted		},
	{ OS_ErrorCode(10054),	UAWError::eNetworkUnknown				},
	{ OS_ErrorCode(10055),	UAWError::eNetworkUnknown				},
	{ OS_ErrorCode(10056),	UAWError::eNetworkUnknown				},
	{ OS_ErrorCode(10057),	UAWError::eNetworkUnknown				},
	{ OS_ErrorCode(10058),	UAWError::eNetworkUnknown				},
	{ OS_ErrorCode(10059),	UAWError::eNetworkUnknown				},
	{ OS_ErrorCode(10060),	UAWError::eNetworkConnectionTimedOut	},
	{ OS_ErrorCode(10061),	UAWError::eNetworkConnectionRefused		},
	{ OS_ErrorCode(10062),	UAWError::eNetworkUnknown				},
	{ OS_ErrorCode(10063),	UAWError::eNetworkUnknown				},
	{ OS_ErrorCode(10064),	UAWError::eNetworkConnectionTimedOut	},
	{ OS_ErrorCode(10065),	UAWError::eNetworkHostUnreachable		},
	{ OS_ErrorCode(11001),	UAWError::eNetworkBadAddress			},
	{ OS_ErrorCode(11002),	UAWError::eNetworkBadAddress			},

	// misc
	{ OS_ErrorCode(258),	UAWError::eTimeout						},
	{ OS_ErrorCode(1121),	UAWError::eTimeout						},

	// application
	{ OS_ErrorCode(1444),	UAWError::eInvalidThreadID				},

	// thread
	{ OS_ErrorCode(155),	UAWError::eCreateThread					},

};

// ---------------------------------------------------------------------
//  TranslateErrorCode                                  [public][static]
// ---------------------------------------------------------------------
// Translate the OS_ErrorCode into our own cross platform error codes

UAWError::EAWError 
UAWError::TranslateErrorCode( OS_ErrorCode inErrorCode )
{
	UAWError::EAWError errCode(UAWError::eNone);
	
	SInt16 n = sizeof(gErrorCodeTranslationTable)/sizeof(gErrorCodeTranslationTable[0]);

	for(SInt16 i = 0; i < n; i++)
	{
		if(gErrorCodeTranslationTable[i].mOS_Code == inErrorCode)
		{
			errCode = gErrorCodeTranslationTable[i].mAW_Code;
			break;
		}
	}

	return errCode;
}


// ---------------------------------------------------------------------
//  MessageFromErrorCode                                [public][static]
// ---------------------------------------------------------------------
// returns the OS specific message attached to this OS specific errCode

/*
CString
UAWError::MessageFromErrorCode( OS_ErrorCode inErrorCode )
{
	Char* where = nil;
	DWORD ret = ::FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER
							   | FORMAT_MESSAGE_FROM_SYSTEM
							   | FORMAT_MESSAGE_IGNORE_INSERTS
				   ,nil //msg source (module id or string)
				   ,inErrorCode
				   ,MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT) //?? Default language
				   ,(LPTSTR) &where
				   ,64  // min allocated buffer size (reasonable initial amount)
				   ,nil); // arguments to format in
	REQUIRE( where != nil );
	// add an ASCIIZ just to be shure
	where[ret] = 0; 
	// make shure the buffer is deallocated with LocalFree
	StLocalMemoryWin buf(where,ret);
	return CString(where);
}
*/



HL_End_Namespace_BigRedH

