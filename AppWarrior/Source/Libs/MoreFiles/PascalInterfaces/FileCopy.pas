{
     File:       FileCopy.p
 
     Contains:   A robust, general purpose file copy routine.
 
     Version:    Technology: MoreFiles
                 Release:    1.5.2
 
     Copyright:  � 1992-2001 by Apple Computer, Inc., all rights reserved.
 
     Bugs?:      For bug reports, consult the following page on
                 the World Wide Web:
 
                     http://developer.apple.com/bugreporter/
 
}

{
    You may incorporate this sample code into your applications without
    restriction, though the sample code has been provided "AS IS" and the
    responsibility for its operation is 100% yours.  However, what you are
    not permitted to do is to redistribute the source as "DSC Sample Code"
    after having made changes. If you're going to re-distribute the source,
    we require that you make it clear in the source that the code was
    descended from Apple Sample Code, but that you've made changes.
}

{$IFC UNDEFINED UsingIncludes}
{$SETC UsingIncludes := 0}
{$ENDC}

{$IFC NOT UsingIncludes}
 UNIT FileCopy;
 INTERFACE
{$ENDC}

{$IFC UNDEFINED __FILECOPY__}
{$SETC __FILECOPY__ := 1}

{$I+}
{$SETC FileCopyIncludes := UsingIncludes}
{$SETC UsingIncludes := 1}

{$IFC UNDEFINED __MACTYPES__}
{$I MacTypes.p}
{$ENDC}
{$IFC UNDEFINED __FILES__}
{$I Files.p}
{$ENDC}

{$PUSH}
{$ALIGN MAC68K}
{$LibExport+}

{***************************************************************************}

FUNCTION FileCopy(srcVRefNum: INTEGER; srcDirID: LONGINT; srcName: Str255; dstVRefNum: INTEGER; dstDirID: LONGINT; dstPathname: Str255; copyName: Str255; copyBufferPtr: UNIV Ptr; copyBufferSize: LONGINT; preflight: BOOLEAN): OSErr;

{
    The FileCopy function duplicates a file and optionally renames it.
    Since the PBHCopyFile routine is only available on some
    AFP server volumes under specific conditions, this routine
    either uses PBHCopyFile, or does all of the work PBHCopyFile
    does.  The srcVRefNum, srcDirID and srcName are used to
    determine the location of the file to copy.  The dstVRefNum
    dstDirID and dstPathname are used to determine the location of
    the destination directory.  If copyName <> NIL, then it points
    to the name of the new file.  If copyBufferPtr <> NIL, it
    points to a buffer of copyBufferSize that is used to copy
    the file's data.  The larger the supplied buffer, the
    faster the copy.  If copyBufferPtr = NIL, then this routine
    allocates a buffer in the application heap. If you pass a
    copy buffer to this routine, make its size a multiple of 512
    ($200) bytes for optimum performance.
    
    srcVRefNum      input:  Source volume specification.
    srcDirID        input:  Source directory ID.
    srcName         input:  Source file name.
    dstVRefNum      input:  Destination volume specification.
    dstDirID        input:  Destination directory ID.
    dstPathname     input:  Pointer to destination directory name, or
                            nil when dstDirID specifies a directory.
    copyName        input:  Points to the new file name if the file is
                            to be renamed or nil if the file isn't to
                            be renamed.
    copyBufferPtr   input:  Points to a buffer of copyBufferSize that
                            is used the i/o buffer for the copy or
                            nil if you want FileCopy to allocate its
                            own buffer in the application heap.
    copyBufferSize  input:  The size of the buffer pointed to
                            by copyBufferPtr.
    preflight       input:  If true, FileCopy makes sure there are enough
                            allocation blocks on the destination volume to
                            hold both the data and resource forks before
                            starting the copy.
    
    Result Codes
        noErr               0       No error
        readErr             �19     Driver does not respond to read requests
        writErr             �20     Driver does not respond to write requests
        badUnitErr          �21     Driver reference number does not
                                    match unit table
        unitEmptyErr        �22     Driver reference number specifies a
                                    nil handle in unit table
        abortErr            �27     Request aborted by KillIO
        notOpenErr          �28     Driver not open
        dskFulErr           -34     Destination volume is full
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        tmfoErr             -42     Too many files open
        fnfErr              -43     Source file not found, or destination
                                    directory does not exist
        wPrErr              -44     Volume locked by hardware
        fLckdErr            -45     File is locked
        vLckdErr            -46     Destination volume is read-only
        fBsyErr             -47     The source or destination file could
                                    not be opened with the correct access
                                    modes
        dupFNErr            -48     Destination file already exists
        opWrErr             -49     File already open for writing
        paramErr            -50     No default volume or function not
                                    supported by volume
        permErr             -54     File is already open and cannot be opened using specified deny modes
        memFullErr          -108    Copy buffer could not be allocated
        dirNFErr            -120    Directory not found or incomplete pathname
        wrgVolTypErr        -123    Function not supported by volume
        afpAccessDenied     -5000   User does not have the correct access
        afpDenyConflict     -5006   The source or destination file could
                                    not be opened with the correct access
                                    modes
        afpObjectTypeErr    -5025   Source is a directory, directory not found
                                    or incomplete pathname
    
    __________
    
    Also see:   FSpFileCopy, DirectoryCopy, FSpDirectoryCopy
}

{***************************************************************************}

FUNCTION FSpFileCopy({CONST}VAR srcSpec: FSSpec; {CONST}VAR dstSpec: FSSpec; copyName: Str255; copyBufferPtr: UNIV Ptr; copyBufferSize: LONGINT; preflight: BOOLEAN): OSErr;

{
    The FSpFileCopy function duplicates a file and optionally renames it.
    Since the PBHCopyFile routine is only available on some
    AFP server volumes under specific conditions, this routine
    either uses PBHCopyFile, or does all of the work PBHCopyFile
    does.  The srcSpec is used to
    determine the location of the file to copy.  The dstSpec is
    used to determine the location of the
    destination directory.  If copyName <> NIL, then it points
    to the name of the new file.  If copyBufferPtr <> NIL, it
    points to a buffer of copyBufferSize that is used to copy
    the file's data.  The larger the supplied buffer, the
    faster the copy.  If copyBufferPtr = NIL, then this routine
    allocates a buffer in the application heap. If you pass a
    copy buffer to this routine, make its size a multiple of 512
    ($200) bytes for optimum performance.
    
    srcSpec         input:  An FSSpec record specifying the source file.
    dstSpec         input:  An FSSpec record specifying the destination
                            directory.
    copyName        input:  Points to the new file name if the file is
                            to be renamed or nil if the file isn't to
                            be renamed.
    copyBufferPtr   input:  Points to a buffer of copyBufferSize that
                            is used the i/o buffer for the copy or
                            nil if you want FileCopy to allocate its
                            own buffer in the application heap.
    copyBufferSize  input:  The size of the buffer pointed to
                            by copyBufferPtr.
    preflight       input:  If true, FSpFileCopy makes sure there are
                            enough allocation blocks on the destination
                            volume to hold both the data and resource forks
                            before starting the copy.
    
    Result Codes
        noErr               0       No error
        readErr             �19     Driver does not respond to read requests
        writErr             �20     Driver does not respond to write requests
        badUnitErr          �21     Driver reference number does not
                                    match unit table
        unitEmptyErr        �22     Driver reference number specifies a
                                    nil handle in unit table
        abortErr            �27     Request aborted by KillIO
        notOpenErr          �28     Driver not open
        dskFulErr           -34     Destination volume is full
        nsvErr              -35     No such volume
        ioErr               -36     I/O error
        bdNamErr            -37     Bad filename
        tmfoErr             -42     Too many files open
        fnfErr              -43     Source file not found, or destination
                                    directory does not exist
        wPrErr              -44     Volume locked by hardware
        fLckdErr            -45     File is locked
        vLckdErr            -46     Destination volume is read-only
        fBsyErr             -47     The source or destination file could
                                    not be opened with the correct access
                                    modes
        dupFNErr            -48     Destination file already exists
        opWrErr             -49     File already open for writing
        paramErr            -50     No default volume or function not
                                    supported by volume
        permErr             -54     File is already open and cannot be opened using specified deny modes
        memFullErr          -108    Copy buffer could not be allocated
        dirNFErr            -120    Directory not found or incomplete pathname
        wrgVolTypErr        -123    Function not supported by volume
        afpAccessDenied     -5000   User does not have the correct access
        afpDenyConflict     -5006   The source or destination file could
                                    not be opened with the correct access
                                    modes
        afpObjectTypeErr    -5025   Source is a directory, directory not found
                                    or incomplete pathname
    
    __________
    
    Also see:   FileCopy, DirectoryCopy, FSpDirectoryCopy
}

{***************************************************************************}

{$ALIGN RESET}
{$POP}

{$SETC UsingIncludes := FileCopyIncludes}

{$ENDC} {__FILECOPY__}

{$IFC NOT UsingIncludes}
 END.
{$ENDC}
