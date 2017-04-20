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

size_t net_record_length(const vector<Attribute> &recordDescriptor)
{
    size_t net_length = 0;
    for(int i=0; i<recordDescriptor.size();i++)
    {
        net_length=net_length +recordDescriptor[i].length;
    }
    return net_length;
}

RC RecordBasedFileManager::insertRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid) {
    // cast data to char pointer so that we can perform pointer arithmetic
    char* _data = (char*) data;
    size_t numOfFds = recordDescriptor.size();
    uint16_t _dataLen = net_record_length(recordDescriptor)+numOfFds/8;

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
                rid.slotNum=N+1;
                rid.pageNum=itPage;
                success = true;

                free(newFS);
                free(newN);
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
        memcpy(pdptr + PAGE_SIZE - 8, sltptr, 4);
        uint16_t* newFS = new uint16_t(_dataLen);
        memcpy(pdptr + PAGE_SIZE - 2, newFS, 2);
        uint16_t* newN = new uint16_t(1);
        memcpy(pdptr + PAGE_SIZE - 4, newN, 2);
        fileHandle.appendPage(pageData);

        rid.slotNum=0;
        rid.pageNum=curPage+1;

        free(newFS);
        free(newN);
    }
    return 0;
}

RC RecordBasedFileManager::readRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, void *data) {
    
    unsigned pageNum = rid.pageNum;
    uint16_t slotNum = rid.slotNum;
    unsigned char * pageData = (unsigned char *) malloc(PAGE_SIZE);
    if (pageNum >= fileHandle.getNumberOfPages()) {
        cout << "Invalid page number" << endl;
        return -1;
    }
    fileHandle.readPage(pageNum, pageData);
    uint16_t * N = (uint16_t *) malloc(2);
    memcpy(N, pageData + PAGE_SIZE - 4, 2);
    if (slotNum >= *N) {
        cout << "Invalid slot number" << endl;
        return -1;
    }
    uint16_t * slot = (uint16_t *) malloc(4);
    memcpy(slot,pageData + PAGE_SIZE - 4 - SLOT_SIZE * (slotNum+1), SLOT_SIZE);
    memcpy((char *)data, pageData + slot[0], slot[1]);
    
    free(pageData);
    free(slot);
    free(N);
    return 0;
}

// working
RC RecordBasedFileManager::printRecord(const vector<Attribute> &recordDescriptor, const void *data) {
    int offset = 0;
    char* _data = (char*)data;
    size_t numOfFds = recordDescriptor.size();
    int bytes = ceil( (double) numOfFds / BYTE_SIZE );
    unsigned char * nulls = (unsigned char *) malloc(bytes);
    memcpy(nulls, _data + offset, bytes);
    // move the pointer to where the first field starts
    offset += bytes;
    for (size_t i = 0; i < numOfFds; ++i) {
        cout << recordDescriptor[i].name << ": ";
        // null handling
        if (*nulls & (0b01 << (bytes - 1 - i)))
            cout << "NULL ";
        else // non-null handling
            switch (recordDescriptor[i].type) {
                case TypeInt:
                {
                    unsigned char * intBuf = (unsigned char *) malloc(INT_SIZE);
                    memcpy(intBuf, _data + offset, INT_SIZE);
                    cout << *(int*)((void*)intBuf) <<" ";
                    offset += INT_SIZE;

                    free(intBuf);
                    break;
                }
                case TypeReal:
                {
                    unsigned char * realBuf = (unsigned char *) malloc(REAL_SIZE);
                    memcpy(realBuf, _data + offset, REAL_SIZE);
                    cout << *(float*)((void*)realBuf) <<" ";
                    offset += REAL_SIZE;

                    free(realBuf);
                    break;
                }
                case TypeVarChar:
                {
                    unsigned char * vclenBuf = (unsigned char *) malloc(VARCHAR_LENGTH_SIZE);
                    memcpy(vclenBuf, _data + offset, VARCHAR_LENGTH_SIZE);
                    int vclen = *((int*)((void*)vclenBuf));
                    offset += VARCHAR_LENGTH_SIZE;
                    unsigned char * varchar = (unsigned char *) malloc(vclen + 1);
                    memcpy(varchar, _data + offset, vclen);
                    varchar[vclen] = '\0'; // terminating the char 
                    cout << varchar <<" ";
                    offset += vclen;

                    free(varchar);
                    free(vclenBuf);
                    break;
                }
                default:
                    cout << endl << "Invalid type" << endl;
                    return -1;
            }
    }
    cout << endl;
    free(nulls);
    return 0;
}
