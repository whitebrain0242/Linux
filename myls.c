#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
// 储存所有选项名称
typedef struct
{
    int show_all;    //-a
    int long_format; //-l
    int show_inode;//-i
    int show_blocks;//-s 磁盘数
} Options;


//文件信息结构提
typedef struct{
    char*name;//文件名字
    char*path;//完整路径很重要
    struct stat st;//文件属性
}FileInfo;

// 解析命令行参数
void parse_options(int argc, char *argv[], Options *opts)
{
    int i, j;
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-')
        { // 不是参数下一个
            continue;
        }
        if (argv[i][1] == '\0')
        { // 只有一个-
            continue;
        }
        for (j = 1; argv[i][j] != '\0'; j++)
        { // 便利这个参数
            switch (argv[i][j])
            {
            case 'a':
                opts->show_all = 1;
                break;
            case 'l':
                opts->long_format = 1;
                break;
            case 'i':
            opts->show_inode=1;
            break;
            case 's':
            opts->show_blocks=1;
            break;
            default:
                fprintf(stderr,
                        "myls:invalid option -- '%c'\n",
                        argv[i][j]);
                break;
            }
        }
    }
}


//获得完整路径：目录名字加上文件名字
char*join_path(const char*dir,const char * name){
    int len;
    char*path;
    len=strlen(dir)+strlen(name)+2;//一个分隔符一个'\0'
    path=malloc(len);
    if(path==NULL){
        return NULL;
    }
    sprintf(path,"%s/%s",dir,name);
    return path;
}

//读取目录内文件信息数组，名字路径属性
FileInfo *read_dir_entries(const char *path,Options *opts,int *count){
    //目录流指针，结构体，文件信息，初始容量，文件数量
    DIR*dir;
    struct dirent*entry;
    FileInfo *files=NULL;
    int capacity=16;
    *count=0;

    dir=opendir(path);
    if(dir==NULL){
        fprintf(stderr,
        "myls:cannot open directory '%s':%s\n",
        path,
    strerror(errno));
    return NULL;
    }

    //先给文件信息分配初始数量
    files=malloc(sizeof(FileInfo)*capacity);
    if(files==NULL){
        closedir(dir);
        return NULL;
    }

    while ((entry = readdir(dir)) != NULL){
        char*fullpath;
        //没有-a跳过
        if (entry->d_name[0] == '.' && !opts->show_all)
        {
            continue; // 因为以.开头是隐藏文件，.代表当前目录
        }
        
        //数组满了扩容
        if(*count>=capacity){
            capacity*=2;
            files=realloc(files,sizeof(FileInfo)*capacity);
            if(files==NULL){
                closedir(dir);
                return NULL;
            }
        }

//想要获得信息，1.先得到完整路径2.获得文件属性3.保存到信息数组里面
//记得文件数量要加一，更新
        //获得完整路径
        fullpath=join_path(path,entry->d_name);
        if(fullpath==NULL){
            continue;
        }

        //获取文件属性
        if(lstat(fullpath,&files[*count].st)==-1){
            fprintf(stderr,"myls: last failed '%s':%s\n",
                fullpath,
            strerror(errno));
            continue;
        }

        //保存文件信息
        files[*count].name=strdup(entry->d_name);
        files[*count].path=fullpath;
        (*count)++;
    }
    closedir(dir);
    return files;
}

//输出文件信息
void print_entries(FileInfo*files,int count,Options*opts){
    //这个函数实现打印inode和块大小,文件名字
    int i;
    for(int i=0;i<count;i++){
        if(opts->show_inode){
            printf("%lu ",files[i].st.st_ino);
        }
//打印的是文件大小，磁盘数量和文件大小不一样
        if(opts->show_blocks){
            printf("ld",(files[i].st.st_blocks+1)/2);
        }

        printf("%s\n",files[i].name);
    }
}

//释放FileInfo数组
void free_entries(FileInfo*files,int count){
    int i;
    for(int i=0;i<count;i++){
        //先释放每一个元素名字和路径，在释放整个数组
        free(files[i].name);
        free(files[i].path);
    }
    free(files);
}


//列出目录
// 参数path目录路径,show_title是否显示目录标题（输入多个目录的是时候
int list_dir(const char *path, int show_title, Options *opts)
{
    FileInfo*files;
    int count;

    if (show_title)
    {
        printf("%s:\n", path);
    }

    //读取目录,得到文件信息，名字完整路径属性
    files=read_dir_entries(path,opts,&count);
    if(files==NULL)return -1;
    
    //输出
    print_entries(files,count,opts);

    //释放
    free_entries(files,count);
    return 0;

}
int main(int argc, char *argv[])
{ // 参数数量和存放进来的目录
    int i;
    int path_count = 0;

    // 创建options结构体，初始化为0
    Options opts = {0};
    // 解析选项,也就是options初始化
    parse_options(argc, argv, &opts);

    if (argc == 1)
    {
        list_dir(".", 0, &opts); // 之输入了myls,默认列出当前路径.
        return 0;
    }

    // 路径数量
    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-')
            path_count++;
    }

    // 没有路径，默认当前目录
    if (path_count == 0)
    {
        list_dir(".", 0, &opts);
        return 0;
    }

    // 依次处理每一个路径，如果路径数量大于1,就显示标题
    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            continue;
        } // 跳过以-开头的选项，输入不完整

        list_dir(argv[i], path_count > 1, &opts);

        if (path_count > 1 && i != argc - 1)
        { // 当前不是最后一个参数
            printf("\n");
        }
    }
    return 0;
}