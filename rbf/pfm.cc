#include "pfm.h"
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <vector>

extern vector<size_t> PageCurSizes;

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
    if(!_pf_manager) delete _pf_manager;
}

// helper copied from rbftest.cc
bool PagedFileManager::fileExists(const string &fileName)
{
    struct stat stFileInfo;

    if (stat(fileName.c_str(), &stFileInfo) == 0)
        return true;
    else
        return false;
}


RC PagedFileManager::createFile(const string &fileName)
{
    // If file already exists, error
    if (fileExists(fileName)) return -1;
    // create the file
    FILE* pagefile = fopen(fileName.c_str(), "w");
    if (fclose(pagefile)) return -1;
    return 0; // on success
}


RC PagedFileManager::destroyFile(const string &fileName)
{
    // delete the file
    return remove(fileName.c_str());
}


RC PagedFileManager::openFile(const string &fileName, FileHandle &fileHandle)
{
    // If file not exists, error.
    if (!fileExists(fileName)) return -1;
    // if the handle has file already, error out
    if (fileHandle.getFile()) return -1;
    // open file
    FILE* pagefile = fopen(fileName.c_str(), "r+");
    if (!pagefile) return -1;
    
    // attach the file to the handle
    fileHandle.setFile(pagefile);
    return 0;
}


RC PagedFileManager::closeFile(FileHandle &fileHandle)
{
    FILE* pagefile = fileHandle.getFile();
    if (!pagefile) return -1;
    fflush(pagefile);
    fileHandle.setFile(nullptr); // leak??
    return fclose(pagefile);
}


FileHandle::FileHandle()
{
    readPageCounter = 0;
    writePageCounter = 0;
    appendPageCounter = 0;
    _file = nullptr;
}


FileHandle::~FileHandle()
{
}


RC FileHandle::readPage(PageNum pageNum, void *data)
{
    if (pageNum > getNumberOfPages()) return -1;
    // find the start of the page
    if (fseek(_file,
            PAGE_SIZE * pageNum,
            SEEK_SET))
        return -1;
    if (fread(data, 1, PAGE_SIZE, _file) != PAGE_SIZE)
        return -1;
    ++readPageCounter;
    return 0;
}


RC FileHandle::writePage(PageNum pageNum, const void *data)
{
    if (pageNum > getNumberOfPages()) return -1;
    if (fseek(_file,
            + PAGE_SIZE * pageNum,
            SEEK_SET))
        return -1;
    if (fwrite(data, 1, PAGE_SIZE, _file) != PAGE_SIZE)
        return -1;
    fflush(_file);
    ++writePageCounter;
    return 0;
}


RC FileHandle::appendPage(const void *data)
{
    if(fseek(_file, 0, SEEK_END)) return -1;
    if (fwrite(data, 1, PAGE_SIZE, _file) != PAGE_SIZE) return -1;
    fflush(_file);
    ++appendPageCounter;
    return 0;
}


unsigned FileHandle::getNumberOfPages()
{
    // numOfPages = (fileSize - headerSize) / pageSize
    fseek(_file, 0, SEEK_END);
    auto fileSize = ftell(_file);
    return (fileSize / PAGE_SIZE);
}


RC FileHandle::collectCounterValues(unsigned &readPageCount, unsigned &writePageCount, unsigned &appendPageCount)
{
    readPageCount = readPageCounter;
    writePageCount = writePageCounter;
    appendPageCount = appendPageCounter;
    return 0;
}

// getter & setter
FILE* FileHandle::getFile()
{
    return _file;
}

void FileHandle::setFile(FILE* file) 
{
    _file = file;
}