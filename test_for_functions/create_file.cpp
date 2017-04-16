#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#define PAGE_HEADER_OFFSET sizeof("PAGE_HEADER")
#define PAGE_SIZE 10
using namespace std;

int createFile(const string &fileName)
{
    FILE * file_to_create; // Created_by_Daniel --> variable used to create a new file

    file_to_create = fopen(fileName.c_str(),"rb"); // Method used to ensure the specified file doesn't exist
    if(file_to_create == NULL) // If it returns null, then the specified file doesn't exist'
    {
        file_to_create = fopen(fileName.c_str(), "wb"); // create the new file
        if(file_to_create != NULL) // Make sure the file was created
        {
            fputs("PAGE_HEADER",file_to_create);
            printf("file created");
            fclose(file_to_create); // close the file
            return 0;
        }
        else
            printf("can't create the file");
            return -1;
    }
    else{
        printf("the file already exists");
        fclose(file_to_create); // close the file
        return -1;
    }
}

int destroyFile(const string &fileName)
{
    if(remove(fileName.c_str())!=0)
    {
    return -1;
    }
    else{
        return 0;
    }
}

int readPage(int pageNum, void *data, FILE *handleFile)
{
   size_t result;
   char *buffer;
   int pageStart = PAGE_HEADER_OFFSET+PAGE_SIZE*(pageNum-1);
   buffer = (char*)malloc(sizeof(char)*PAGE_SIZE);
   if(pageNum>=1)
   { 
    fseek(handleFile,pageStart,SEEK_SET);
    result = fread(buffer,1,PAGE_SIZE,handleFile);
    *(char **)data = buffer;
    rewind(handleFile);

    return 0;
   }
   return -1;
}

int appendPage(const void *data,int page, FILE* handleFile)
{
    
    size_t result;
    //appendPageCounter++;
    int pageStart = PAGE_SIZE*(page);
    char *buffer = (char *)data;

    fseek(handleFile,pageStart,SEEK_SET);
    if(strlen(buffer)<=PAGE_SIZE)
    {
        result = fwrite(buffer,1,strlen(buffer),handleFile);
        if(result == strlen(buffer))
        {
            return 0;
        }
        else
            return -1;
    }
    return -1;
}


int main(int argc , char** argv)
{
   char* myarr= "7777777777";
   void *ptr = myarr;
   FILE * fp = fopen(argv[1],"rb+");
   if(fp == NULL)
   {
        printf("cant open file");
        return 1;
   }

   appendPage(ptr,3,fp);


}