void strcpy(char* des,const char *sou)
{
    while(*sou!=0)
    {
        *des++=*sou++;
    };
    *des=0;
    return;
}