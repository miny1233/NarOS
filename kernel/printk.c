void printk(char* str){
  char *cursor = (void*)0xb8000;
  while(*str!='\0')
  {
    *(cursor++) = *(str++);
    *(cursor++) = 0x30;
  }
}
