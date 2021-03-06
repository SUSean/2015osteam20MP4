// filesys.cc
//	Routines to manage the overall operation of the file system.
//	Implements routines to map from textual file names to files.
//
//	Each file in the file system has:
//	   A file header, stored in a sector on disk
//		(the size of the file header data structure is arranged
//		to be precisely the size of 1 disk sector)
//	   A number of data blocks
//	   An entry in the file system directory
//
// 	The file system consists of several data structures:
//	   A bitmap of free disk sectors (cf. bitmap.h)
//	   A directory of file names and file headers
//
//      Both the bitmap and the directory are represented as normal
//	files.  Their file headers are located in specific sectors
//	(sector 0 and sector 1), so that the file system can find them
//	on bootup.
//
//	The file system assumes that the bitmap and directory files are
//	kept "open" continuously while Nachos is running.
//
//	For those operations (such as Create, Remove) that modify the
//	directory and/or bitmap, if the operation succeeds, the changes
//	are written immediately back to disk (the two files are kept
//	open during all this time).  If the operation fails, and we have
//	modified part of the directory and/or bitmap, we simply discard
//	the changed version, without writing it back to disk.
//
// 	Our implementation at this point has the following restrictions:
//
//	   there is no synchronization for concurrent accesses
//	   files have a fixed size, set when the file is created
//	   files cannot be bigger than about 3KB in size
//	   there is no hierarchical directory structure, and only a limited
//	     number of files can be added to the system
//	   there is no attempt to make the system robust to failures
//	    (if Nachos exits in the middle of an operation that modifies
//	    the file system, it may corrupt the disk)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.
#ifndef FILESYS_STUB

#include "copyright.h"
#include "debug.h"
#include "disk.h"
#include "pbitmap.h"
#include "directory.h"
#include "filehdr.h"
#include "filesys.h"

// Sectors containing the file headers for the bitmap of free sectors,
// and the directory of files.  These file headers are placed in well-known
// sectors, so that they can be located on boot-up.
#define FreeMapSector 		0
#define DirectorySector 	1

// Initial file sizes for the bitmap and directory; until the file system
// supports extensible files, the directory size sets the maximum number
// of files that can be loaded onto the disk.
#define FreeMapFileSize 	(NumSectors / BitsInByte)
#define NumDirEntries 		64//10
#define DirectoryFileSize 	(sizeof(DirectoryEntry) * NumDirEntries)

//----------------------------------------------------------------------
// FileSystem::FileSystem
// 	Initialize the file system.  If format = TRUE, the disk has
//	nothing on it, and we need to initialize the disk to contain
//	an empty directory, and a bitmap of free sectors (with almost but
//	not all of the sectors marked as free).
//
//	If format = FALSE, we just have to open the files
//	representing the bitmap and the directory.
//
//	"format" -- should we initialize the disk?
//----------------------------------------------------------------------

FileSystem::FileSystem(bool format)
{
    DEBUG(dbgFile, "Initializing the file system.");
    if (format) {
        PersistentBitmap *freeMap = new PersistentBitmap(NumSectors);
        Directory *directory = new Directory(NumDirEntries);
		FileHeader *mapHdr = new FileHeader;
		FileHeader *dirHdr = new FileHeader;

        DEBUG(dbgFile, "Formatting the file system.");

		// First, allocate space for FileHeaders for the directory and bitmap
		// (make sure no one else grabs these!)
		freeMap->Mark(FreeMapSector);
		freeMap->Mark(DirectorySector);

		// Second, allocate space for the data blocks containing the contents
		// of the directory and bitmap files.  There better be enough space!

		ASSERT(mapHdr->Allocate(freeMap, FreeMapFileSize));
		ASSERT(dirHdr->Allocate(freeMap, DirectoryFileSize));

		// Flush the bitmap and directory FileHeaders back to disk
		// We need to do this before we can "Open" the file, since open
		// reads the file header off of disk (and currently the disk has garbage
		// on it!).

        DEBUG(dbgFile, "Writing headers back to disk.");
		mapHdr->WriteBack(FreeMapSector);
		dirHdr->WriteBack(DirectorySector);

		// OK to open the bitmap and directory files now
		// The file system operations assume these two files are left open
		// while Nachos is running.

        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
		// Once we have the files "open", we can write the initial version
		// of each file back to disk.  The directory at this point is completely
		// empty; but the bitmap has been changed to reflect the fact that
		// sectors on the disk have been allocated for the file headers and
		// to hold the file data for the directory and bitmap.

        DEBUG(dbgFile, "Writing bitmap and directory back to disk.");
		freeMap->WriteBack(freeMapFile);	 // flush changes to disk
		directory->WriteBack(directoryFile);

		if (debug->IsEnabled('f')) {
			freeMap->Print();
			directory->Print();
        }
        delete freeMap;
		delete directory;
		delete mapHdr;
		delete dirHdr;
    } else {
		// if we are not formatting the disk, just open the files representing
		// the bitmap and directory; these are left open while Nachos is running
        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
    }
}

//----------------------------------------------------------------------
// MP4 mod tag
// FileSystem::~FileSystem
//----------------------------------------------------------------------
FileSystem::~FileSystem()
{
	delete freeMapFile;
	delete directoryFile;
}

//----------------------------------------------------------------------
// FileSystem::Create
// 	Create a file in the Nachos file system (similar to UNIX create).
//	Since we can't increase the size of files dynamically, we have
//	to give Create the initial size of the file.
//
//	The steps to create a file are:
//	  Make sure the file doesn't already exist
//        Allocate a sector for the file header
// 	  Allocate space on disk for the data blocks for the file
//	  Add the name to the directory
//	  Store the new file header on disk
//	  Flush the changes to the bitmap and the directory back to disk
//
//	Return TRUE if everything goes ok, otherwise, return FALSE.
//
// 	Create fails if:
//   		file is already in directory
//	 	no free space for file header
//	 	no free entry for file in directory
//	 	no free space for data blocks for the file
//
// 	Note that this implementation assumes there is no concurrent access
//	to the file system!
//
//	"name" -- name of file to be created
//	"initialSize" -- size of file to be created
//----------------------------------------------------------------------


bool
FileSystem::Create(char *name, int initialSize, bool directoryFlag )
{
    Directory *root;
    PersistentBitmap *freeMap;
    FileHeader *hdr;
    int nowDirectorySector;
    int sector;
    bool success;
    DEBUG(dbgFile, "Creating Directory " << name );

    root = new Directory(NumDirEntries);
    root->FetchFrom(directoryFile);

    char directory[256];
    char fileName[10];
    char temp[256];
    char *cut;
    char target[]= "/";
    int length=0;
    strcpy(temp,name);
    temp[strlen(name)]='\0';
    cut = strtok(temp, target);
    sprintf(fileName,"/%s",cut);
    fileName[strlen(cut)+1]='\0';
    cut = strtok(NULL,target);
    while(cut!=NULL){
        sprintf(fileName,"/%s",cut);
        fileName[strlen(cut)+1]='\0';
        length+=strlen(cut)+1;
        cut = strtok(NULL,target);
    }
    if(length==0)
        length=1;
    strncpy(directory,name,length);
    directory[length]='\0';//find directory/filename
    nowDirectorySector = root->FindFormRoot(directory);
    if(nowDirectorySector >= 0){
        OpenFile *nowDirectoryFile = new OpenFile(nowDirectorySector);
        Directory *nowDirectory = new Directory(NumDirEntries);
        nowDirectory->FetchFrom(nowDirectoryFile);

        if (nowDirectory->Find(fileName) != -1)
            success = FALSE;
        else {
            freeMap = new PersistentBitmap(freeMapFile,NumSectors);
            sector = freeMap->FindAndSet();
            if (sector == -1)
                success = FALSE;		// no free block for file header
            else if (!nowDirectory->Add(fileName, sector,directoryFlag))
                success = FALSE;	// no space in directory
            else {
                hdr = new FileHeader;
                if(directoryFlag)
                    initialSize=DirectoryFileSize;
                if (!hdr->Allocate(freeMap, initialSize))
                        success = FALSE;	// no space on disk for data
                else {
                    success = TRUE;
                // everthing worked, flush all changes back to disk
                    hdr->WriteBack(sector);
                    nowDirectory->WriteBack(nowDirectoryFile);
                    freeMap->WriteBack(freeMapFile);
                }
                delete hdr;
            }
            delete freeMap;
        }
        delete nowDirectoryFile;
        delete nowDirectory;
        if(directoryFlag){
            OpenFile *newDirectoryFile = new OpenFile(sector);
            Directory *newDirectory = new Directory(NumDirEntries);
            newDirectory->WriteBack(newDirectoryFile);

            delete newDirectoryFile;
            delete newDirectory;
        }//initial new directory
    }
    else
        success = FALSE;

    delete root;
    return success;
}
//----------------------------------------------------------------------
// FileSystem::Open
// 	Open a file for reading and writing.
//	To open a file:
//	  Find the location of the file's header, using the directory
//	  Bring the header into memory
//
//	"name" -- the text name of the file to be opened
//----------------------------------------------------------------------

OpenFile *
FileSystem::Open(char *name)
{
    Directory *directory = new Directory(NumDirEntries);
    OpenFile *openFile = NULL;
    int sector;

    DEBUG(dbgFile, "Opening file" << name);
    directory->FetchFrom(directoryFile);
    sector = directory->FindFormRoot(name);
    if (sector >= 0)
        openFile = new OpenFile(sector);	// name was found in directory
    delete directory;
    return openFile;				// return NULL if not found
}

int
FileSystem::Openfile(char *name)
{
    Directory *directory = new Directory(NumDirEntries);
    OpenFile *openFile = NULL;
    int sector,i,id=-1;

    DEBUG(dbgFile, "Opening file" << name);
    directory->FetchFrom(directoryFile);
    sector = directory->FindFormRoot(name);
    if (sector >= 0){
        for(i=1;i<20;i++){
            if(openFileTable[i]==NULL){
                openFile = new OpenFile(sector);	// name was found in directory
                openFileTable[i]=openFile;
                id=i;
                break;
            }
        }
    }
    delete directory;
    if(id>=1&&id<20&&openFileTable[id]!=NULL)
        return i;		// return NULL if not found
    return -1;
}
int
FileSystem::Write(char *buffer, int size, int id)
{
    if(id>=1&&id<20&&openFileTable[id]!=NULL)
        return openFileTable[id]->Write(buffer,size);
    return -1;
}
int
FileSystem::Read(char *buffer, int size, int id)
{
    if(id>=1&&id<20&&openFileTable[id]!=NULL)
        return openFileTable[id]->Read(buffer,size);
    return -1;
}
int
FileSystem::Close(int id)
{
    if(id>=1&&id<20&&openFileTable[id]!=NULL){
        delete openFileTable[id];
        return 1;
    }
    return -1;
}
//----------------------------------------------------------------------
// FileSystem::Remove
// 	Delete a file from the file system.  This requires:
//	    Remove it from the directory
//	    Delete the space for its header
//	    Delete the space for its data blocks
//	    Write changes to directory, bitmap back to disk
//
//	Return TRUE if the file was deleted, FALSE if the file wasn't
//	in the file system.
//
//	"name" -- the text name of the file to be removed
//----------------------------------------------------------------------

bool
FileSystem::Remove(char *name,bool recursiveRemoveFlag)
{
    Directory *root;
    PersistentBitmap *freeMap;
    FileHeader *fileHdr;
    int nowDirectorySector;
    int sector;
    root = new Directory(NumDirEntries);
    root->FetchFrom(directoryFile);

    char directory[256];
    char fileName[10];
    char temp[256];
    char *cut;
    char target[]= "/";
    int length=0;
    strcpy(temp,name);
    temp[strlen(name)]='\0';
    cut = strtok(temp, target);
    sprintf(fileName,"/%s",cut);
    fileName[strlen(cut)+1]='\0';
    cut = strtok(NULL,target);
    while(cut!=NULL){
        sprintf(fileName,"/%s",cut);
        fileName[strlen(cut)+1]='\0';
        length+=strlen(cut)+1;
        cut = strtok(NULL,target);
    }
    if(length==0)
        length=1;
    strncpy(directory,name,length);
    directory[length]='\0';         //find directory/filename
    nowDirectorySector = root->FindFormRoot(directory);

    if (nowDirectorySector == -1) {
        delete root;
        return FALSE;			    // file not found
    }
    OpenFile *nowDirectoryFile = new OpenFile(nowDirectorySector);
    Directory *nowDirectory = new Directory(NumDirEntries);
    nowDirectory->FetchFrom(nowDirectoryFile);
    sector=nowDirectory->Find(fileName);
    int dFlag=nowDirectory->GetFlag(fileName);
    if (sector == -1||(dFlag&&!recursiveRemoveFlag)) {
        delete root;
        delete nowDirectoryFile;
        delete nowDirectory;
        return FALSE;			    // file not found
    }
    if(dFlag&&recursiveRemoveFlag){
        OpenFile *traceDirectoryFile = new OpenFile(sector);
        Directory *traceDirectory = new Directory(NumDirEntries);
        traceDirectory->FetchFrom(traceDirectoryFile);
        for(int i=0;i<NumDirEntries;i++){
            if(traceDirectory->IsUse(i)){
                char temp[256];
                sprintf(temp,"%s%s",name,traceDirectory->GetName(i));
                temp[strlen(name)+strlen(traceDirectory->GetName(i))]='\0';
                Remove(temp,recursiveRemoveFlag);
            }
        }                           //trace  directory entry
        traceDirectory->WriteBack(traceDirectoryFile);
        delete traceDirectoryFile;
        delete traceDirectory;
    }

    fileHdr = new FileHeader;
    fileHdr->FetchFrom(sector);
    freeMap = new PersistentBitmap(freeMapFile,NumSectors);
    fileHdr->Deallocate(freeMap);  		        // remove data blocks
    FileHeader *traceFileHeader = fileHdr;
    int traceFileHeaderSector = sector;
    while(traceFileHeader != NULL){
        freeMap->Clear(traceFileHeaderSector);
        traceFileHeaderSector = traceFileHeader->GetNextFileHeaderSector();
        traceFileHeader = traceFileHeader->GetNextFileHeader();
    }			                                // remove header block
    nowDirectory->Remove(fileName);

    freeMap->WriteBack(freeMapFile);		    // flush to disk
    nowDirectory->WriteBack(nowDirectoryFile);  // flush to disk
    delete fileHdr;
    delete root;
    delete nowDirectoryFile;
    delete nowDirectory;
    delete freeMap;
    return TRUE;
}

//----------------------------------------------------------------------
// FileSystem::List
// 	List all the files in the file system directory.
//----------------------------------------------------------------------

void
FileSystem::List(char* name,bool recursiveListFlag)
{
    int sector;
    Directory *root = new Directory(NumDirEntries);
    root->FetchFrom(directoryFile);
    sector = root->FindFormRoot(name);

    OpenFile *listDirectoryFile = new OpenFile(sector);
    Directory *directory = new Directory(NumDirEntries);
    directory->FetchFrom(listDirectoryFile);
    if(recursiveListFlag)
        directory->ListAll("");
    else
        directory->List();
    delete listDirectoryFile;
    delete directory;
    delete root;
}

//----------------------------------------------------------------------
// FileSystem::Print
// 	Print everything about the file system:
//	  the contents of the bitmap
//	  the contents of the directory
//	  for each file in the directory,
//	      the contents of the file header
//	      the data in the file
//----------------------------------------------------------------------

void
FileSystem::Print()
{
    FileHeader *bitHdr = new FileHeader;
    FileHeader *dirHdr = new FileHeader;
    PersistentBitmap *freeMap = new PersistentBitmap(freeMapFile,NumSectors);
    Directory *directory = new Directory(NumDirEntries);

    printf("Bit map file header:\n");
    bitHdr->FetchFrom(FreeMapSector);
    bitHdr->Print();

    printf("Directory file header:\n");
    dirHdr->FetchFrom(DirectorySector);
    dirHdr->Print();

    freeMap->Print();

    directory->FetchFrom(directoryFile);
    directory->Print();

    delete bitHdr;
    delete dirHdr;
    delete freeMap;
    delete directory;
}

#endif // FILESYS_STUB
