#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <unistd.h>
#include <limits.h>

#include <pwd.h>
#include <grp.h>

#include <time.h>
#include <linux/limits.h>
// 储存所有选项名称
typedef struct
{
    int show_all;    //-a
    int long_format; //-l

    int show_inode;  //-i
    int show_blocks; //-s 磁盘数

    int recursive;//-R递归
    int sort_time;   // t,按照时间修改顺序 新-旧
    int reverse;     //-r, 反转排序
} Options;

// 文件信息结构提
typedef struct
{
    char *name;     // 文件名字
    char *path;     // 完整路径很重要
    struct stat st; // 文件属性
} FileInfo;

//看看需要实现哪些功能？？参数解析
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
                opts->show_inode = 1;
                break;
            case 's':
                opts->show_blocks = 1;
                break;
            case 't':
                opts->sort_time = 1;
                break;
            case 'r':
                opts->reverse = 1;
                break;
            case 'R':
            opts ->recursive=1;
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

// 获得完整路径：目录名字加上文件名字
char *join_path(const char *dir, const char *name)
{
    int len;
    char *path;
    len = strlen(dir) + strlen(name) + 2; // 一个分隔符一个'\0'
    path = malloc(len);
    if (path == NULL)
    {
        return NULL;
    }
    sprintf(path, "%s/%s", dir, name);
    return path;
}

//mode，创建mode权限数组
void mode_to_string(mode_t mode,char*str){
    if(S_ISREG(mode))//是否是普通文件？
    str[0]='-';
    else if(S_ISDIR(mode))//目录u
    str[0]='d';
    else if(S_ISLNK(mode))//链接
str[0] = 'l';
    else if(S_ISCHR(mode))//字符设备：键盘，显示器，鼠标
    str[0] = 'c';
    else if(S_ISBLK(mode))//块设备（硬盘，u盘
        str[0] = 'b';
    else if(S_ISFIFO(mode))//管道文件
        str[0] = 'p';
    else if(S_ISSOCK(mode))//是不是套接字文件
        str[0] = 's';
    else
        str[0] = '?';

    //文件所有者
    str[1] = (mode & S_IRUSR) ? 'r' : '-';//读
    str[2] = (mode & S_IWUSR) ? 'w' : '-';//写
    str[3] = (mode & S_IXUSR) ? 'x' : '-';//执行
    //组成员

    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';
    //其他用户

    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';

    str[10] = '\0';
}

//时间格式化:也就是把时间戳变成年月日
//传入时间戳
void format_time(time_t t,char *buf,int size){
    struct tm* tm_info;//存储年月日是分秒
    tm_info=localtime(&t);//把时间戳转变年月日是分
    strftime(buf,size,"%Y-%m-%d %H:%M",tm_info);//格式化称字符串
}

// 读取目录内文件信息数组，名字路径属性
FileInfo *read_dir_entries(const char *path, Options *opts, int *count)
{
    // 目录流指针，结构体，文件信息，初始容量，文件数量
    DIR *dir;
    struct dirent *entry;
    FileInfo *files = NULL;
    int capacity = 16;
    *count = 0;

    dir = opendir(path);
    if (dir == NULL)
    {
        fprintf(stderr,
                "myls:cannot open directory '%s':%s\n",
                path,
                strerror(errno));
        return NULL;
    }

    // 先给文件信息分配初始数量
    files = malloc(sizeof(FileInfo) * capacity);
    if (files == NULL)
    {
        closedir(dir);
        return NULL;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        char *fullpath;
        // 没有-a跳过
        if (entry->d_name[0] == '.' && !opts->show_all)
        {
            continue; // 因为以.开头是隐藏文件，.代表当前目录
        }

        // 数组满了扩容
        if (*count >= capacity)
        {
            capacity *= 2;
            FileInfo*tmp;
            tmp = realloc(files, sizeof(FileInfo) * capacity);
            if (tmp == NULL)
            {
                free(files);
                closedir(dir);
                return NULL;
            }
            files=tmp;
        }

        // 想要获得信息，1.先得到完整路径2.获得文件属性3.保存到信息数组里面
        // 记得文件数量要加一，更新
        // 获得完整路径
        fullpath = join_path(path, entry->d_name);
        if (fullpath == NULL)
        {
            continue;
        }

        // 获取文件属性
        if (lstat(fullpath, &files[*count].st) == -1)
        {
            fprintf(stderr,
                    "myls: lstat failed '%s':%s\n",
                    fullpath,
                    strerror(errno));
            free(fullpath);
            continue;
        }

        // 保存文件信息
        files[*count].name = strdup(entry->d_name);
        files[*count].path = fullpath;
        (*count)++;
    }
    closedir(dir);
    return files;
}


/*
-t实现
*/
//按照文件名字排序，穿进去的是两个文件
int compare_name(const void *a,const void *b){
    const FileInfo*fa=a;
    const FileInfo*fb=b;

    return strcmp(fa->name,fb->name);//按照字母从小到大排序，ls默认排序
}


//为-t服务，按照修改时间排序
int compare_time(const void *a,const void *b){
const FileInfo*fa=a;
const FileInfo*fb=b;
//st_time是文件最后修改时间，数字越大，文件月薪
if(fb->st.st_mtime > fa->st.st_mtime){
return 1;
}else{
    return -1;
}
return strcmp(fa->name,fb->name);//时间一样就按照名字排序
}

//排序
void  sort_entries(FileInfo*files,int count,Options*opts){
    if(opts->sort_time){//有-t就按照时间顺序排序
qsort(files,count,sizeof(FileInfo),compare_time);
    }else{
        qsort(files,//没有就按照名字排序
              count,
              sizeof(FileInfo),
              compare_name);
    }
}

//长格式输出
void print_long(FileInfo *file,Options*opts){
    char mode_str[11];
    struct passwd*pw;//储存用户信息，用户名UID等
    struct group *gr;
    char time_buf[64];//存放格式化后的时间

    //权限便成为字符串
    mode_to_string(file->st.st_mode,mode_str);
    //时间戳格式化
    format_time(file->st.st_mtime,time_buf,sizeof(time_buf));
    //把iD数字变成用户名
    pw=getpwuid(file->st.st_uid);
    //组ID变成组名
    gr=getgrgid(file->st.st_gid);


    //顺序打印inode,blocks,mode,应连接数量,用户名小组名，文件大小，时间
    if (opts->show_inode)
        {
            printf("%-8lu ", file->st.st_ino);
        }
        // 打印的是文件大小，磁盘数量和文件大小不一样
        if (opts->show_blocks)
        {
            printf("%-4ld ",
                   (file->st.st_blocks + 1) / 2);
        }
        printf("%s ",
           mode_str);

    printf("%-3lu ",
           file->st.st_nlink);

    if(pw)
        printf("%-8s ",
               pw->pw_name);
    else
        printf("%-8d ",
               file->st.st_uid);

    if(gr)
        printf("%-8s ",
               gr->gr_name);
    else
        printf("%-8d ",
               file->st.st_gid);

    printf("%8ld ",
           file->st.st_size);

    printf("%s ",
           time_buf);


           //软连接显示：如果文件是软链接，就在文件名后面打印：-> 指向的文件
    if(S_ISLNK(file->st.st_mode)){
        char target[PATH_MAX];
        ssize_t len;
        len=readlink(file->path,target,sizeof(target)-1);
        if(len!=-1){
            target[len]='\0';
            printf("->%s",target);
        }
    }
    printf("\n");

}

// 普通输出
void print_entries(FileInfo *files, int count, Options *opts)
{
    // 这个函数实现打印inode和块大小,文件名字
    int i;
/*
-r实现
*/
    int start;
    int end;
    int step;
    if(opts->reverse){
        start=count-1;
        end=-1;
        step=-1;
    }else{
        start=0;
end=count;
step=1;
    }
    for (i = start; i !=end; i+=step)
    {
        if(opts->long_format)
        {
            print_long(&files[i],
                       opts);
        }
        else
        {
            if(opts->show_inode)
            {
                printf("%-8lu ",
                       files[i].st.st_ino);
            }

            if(opts->show_blocks)
            {
                printf("%-4ld ",
                       (files[i].st.st_blocks + 1) / 2);
            }

            printf("\n");
        }

        printf("%s\n", files[i].name);
    }
}

// 释放FileInfo数组
void free_entries(FileInfo *files, int count)
{
    int i;
    for (int i = 0; i < count; i++)
    {
        // 先释放每一个元素名字和路径，在释放整个数组
        free(files[i].name);
        free(files[i].path);
    }
    free(files);
}

// 列出目录，实现递归
//  参数path目录路径,show_title是否显示目录标题（输入多个目录的是时候
int list_dir(const char *path, int show_title, Options *opts)
{
    FileInfo *files;
    int count;

    if (show_title)
    {
        printf("%s:\n", path);
    }

    // 读取目录,得到文件信息，名字完整路径属性
    files = read_dir_entries(path, opts, &count);
    if (files == NULL)
        return -1;

    sort_entries(files,count,opts);

    // 输出
    print_entries(files, count, opts);

    //-R实现
    if(opts->recursive){
        for(int i=0;i<count;i++){
            if(S_ISDIR(files[i].st.st_mode)){
                //跳过.和..，不然就进入死循环了
                if(strcmp(files[i].name,
                          ".") == 0 ||
                   strcmp(files[i].name,
                          "..") == 0)
                {
                    continue;
                }
                printf("\n");
                list_dir(files[i].path,1,opts);
            }
        }
    }
    

    // 释放
    free_entries(files, count);
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