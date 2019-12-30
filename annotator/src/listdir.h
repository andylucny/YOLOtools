#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

#ifdef _WIN32
#include "dirent.h"
#else
#include <dirent.h>
#include <string.h>
#endif

using namespace std;

bool hasExtension(string name, string ext)
{
    return (name.find(ext, (name.length() - ext.length())) != std::string::npos);
}

void listfiles (string dirName, std::vector<string> &fileNames, bool txt=false) 
{
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(dirName.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name,".") == 0 || strcmp(ent->d_name,"..") == 0) continue;
            string temp_name = ent->d_name;
            switch (ent->d_type) {
            case DT_REG:
            case DT_LNK:
                if ((!txt && (hasExtension(temp_name,".jpg") || hasExtension(temp_name,".png"))) ||
                    (txt && hasExtension(temp_name,".txt")))
                    fileNames.push_back(temp_name);
                break;
            }
        }
        closedir(dir);
    }
}
 
void listdirs (string dirName, std::vector<string> &folderNames, std::vector<std::vector<string>> &fileNames, bool txt=false) 
{
  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir(dirName.c_str())) != NULL) {
    while ((ent = readdir(dir)) != NULL) {
      if (strcmp(ent->d_name,".") == 0 || strcmp(ent->d_name,"..") == 0) continue;
      string temp_name = ent->d_name;
      switch (ent->d_type) {
        case DT_DIR:
          folderNames.push_back(temp_name);
          break;
      }
    }
    closedir(dir);
    std::sort(folderNames.begin(),folderNames.end());
    for (size_t i = 0; i < folderNames.size(); i++) {
        fileNames.push_back(std::vector<string>());
        listfiles(dirName + "/" + folderNames[i],fileNames.back(),txt);
    }
  }
}

void printdirs (std::vector<string> &folderNames, std::vector<std::vector<string>> &fileNames)
{
    for (size_t i=0; i<folderNames.size(); i++) {
        cout << folderNames[i] << ":" << endl;
        for (size_t j=0; j<fileNames[i].size(); j++) {
            cout << "  " << fileNames[i][j] << endl;
        }
    }
}

void printdir (std::vector<string> &fileNames)
{
    for (size_t j=0; j<fileNames.size(); j++) {
        cout << "  " << fileNames[j] << endl;
    }
}

string getAnnotationFilename (string imageFilename) 
{
    return imageFilename.substr(0, imageFilename.rfind(".")) + ".txt";
}