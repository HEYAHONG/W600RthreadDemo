#include "fstream"
#include "dirent.h"
#include "unistd.h"
#include "sys/stat.h"
#include  <string>
#include <iostream>
#include <sstream>
#include <functional>
#include  <vector>
#include <map>
#include <stack>
#include "RC_internal.h"
#include "stdlib.h"
#include "stdint.h"

std::vector<std::string> filelist;
void listdir(std::string dirname,std::string root)
{
#ifdef WIN32
#endif // WIN32
    DIR *dir=NULL;
    if(dirname.empty())
    {
        dir=opendir(root.c_str());
    }
    else
    {
        dir=opendir((root+dirname).c_str());
    }
    if(dir!=NULL)
    {
        struct dirent * dirnode=NULL;
        while((dirnode=readdir(dir)))
        {
            std::string name=dirname+std::string(dirnode->d_name);
            struct stat state= {0};
            if(0!=stat((root+name).c_str(),&state))
            {
                continue;
            }
            if(state.st_mode & S_IFREG)
            {
                if(name.empty())
                {
                    continue;
                }
                filelist.push_back(name);
                printf("file: %s\n",name.c_str());
            }
            if(state.st_mode & S_IFDIR)
            {
                if(std::string(dirnode->d_name)=="..")
                {
                    //不向上遍历
                    continue;
                }
                if(std::string(dirnode->d_name)==".")
                {
                    //不遍历本目录
                    continue;
                }
                listdir(name+"/",root);
            }
        }
        closedir(dir);
    }

}

std::vector<RC_Info_t> RC_Info_List;
std::map<std::string,RC_Info_t> Path_RC_Info_List;
void fsgen(std::string filename,std::string root)
{
    std::fstream RC_FS;
    RC_FS.open(filename.c_str(),std::ios_base::out );
    if(RC_FS.is_open())
    {
        RC_FS.clear();

        {
            //包含头文件
            RC_FS<<"#include \"RC_internal.h\"\n";
        }

        //生成RC代码
        {
            size_t current_rc_data=0;
            size_t current_rc_name=0;
            RC_Info_t RC_Info_item= {0};
            {
                //RC_Data
                RC_FS << "\nconst unsigned char RC_Data[] =\n";
                RC_FS << "{\n";
            }
            for(auto it=filelist.begin(); it != filelist.end(); it++)
            {
                if(it==filelist.end())
                {
                    break;
                }
                std::string filename=(*it);
                std::fstream File;
                File.open((root+filename).c_str(),std::ios_base::in|std::ios_base::binary);
                if(File.is_open())
                {
                    RC_Info_item.data_offset=current_rc_data;
                    RC_Info_item.name_offset=current_rc_name;
                    {
                        RC_Info_item.name_size=(filename.length()+1);
                        current_rc_name+=RC_Info_item.name_size;
                    }
                    {
                        RC_FS << "//" << filename.c_str() << "\n";
                    }
                    while(!File.eof())
                    {
                        //写RC_Data
                        unsigned char buff[64]= {0};
                        size_t readbytes=File.readsome((char *)buff,sizeof(buff));
                        if(readbytes==0)
                        {
                            break;
                        }
                        {
                            for(size_t i=0; i<readbytes; i++)
                            {
                                current_rc_data++;
                                char temp[10]= {0};
                                sprintf(temp,"0x%02X,",(uint32_t)buff[i]);
                                RC_FS<< temp;
                            }
                        }
                        RC_FS << "\n";
                    }


                    {
                        RC_Info_item.data_size=current_rc_data-RC_Info_item.data_offset;
                        RC_Info_List.push_back(RC_Info_item);
                        Path_RC_Info_List[filename]=RC_Info_item;
                    }

                    {
                        current_rc_data++;
                        RC_FS << "0x00,\n";
                    }

                    File.close();
                }
            }
            {
                RC_FS << "0x00\n};\n";
            }

            {

                RC_FS << "\nconst unsigned char RC_Name[] =\n";
                RC_FS << "{\n";
            }
            for(auto it=filelist.begin(); it!=filelist.end(); it++)
            {
                if(it==filelist.end())
                {
                    break;
                }
                std::string filename=(*it);
                if(access((root+filename).c_str(),R_OK)==0)
                {
                    {
                        RC_FS << "//" <<  filename.c_str() << "\n";
                    }
                    unsigned char * str=(unsigned char *) filename.c_str();
                    for(size_t i=0; i< filename.length(); i++)
                    {

                        current_rc_data++;
                        char temp[10]= {0};
                        sprintf(temp,"0x%02X,",(int)str[i]);
                        RC_FS<< temp;

                    }
                    {
                        RC_FS << "0x00,\n";
                    }
                }
            }
            {
                RC_FS << "0x00\n};\n";
            }

            {
                RC_FS << "\nconst RC_Info_t RC_Info[] =\n";
                RC_FS << "{\n";
            }
            for(auto it=RC_Info_List.begin(); it!=RC_Info_List.end(); it++)
            {
                char temp[256]= {0};
                sprintf(temp,"{%lu,%lu,%lu,%lu}\n",(*it).data_offset,(*it).data_size,(*it).name_offset,(*it).name_size);
                RC_FS<< temp;
                if((it+1)!=RC_Info_List.end())
                {
                    RC_FS << ",";
                }
                {
                    RC_FS << "\n";
                }
            }
            {
                RC_FS << "};\n";
            }

            {
                char temp[256]= {0};
                sprintf(temp,"\nconst size_t    RC_Info_Size= %lu;\n",RC_Info_List.size());
                RC_FS << temp;
            }
        }

        //生成rtthread romfs相关代码
        {

            {
                RC_FS << "\n#ifdef RC_USING_DFS_ROMFS\n";
            }

            {
                RC_FS << "\n#include <rtthread.h>\n";
            }

            {
                RC_FS << "\n#include <dfs_romfs.h>\n";
            }

            {
                std::stack<std::string> dirent_list_stack;

                //根节点
                dirent_list_stack.push("\
\nconst struct romfs_dirent romfs_root =\n\
{\n\
    ROMFS_DIRENT_DIR, \"/\", (rt_uint8_t *)romfs_root_node, sizeof(romfs_root_node) / sizeof(romfs_root_node[0])\n\
};\n");


                //遍历目录
                std::function<void(std::string,std::vector<std::string>)> gen_dirent_list=[&](std::string root,std::vector<std::string> fs_dir)
                {
                    std::stringstream code;

                    code << "\nconst struct romfs_dirent romfs_root_node";
                    for(size_t i=0; i<fs_dir.size(); i++)
                    {
                        code << "_" <<fs_dir[i];
                    }
                    code << "[]=\n{\n";
                    std::string dirent_name;
                    {
                        dirent_name="romfs_root_node";
                        for(size_t i=0; i<fs_dir.size(); i++)
                        {
                            dirent_name+="_";
                            dirent_name+=fs_dir[i];
                        }

                    }
                    DIR *dir=opendir(root.c_str());
                    std::vector<std::string> dir_list;
                    if(dir!=NULL)
                    {
                        struct dirent * dirnode=NULL;
                        size_t dirnode_count=0;
                        while((dirnode=readdir(dir)))
                        {
                            std::string name=std::string(dirnode->d_name);
                            struct stat state= {0};
                            if(0!=stat((root+name).c_str(),&state))
                            {
                                continue;
                            }
                            if(state.st_mode & S_IFREG)
                            {
                                if(name.empty())
                                {
                                    continue;
                                }
                                {
                                    std::string fs_filename;
                                    if(fs_dir.size()==0)
                                    {
                                        fs_filename=dirnode->d_name;
                                    }
                                    else
                                    {
                                        for(size_t i=0; i<fs_dir.size(); i++)
                                        {
                                            fs_filename+=fs_dir[i];
                                            fs_filename+="/";
                                        }
                                        fs_filename+=dirnode->d_name;
                                    }
                                    if(Path_RC_Info_List.find(fs_filename)!=Path_RC_Info_List.end())
                                    {
                                        RC_Info_t &info = Path_RC_Info_List[fs_filename];
                                        {
                                            if(dirnode_count>0)
                                            {
                                                code << ",\n";
                                            }
                                            char buff[8192];
                                            sprintf(buff,"{ROMFS_DIRENT_FILE, \"%s\", (rt_uint8_t *)&RC_Data[%u],%u}\n",dirnode->d_name,(unsigned)info.data_offset,(unsigned)info.data_size);
                                            code<<buff;
                                            dirnode_count++;
                                        }
                                    }
                                }
                            }
                            if(state.st_mode & S_IFDIR)
                            {
                                if(std::string(dirnode->d_name)=="..")
                                {
                                    //不向上遍历
                                    continue;
                                }
                                if(std::string(dirnode->d_name)==".")
                                {
                                    //不遍历本目录
                                    continue;
                                }

                                {
                                    if(dirnode_count>0)
                                    {
                                        code << ",\n";
                                    }
                                    std::string new_dirent_name=dirent_name+"_"+dirnode->d_name;
                                    char buff[8192];
                                    sprintf(buff,"{ROMFS_DIRENT_DIR, \"%s\", (rt_uint8_t *)%s, sizeof(%s)/sizeof(%s[0])}",dirnode->d_name,new_dirent_name.c_str(),new_dirent_name.c_str(),new_dirent_name.c_str());
                                    code<<buff;
                                    dirnode_count++;
                                }
                                dir_list.push_back(dirnode->d_name);

                            }
                        }
                        closedir(dir);
                    }
                    code << "\n};\n";
                    dirent_list_stack.push(std::string(code.str()));

                    for(size_t i=0; i<dir_list.size(); i++)
                    {
                        std::vector<std::string> new_fs_dir=fs_dir;
                        new_fs_dir.push_back(dir_list[i]);
                        gen_dirent_list(root+dir_list[i]+"/",new_fs_dir);
                    }

                };

                gen_dirent_list(root,std::vector<std::string>());

                while(dirent_list_stack.size()>0)
                {
                    RC_FS << dirent_list_stack.top();
                    dirent_list_stack.pop();
                }
            }

            {
                RC_FS << "\n#endif\n";
            }
        }


        RC_FS.close();
    }
}

int main(int argc,char *argv[])
{
    if(argc<3)
    {
        return -1;
    }

    setbuf(stdout,NULL);

    {
        printf("root:%s\nRC_fs.c:%s\n",argv[1],argv[2]);
    }

    listdir("",std::string(argv[1])+"/");

    fsgen(argv[2],std::string(argv[1])+"/");

    return 0;


}


