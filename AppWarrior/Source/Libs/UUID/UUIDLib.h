/*	UUIDLib.h
 *
 *		The main interface to UUIDLib.
 *
 *		The declaration of the 'uuid' type is lifted from the Digital's
 *	DCE code.
 */

#ifndef PandaWave_UUIDLIB
#define PandaWave_UUIDLIB

#pragma import on

/************************************************************************/
/*																		*/
/*	Errors																*/
/*																		*/
/************************************************************************/

#define	kUUIDInternalError			-21001
#define kUUIDFormatError			-21002

/************************************************************************/
/*																		*/
/*	Unique Identifiers													*/
/*																		*/
/************************************************************************/

#pragma options align=mac68k

/*	uuid
 *
 *		Universal Unique ID
 */

struct uuid_t {
    unsigned long		time_low;
    unsigned short		time_mid;
    unsigned short		time_hi_and_version;
    unsigned char		clock_seq_hi_and_reserved;
    unsigned char		clock_seq_low;
    unsigned char		node[6];
};

#pragma options align=reset

typedef struct uuid_t uuid_t;
typedef struct uuid_t clsid_t;		/* class ID; refers to a particular thing	*/
typedef struct uuid_t iid_t;		/* interface ID; refers to an interface		*/

/************************************************************************/
/*																		*/
/*	UUID Routines														*/
/*																		*/
/*		Note that the format is the same used by Microsoft and HP		*/
/*	xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx								*/
/*	time_low mid  hi/v clock  node										*/
/*																		*/
/************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

extern int			IsUUIDEqual(const uuid_t *, const uuid_t *);
extern OSErr		GenerateUUID(uuid_t *);
extern OSErr		ConvertUUIDToString(const uuid_t *, Str255 string);
extern OSErr		ConvertUUIDFromString(uuid_t *, const Str255 string);

/*
 *	Predefined uuids
 */

extern const uuid_t uuid_NULL;

#pragma import reset

#ifdef __cplusplus
}
#endif

#endif /* PandaWave_UUIDLIB */
