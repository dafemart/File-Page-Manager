#include <stdio.h>
#include <string>
using namespace std;

int createFile(const string &fileName)
{
    FILE * file_to_create; // Created_by_Daniel --> variable used to create a new file

    file_to_create = fopen(fileName.c_str(),"r"); // Method used to ensure the specified file doesn't exist
    if(file_to_create == NULL) // If it returns null, then the specified file doesn't exist'
    {
        file_to_create = fopen(fileName.c_str(), "w"); // create the new file
        if(file_to_create != NULL) // Make sure the file was created
        {
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

int main(int argc , char** argv)
{
    //createFile(argv[1]);

    int i = destroyFile(argv[1]);
    
    if(i!=0)
    {
        printf("cant delete file");
    }
    else
    printf("file deleted");

    return 0;
}