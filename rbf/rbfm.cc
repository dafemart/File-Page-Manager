#include "rbfm.h"
#include "string.h"
#include "math.h"

RecordBasedFileManager* RecordBasedFileManager::_rbf_manager = 0;

RecordBasedFileManager* RecordBasedFileManager::instance()
{
    if(!_rbf_manager)
        _rbf_manager = new RecordBasedFileManager();

    return _rbf_manager;
}

RecordBasedFileManager::RecordBasedFileManager()
{
    pfm = PagedFileManager::instance();
}

RecordBasedFileManager::~RecordBasedFileManager()
{
}

RC RecordBasedFileManager::createFile(const string &fileName) {
    
    return pfm->createFile(fileName);
}

RC RecordBasedFileManager::destroyFile(const string &fileName) {
    return pfm->destroyFile(fileName);
}

RC RecordBasedFileManager::openFile(const string &fileName, FileHandle &fileHandle) {
    return pfm->openFile(fileName, fileHandle);
}

RC RecordBasedFileManager::closeFile(FileHandle &fileHandle) {
    return pfm->closeFile(fileHandle);
}

RC RecordBasedFileManager::insertRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const void *data, RID &rid) {
    // cast data to char pointer so that we can perform pointer arithmetic
    char* _data = (char*) data;
    return -1;
}

RC RecordBasedFileManager::readRecord(FileHandle &fileHandle, const vector<Attribute> &recordDescriptor, const RID &rid, void *data) {
    return -1;
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
                    cout << *(int*)((void*)intBuf) <<" ";
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
