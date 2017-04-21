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
    if (!_pf_manager) delete _pf_manager;
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
    // go to the last page
    int curPage = fileHandle.getNumberOfPages() - 1;
    if (curPage > -1) {
        // try to insert to the last page
        rid.pageNum = curPage;
        if (insertToPage(fileHandle, recordDescriptor, data, rid) == 0)
            return 0;
        // otherwise look for a page with free space from the beginning
        for (int i = 0; i < curPage; ++i) {
            rid.pageNum = i;
            if (insertToPage(fileHandle, recordDescriptor, data, rid) == 0)
                return 0;
        }
        // no previous page has enough free space
        rid.pageNum = curPage + 1;
        insertToNewPage(fileHandle, recordDescriptor, data, rid);
        return 0;
    } else { // no page yet
        rid.pageNum = 0;
        insertToNewPage(fileHandle, recordDescriptor, data, rid);
        return 0;
    }
}

RC RecordBasedFileManager::insertToPage(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid) {
    unsigned char * pageData = (unsigned char *) malloc(PAGE_SIZE);
    uint16_t dataLen = getRecordLength(recordDescriptor) + ceil((double)recordDescriptor.size() / BYTE_SIZE);
    uint16_t * FS = (uint16_t *) malloc (sizeof(uint16_t));
    uint16_t * N = (uint16_t *) malloc (sizeof(uint16_t));
    fileHandle.readPage(rid.pageNum, pageData); // get the page
    memcpy(FS, pageData + PAGE_SIZE - 2, 2); // get the free space pointer
    memcpy(N, pageData + PAGE_SIZE - 4, 2); // get the number of slots
    // if there are enougth space
    // usage: FS + numOfSlots * slotSize + space for FS and N
    // about to insert: slotSize + dataLen
    // usage + about to insert < Page size
    if (*FS + (*N) * SLOT_SIZE + 4 + SLOT_SIZE + dataLen < PAGE_SIZE) {
        memcpy(pageData + *FS, (char*)data, dataLen); // insert data
        // insert slot
        uint16_t slotData[2] = {*FS, dataLen};
        uint16_t* slot = slotData;
        // get to the end, go backwards by 4 (FS and N), go backwards by N*SLOT_SIZE
        // finally go backwards by 1*SLOT_SIZE
        memcpy(pageData + PAGE_SIZE - 4 - (*N) * SLOT_SIZE - SLOT_SIZE, slot, SLOT_SIZE);
        // update FS
        uint16_t* newFS = new uint16_t(*FS + dataLen);
        memcpy(pageData + PAGE_SIZE - 2, newFS, 2);
        // update N
        uint16_t* newN = new uint16_t(*N + 1);
        memcpy(pageData + PAGE_SIZE - 4, newN, 2);
        fileHandle.writePage(rid.pageNum, pageData);
        // slot number is the last one
        rid.slotNum = *newN - 1;
        free(pageData); free(FS); free(N); delete newFS; delete newN;
        return 0; // success
    } else { // no enough space, fail to insert to page
        free(pageData); free(FS); free(N);
        return -1;
    }

}

RC RecordBasedFileManager::insertToNewPage(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid) {
    unsigned char * pageData = (unsigned char *) malloc(PAGE_SIZE);
    memset(pageData, 0, PAGE_SIZE);
    uint16_t dataLen = getRecordLength(recordDescriptor) + ceil((double)recordDescriptor.size() / BYTE_SIZE);
    memcpy(pageData, (char*)data, dataLen); // insert data
    uint16_t* FS = new uint16_t(dataLen); // FS = dataLen
    memcpy(pageData + PAGE_SIZE - 2, FS, 2); // set FS pointer
    uint16_t* N = new uint16_t(1); // N = 1
    memcpy(pageData + PAGE_SIZE - 4, N, 2); // set N
    uint16_t slotData[2] = {0, dataLen}; // 0th slot with length=dataLen
    uint16_t* slot = slotData;
    memcpy(pageData + PAGE_SIZE - 4 - SLOT_SIZE, slot, SLOT_SIZE); // set 1st slot
    fileHandle.appendPage(pageData);
    rid.slotNum = 0;
    free(pageData); delete FS; delete N;
    return 0;
}

int RecordBasedFileManager::getRecordLength (const vector<Attribute> &recordDescriptor) {
    int length = 0;
    for (size_t i = 0; i < recordDescriptor.size(); ++i)
        switch (recordDescriptor[i].type) {
            case TypeInt:
                length += INT_SIZE;
                break;
            case TypeReal:
                length += REAL_SIZE;
                break;
            case TypeVarChar:
                length = length + VARCHAR_LENGTH_SIZE + recordDescriptor[i].length;
                break;
        }
    return length;
}

RC RecordBasedFileManager::readRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, void *data) {
    if (rid.pageNum >= fileHandle.getNumberOfPages()) {
        cout << "Invalid page number" << endl;
        return -1;
    }
    unsigned char * pageData = (unsigned char *) malloc(PAGE_SIZE);
    fileHandle.readPage(rid.pageNum, pageData);
    uint16_t * N = (uint16_t *) malloc(sizeof(uint16_t));
    memcpy(N, pageData + PAGE_SIZE - 4, 2);
    if (rid.slotNum >= *N) {
        cout << "Invalid slot number" << endl;
        return -1;
    }
    uint16_t * slot = (uint16_t *) malloc(SLOT_SIZE);
    memcpy(slot, pageData + PAGE_SIZE - 4 - SLOT_SIZE * rid.slotNum - SLOT_SIZE, SLOT_SIZE);
    memcpy((char *)data, pageData + slot[0], slot[1]);
    free(pageData); free(N); free(slot);
    return 0;
}

// working
RC RecordBasedFileManager::printRecord(const vector<Attribute> &recordDescriptor, const void *data) {
    int offset = 0;
    size_t numOfFds = recordDescriptor.size();
    int bytes = ceil( (double) numOfFds / BYTE_SIZE );
    unsigned char * nulls = (unsigned char *) malloc(bytes);
    memcpy(nulls, (char*)data + offset, bytes);
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
                    memcpy(intBuf, (char*)data + offset, INT_SIZE);
                    cout << *(int*)((void*)intBuf) <<" ";
                    offset += INT_SIZE;
                    free(intBuf);
                    break;
                }
                case TypeReal:
                {
                    unsigned char * realBuf = (unsigned char *) malloc(REAL_SIZE);
                    memcpy(realBuf, (char*)data + offset, REAL_SIZE);
                    cout << *(float*)((void*)realBuf) <<" ";
                    offset += REAL_SIZE;
                    free(realBuf);
                    break;
                }
                case TypeVarChar:
                {
                    unsigned char * vclenBuf = (unsigned char *) malloc(VARCHAR_LENGTH_SIZE);
                    memcpy(vclenBuf, (char*)data + offset, VARCHAR_LENGTH_SIZE);
                    int vclen = *(int*)((void*)vclenBuf);
                    offset += VARCHAR_LENGTH_SIZE;
                    unsigned char * varchar = (unsigned char *) malloc(vclen + 1);
                    memcpy(varchar, (char*)data + offset, vclen);
                    varchar[vclen] = '\0'; // terminating the char array
                    cout << varchar <<" ";
                    offset += vclen;
                    free(vclenBuf); free(varchar);
                    break;
                }
                default:
                    cout << endl << "Invalid type" << endl;
                    free(nulls);
                    return -1;
            }
    }
    cout << endl;
    free(nulls);
    return 0;
}
