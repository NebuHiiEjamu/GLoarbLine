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
	
	// programmer
	{ OS_ErrorCode(-5019),	UAWError::eRequire						},
	{ OS_ErrorCode(-112),	UAWError::eRequire						},
	{ OS_ErrorCode(-50),	UAWError::eRequire						},
	{ OS_ErrorCode(-40),	UAWError::eRequire						},

	// memory
	{ OS_ErrorCode(-151),	UAWError::eMemoryNotEnough				},
	{ OS_ErrorCode(-108),	UAWError::eMemoryNotEnough				},
	{ OS_ErrorCode(-41),	UAWError::eMemoryNotEnough				},
	{ OS_ErrorCode(-117),	UAWError::eMemoryInvalid				},
	{ OS_ErrorCode(-116),	UAWError::eMemoryInvalid				},
	{ OS_ErrorCode(-115),	UAWError::eMemoryInvalid				},
	{ OS_ErrorCode(-114),	UAWError::eMemoryInvalid				},
	{ OS_ErrorCode(-113),	UAWError::eMemoryInvalid				},
	{ OS_ErrorCode(-111),	UAWError::eMemoryInvalid				},
	{ OS_ErrorCode(-110),	UAWError::eMemoryInvalid				},
	{ OS_ErrorCode(-109),	UAWError::eMemoryInvalid				},
	{ OS_ErrorCode(-99),	UAWError::eMemoryInvalid				},

	// resources
	{ OS_ErrorCode(-199),	UAWError::eResourceInvalid				},
	{ OS_ErrorCode(-197),	UAWError::eResourceRemoveFailed			},
	{ OS_ErrorCode(-196),	UAWError::eResourceRemoveFailed			},
	{ OS_ErrorCode(-195),	UAWError::eResourceAddFailed			},
	{ OS_ErrorCode(-194),	UAWError::eResourceAddFailed			},
	{ OS_ErrorCode(-192),	UAWError::eResourceNotFound				},
	{ OS_ErrorCode(-186),	UAWError::eResourceInvalid				},
	{ OS_ErrorCode(-185),	UAWError::eResourceInvalid				},

	// files
	{ OS_ErrorCode(-5039),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-5038),	UAWError::eSourceDestinationSame		},
	{ OS_ErrorCode(-5037),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-5036),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-5035),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-5034),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-5033),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-5032),	UAWError::eItemLocked					},
	{ OS_ErrorCode(-5031),	UAWError::eDiskLocked					},
	{ OS_ErrorCode(-5030),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-5029),	UAWError::eNoSuchFolder					},
	{ OS_ErrorCode(-5028),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-5027),	UAWError::eDiskDisappeared				},
	{ OS_ErrorCode(-5026),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-5025),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-5024),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-5023),	UAWError::eAccessDenied					},
	{ OS_ErrorCode(-5022),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-5021),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-5020),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-5018),	UAWError::eNoSuchItem					},
	{ OS_ErrorCode(-5017),	UAWError::eItemAlreadyExists			},
	{ OS_ErrorCode(-5016),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-5015),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-5014),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-5013),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-5012),	UAWError::eNoSuchItem					},
	{ OS_ErrorCode(-5011),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-5010),	UAWError::eFileInUse					},
	{ OS_ErrorCode(-5009),	UAWError::eEndOfFile					},
	{ OS_ErrorCode(-5008),	UAWError::eNotEnoughSpace				},
	{ OS_ErrorCode(-5007),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-5006),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-5005),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-5004),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-5003),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-5002),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-5001),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-5000),	UAWError::eAccessDenied					},
	{ OS_ErrorCode(-1310),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-1309),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-1308),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-1307),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-1306),	UAWError::eSourceDestinationSame		},
	{ OS_ErrorCode(-1305),	UAWError::eDiskSoftwareDamaged			},
	{ OS_ErrorCode(-1304),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-1303),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-1302),	UAWError::eFileExpected					},
	{ OS_ErrorCode(-1301),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-1300),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-193),	UAWError::eFileNotFound					},
	{ OS_ErrorCode(-124),	UAWError::eDiskDisappeared				},
	{ OS_ErrorCode(-123),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-122),	UAWError::eBadMove						},
	{ OS_ErrorCode(-121),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-120),	UAWError::eNoSuchFolder					},
	{ OS_ErrorCode(-61),	UAWError::eAccessDenied					},
	{ OS_ErrorCode(-60),	UAWError::eDiskSoftwareDamaged			},
	{ OS_ErrorCode(-59),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-58),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-57),	UAWError::eDiskFormatUnsupported		},
	{ OS_ErrorCode(-56),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-55),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-54),	UAWError::eFileInUse					},
	{ OS_ErrorCode(-53),	UAWError::eDiskOffline					},
	{ OS_ErrorCode(-52),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-51),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-49),	UAWError::eFileInUse					},
	{ OS_ErrorCode(-48),	UAWError::eItemAlreadyExists			},
	{ OS_ErrorCode(-47),	UAWError::eFileInUse					},
	{ OS_ErrorCode(-46),	UAWError::eDiskLocked					},
	{ OS_ErrorCode(-45),	UAWError::eItemLocked					},
	{ OS_ErrorCode(-44),	UAWError::eDiskLocked					},
	{ OS_ErrorCode(-43),	UAWError::eNoSuchItem					},
	{ OS_ErrorCode(-42),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-39),	UAWError::eEndOfFile					},
	{ OS_ErrorCode(-38),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-37),	UAWError::eBadName						},
	{ OS_ErrorCode(-36),	UAWError::eUnknownFileSysError			},
	{ OS_ErrorCode(-35),	UAWError::eNoSuchDisk					},
	{ OS_ErrorCode(-34),	UAWError::eNotEnoughSpace				},
	{ OS_ErrorCode(-33),	UAWError::eUnknownFileSysError			},

	// network
	{ OS_ErrorCode(-619),	UAWError::eNetworkUnknown				},
	{ OS_ErrorCode(-198),	UAWError::eNetworkUnknown				},
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


HL_End_Namespace_BigRedH
