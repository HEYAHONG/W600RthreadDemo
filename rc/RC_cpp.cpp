#include "RC.h"
#include "RC_cpp.h"
#include "RC_internal.h"

std::vector<std::string> RCGetFileList()
{
    std::vector<std::string> ret;

    for(size_t i=0; i<RC_Info_Size; i++)
    {
        const RC_Info_t &rc_info=RC_Info[i];
        ret.push_back(std::string((const char *)&RC_Name[rc_info.name_offset]));
    }

    return ret;
}

static void string_replace(std::string &str,std::string s_old,std::string s_new)
{
    while(str.find(s_old)!=std::string::npos)
    {
        str.replace(str.begin()+str.find(s_old),str.begin()+str.find(s_old)+s_old.length(),s_new);
    }
}

static std::string GetPathByFileName(std::string filename)
{
    //替换\为/
    string_replace(filename,"\\","/");
    //替换//为/
    string_replace(filename,"//","/");
    //删除开头的/
    while(filename.c_str()[0] == '/')
    {
        filename=filename.substr(1);
    }
    return filename;
}

bool RCGetHasFile(std::string filename)
{
    std::string path=GetPathByFileName(filename);
    std::vector<std::string> List=RCGetFileList();
    std::vector<std::string>::iterator it =std::find(List.begin(),List.end(),path);
    return it!=List.end();
}

//通过名称获取文件大小
size_t RCGetFileSize(std::string filename)
{
    return RCGetSize(GetPathByFileName(filename).c_str());
}

//通过名称获取文件指针
const unsigned char * RCGetFileHandle(std::string filename)
{
    return RCGetHandle(GetPathByFileName(filename).c_str());
}
