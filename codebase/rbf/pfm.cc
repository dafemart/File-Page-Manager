#include "pfm.h"

PagedFileManager* PagedFileManager::_pf_manager = 0;

PagedFileManager* PagedFileManager::instance()
{
    if(!_pf_manager)
        _pf_manager = new PagedFileManager();

    return _pf_manager;
}


PagedFileManager::PagedFileManager()
{
}


PagedFileManager::~PagedFileManager()
{
}


RC PagedFileManager::createFile(const string &fileName)
{
    FILE * file_to_create; // Created_by_Daniel --> variable used to create a new file

    file_to_create = fopen(fileName.c_str(),"r"); // Method used to ensure the specified file doesn't exist
    if(file_to_create == NULL) // If it returns null, then the specified file doesn't exist'
    {
        file_to_create = fopen(fileName.c_str(), "w"); // create the new file
        if(file_to_create != NULL) // Make sure the file was created
        {
            //printf("file created");
            fclose(file_to_create); // close the file
            return 0;
        }
        else
           // printf("can't create the file");
            return -1;
    }
    else{
        //printf("the file already exists");
        fclose(file_to_create); // close the file
        return -1;
    }
}


RC PagedFileManager::destroyFile(const string &fileName)
{
    return -1;
}


RC PagedFileManager::openFile(const string &fileName, FileHandle &fileHandle)
{
    return -1;
}


RC PagedFileManager::closeFile(FileHandle &fileHandle)
{
    return -1;
}


FileHandle::FileHandle()
{
    readPageCounter = 0;
    writePageCounter = 0;
    appendPageCounter = 0;
}


FileHandle::~FileHandle()
{
}


RC FileHandle::readPage(PageNum pageNum, void *data)
{
    return -1;
}


RC FileHandle::writePage(PageNum pageNum, const void *data)
{
    return -1;
}


RC FileHandle::appendPage(const void *data)
{
    return -1;
}


unsigned FileHandle::getNumberOfPages()
{
    return -1;
}


RC FileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount)
{
    return -1;
}
