//
// Created by 谢子南 on 24-6-7.
//

#ifndef NAROS_UNISTD_H
#define NAROS_UNISTD_H

int open(const char* path,char mode);
int read(int fd,char* buf,size_t buf_size);
int write(int fd,const char* buf,size_t len);
void yield();
void exit();

#endif //NAROS_UNISTD_H
