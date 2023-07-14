#ifndef RC_CPP_H
#define RC_CPP_H
#ifdef __cplusplus
#include "stdint.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "string"
#include "vector"
#include "algorithm"

//获取RC资源文件列表
std::vector<std::string> RCGetFileList();

//判断RC资源是否具有某文件
bool RCGetHasFile(std::string filename);

//通过名称获取文件大小
size_t RCGetFileSize(std::string filename);

//通过名称获取文件指针
const unsigned char * RCGetFileHandle(std::string filename);

#endif // __cplusplus
#endif // RC_CPP_H
