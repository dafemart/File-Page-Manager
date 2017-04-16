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
    FILE *file_to_create; //  variable used to create a new file

    file_to_create = fopen(fileName.c_str(),"rb"); // Method used to ensure the specified file doesn't exist
    if(file_to_create == NULL) // If it returns null, then the specified file doesn't exist'
    {
        file_to_create = fopen(fileName.c_str(), "wb"); // create the new file
        if(file_to_create != NULL) // Make sure the file was created
        {
            fputs("PAGE_HEADER",file_to_create);
            fclose(file_to_create); // close the file
            return 0;
        }
        else
           
            return -1;
    }
    else{
        fclose(file_to_create); // close the file
        return -1;
    }
}


RC PagedFileManager::destroyFile(const string &fileName)
{
    if(remove(fileName.c_str())!=0)
    {
    return -1;
    }
    else{
        return 0;
    }
}


RC PagedFileManager::openFile(const string &fileName, FileHandle &fileHandle)
{
    if(fileHandle.handleFile==NULL) //makesure handlefile doesn't point to another file
    {
        FILE *file_to_open = fopen(fileName.c_str(),"rb+");  //open file for reading and writing
        if(file_to_open != NULL) // if the file was opened
            {
                fileHandle.handleFile=file_to_open;
                return 0;
            }
    }
    return -1;
}


RC PagedFileManager::closeFile(FileHandle &fileHandle)
{
    if(fclose(fileHandle.handleFile)==0) // close the file and flush the data
    {
        fileHandle.handleFile==NULL; // set handlefile for reuse purposes
        return 0;
    }
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
   size_t result; //variable used for the return value of the function fread
   char *buffer; // buffer to data read
   int pageStart = PAGE_SIZE*(pageNum); // place where we start reading
   buffer = (char*)malloc(sizeof(char)*PAGE_SIZE); //buffer to store what is read
   if(pageNum>=0) //it can't be less than 0
   { 
    fseek(handleFile,pageStart,SEEK_SET); // place the file cursor in the right starting place to read
    result = fread(buffer,1,PAGE_SIZE,handleFile); // read the data and store it in a buffer
    *(char **)data = buffer; // make the pointer data points to what buffer is poiting to
    rewind(handleFile); // rewind the cursor for further readings
    readPageCounter++;  // increment readpagecounter

    return 0;
   }
   return -1;
}


RC FileHandle::writePage(PageNum pageNum, const void *data)
{
    size_t result;
    int pageStart = PAGE_SIZE*(pageNum);
    char *Buffer = (char *)data;
    //not done yet
    return -1;
}


RC FileHandle::appendPage(const void *data)
{
    
    size_t result;  
    appendPageCounter++;
    int pageStart = PAGE_SIZE*(appendPageCounter);
    char *buffer = (char *)data;

    fseek(handleFile,pageStart,SEEK_SET);
    if(strlen(buffer)<=PAGE_SIZE) // the size of the data can't be greater than page_size
    {
        result = fwrite(buffer,1,strlen(buffer),handleFile); //write the data to the page
        rewind(handleFile); // rewind for further readings
        if(result == strlen(buffer))
        {
            return 0;
        }
        else
            return -1;
    }
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
