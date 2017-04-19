#include "rbfm.h"
#include "string.h"
#include "math.h"
#include "stdint.h"

RecordBasedFileManager* RecordBasedFileManager::_rbf_manager = 0;

RecordBasedFileManager* RecordBasedFileManager::instance()
{
    if(!_rbf_manager)
        _rbf_manager = new RecordBasedFileManager();

    return _rbf_manager;
}

RecordBasedFileManager::RecordBasedFileManager()
{
    _pf_manager = PagedFileManager::instance();
}

RecordBasedFileManager::~RecordBasedFileManager()
{
}

RC RecordBasedFileManager::createFile(const string &fileName) {
    
    return _pf_manager->createFile(fileName);
}

RC RecordBasedFileManager::destroyFile(const string &fileName) {
    return _pf_manager->destroyFile(fileName);
}

RC RecordBasedFileManager::openFile(const string &fileName, FileHandle &fileHandle) {
    return _pf_manager->openFile(fileName, fileHandle);
}

RC RecordBasedFileManager::closeFile(FileHandle &fileHandle) {
    return _pf_manager->closeFile(fileHandle);;
}

RC RecordBasedFileManager::insertRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid) {
    // cast data to char pointer so that we can perform pointer arithmetic
    char* _data = (char*) data;
    uint16_t _dataLen = strlen(_data);
    size_t numOfFds = recordDescriptor.size();
    // set the curPage to the last page
    int curPage = fileHandle.getNumberOfPages() - 1;
    char bufFS[2]; // 2-byte int for free space pointer
    char bufN[2]; // 2-byte int for number of slots/records
    char pageData[PAGE_SIZE]; // page data of 4096 bytes
    char* pdptr = pageData;
    bool success = false;
    if (curPage > -1) {
        int itPage = curPage;
        bool inLastPage = true;
        do {
            fileHandle.readPage(itPage, pageData);
            memcpy(bufFS, pdptr + PAGE_SIZE - 2, 2); // get the free space pointer
            memcpy(bufN, pdptr + PAGE_SIZE - 4, 2); // get the number of slots
            uint16_t FS = *(uint16_t*)((void*)bufFS); // get the free space pointer in int format
            uint16_t N = *(uint16_t*)((void*)bufN); // get the number of slots in int format
            if (FS + N * SLOT_SIZE + SLOT_SIZE + _dataLen < PAGE_SIZE) {
                // insert
                memcpy(pdptr + FS, _data, _dataLen); // insert data at FS
                // insert a new slot
                uint16_t slot[2] = {FS, _dataLen}; 
                uint16_t* sltptr = slot;
                memcpy(pdptr + PAGE_SIZE - 8 - N * SLOT_SIZE, sltptr, 4);
                // update FS
                uint16_t* newFS = new uint16_t(FS + _dataLen);
                memcpy(pdptr + PAGE_SIZE - 2, newFS, 2);
                // update N
                uint16_t* newN = new uint16_t(N + 1);
                memcpy(pdptr + PAGE_SIZE - 4, newN, 2);
                // finally write page to disk
                fileHandle.writePage(itPage, pageData);
                success = true;
                break;
            } else {
                if (inLastPage) {
                    itPage = 0;
                    inLastPage = false;
                }
            }
        } while (itPage < curPage);
    }
    // need to append page
    if (!success) {
        memcpy(pdptr, _data, _dataLen);
        uint16_t slot[2] = {0, _dataLen};
        uint16_t* sltptr = slot;
        memcpy(pdptr + PAGE_SIZE - 6, sltptr, 2);
        uint16_t* newFS = new uint16_t(_dataLen);
        memcpy(pdptr + PAGE_SIZE - 2, newFS, 2);
        uint16_t* newN = new uint16_t(1);
        memcpy(pdptr + PAGE_SIZE - 4, newN, 2);
        fileHandle.appendPage(pageData);
    }
    return 0;
}

RC RecordBasedFileManager::readRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, void *data) {
    return 0;
}

RC RecordBasedFileManager::printRecord(const vector<Attribute> &recordDescriptor, const void *data) {
    // cast data to char pointer so that we can perform pointer arithmetic
    char* _data = (char*)data;
    // number of fields
    size_t numOfFds = recordDescriptor.size();
    // number of bytes for null indicator
    size_t bytes = (size_t) ceil( (float) numOfFds / BYTE_SIZE );
    // get null indicator bits
    char nulls[bytes];
    memcpy(nulls, _data, bytes);
    // move the pointer to where the first field starts
    _data += bytes;
    for (size_t i = 0; i < numOfFds; ++i) {
        cout << recordDescriptor[i].name << ": ";
        // null handling
        if (*nulls & (0b01 << (bytes - 1 - i)))
            cout << "NULL ";
        else // non-null handling
            switch (recordDescriptor[i].type) {
                case TypeInt:
                {
                    char intBuf[INT_SIZE]; 
                    // void* memcpy( void* dest, const void* src, std::size_t count );
                    memcpy(intBuf, _data, INT_SIZE);
                    // how to convert intBuf to integer?
                    cout << *(int*)((void*)intBuf) <<" ";
                    // move the pointer to next chunk
                    _data += INT_SIZE;
                    break;
                }
                case TypeReal:
                {
                    char realBuf[REAL_SIZE];
                    memcpy(realBuf, _data, REAL_SIZE);
                    cout << *(int*)((void*)realBuf) <<" ";
                    _data += REAL_SIZE;
                    // how to convert intBuf to real?
                    cout << *(float*)((void*)realBuf) <<" ";
                    // move the pointer to next chunk
                    _data += INT_SIZE;
                    break;
                }
                case TypeVarChar:
                {
                    char vclenBuf[VARCHAR_LENGTH_SIZE];
                    memcpy(vclenBuf, _data, VARCHAR_LENGTH_SIZE);
                    // how to convert vclenBuf to VARCHAR length?
                    int vclen = *(int*)((void*)vclenBuf);
                    // move the pointer
                    _data += VARCHAR_LENGTH_SIZE;
                    // copy out the chars
                    char varchar[vclen];
                    memcpy(varchar, _data, vclen);
                    cout << varchar <<" ";
                    // move the pointer to next chunk
                    _data += vclen;
                    break;
                }
                default:
                    cout << endl << "Invalid type" << endl;
                    return -1;
            }
        cout << "endl";
    }
    return 0;
}
