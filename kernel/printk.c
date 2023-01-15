char *cursor = (void*)0xb8000;

void printk(char* str){
  while(*str!='\0')
  {
    *(cursor++) = *(str++);
    *(cursor++) = 0x30;
  }
}
