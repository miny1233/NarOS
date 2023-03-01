#!/bin/bash

pause()
{
  SAVEDSTTY=`stty -g`
  stty -echo
  stty raw
  dd if=/dev/tty bs=1 count=1 2> /dev/null
  stty -raw
  stty echo
  stty $SAVEDSTTY
}
 
if [ -z "$1" ]; then
	echo '请按任意键继续...'
else
	echo -e "$1"
fi


cd build
echo Building Boot
nasm ../boot/boot.s -o boot.bin
nasm ../boot/setup.s -o setup.bin

echo Building Kernel
cmake ..
make clean
make
objcopy -O binary NarOS kernel.bin

echo Enter any key to continue

pause

echo Writing to Disk

dd if=boot.bin of=c.img bs=512 count=1 conv=notrunc
dd if=setup.bin of=c.img bs=512 count=1 seek=1 conv=notrunc
dd if=kernel.bin of=c.img bs=512 count=50 seek=2 conv=notrunc

echo Running Bochs

bochs -q