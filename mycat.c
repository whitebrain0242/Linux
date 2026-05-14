#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
int main(){
    //文件拷贝代码（底层cp
    //1.打开源文件（只读source.txt
    //2.创建/清空目标文件（只写，权限0644 dest.txt
    //3.循环读写：双层循环：外层是当可以从src里面读到字节的时候，存入buf,字节数是buf的大小
    //          内层循环：当当前写入数字小于当前读到的数字的时候继续，
    //                  从buf+wrritten里写入dst,大小是读到数据-已经写入数据个数，写入最后要加上这次写入个数
    int src=open("source.txt",O_RDONLY);
    if(src==-1){
        perror("open source");
        return 1;
    }
    int dst=open("dest.txt",O_WRONLY|O_CREAT|O_TRUNC,0644);
    if(dst==-1){
        perror("write");
        close(src);
        return 1;
    }
    
    char buf[10];
    ssize_t n;
    while(n=(read(src,buf,sizeof(buf)))>0){
        ssize_t written=0;
        while(written<n){
            ssize_t m=write(dst,buf+written,n-written);//这里buf是开头，written是已经写进去的
            if(m==-1){
                perror("write");
                close(src);
                close(dst);
                return 1;
            }
            written+=m;
        }
    }
    if(n==-1){
        perror("read");
    }
    close(src);
    close(dst);
    return 0;
}