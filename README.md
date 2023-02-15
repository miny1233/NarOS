# NarOS
#### 32位系统
#### 自制操作系统，主要是用来学习。
> 因为主要是学习用的，所以你可以向我提交小的BUG修复，但是提交功能的话，我可能不会通过 \
> 因为我还要拿来练手的（哈哈）
## 微内核
### 不挖了 ~~挖个坑，这次要用点不一样的用户管理，要比CloudLinux拥有更大的自由度~~

## 架构
### boot
- boot 用于载入内核加载器
- load 读取内核写入内存并跳转至启动点
### io
- mount 挂载io
- unmount 取消挂载io
- write 写入io 每次1kb
- read 读入io 每次1kb

### syscall
- syscall 用户调用系统调用
- __syscall 内核处理系统调用
- syscallList 系统调用表

> 系统调用必须使用fastcall调用协定！

## 目前的一些问题及解决方案
### boot
>- 默认是直接读入8M数据写入内存
>> 这个确实是方便写才这么做的，实际上我也想要根据实际大小写入，不过没那精力了
>- 入口点要手动设置
>> 本来是想着自动载入的，一般入口在内存0处，但是现在的问题是gcc似乎不肯指定位置 \
>> 我的想法是使用grub来载入，还减少麻烦调试汇编，不过我还不懂grub的头要怎么整 \
>> 有佬来教教我就好了
### io
>- 不安全，恶意程序可能会破坏正常的传输
>> io暂时不考虑恶意程序的问题，未来可能会标注io所有者
>- 效率低，寻找io过慢，以及1kb的限制，小数据包和大数据包传输效率较低
>> 未来增加open函数用来更加快速的打开io （暂时不考虑，现在是能跑就行，越精简越好）
### syscall
>- 内联汇编过不了编译
>> 这应该是个小问题，未来修复难度不大
>- 作为微内核，syscall功能过少
>> 未来会更新动态系统调用，可以在内核运行时添加系统调用，实现一个高拓展性的微内核
>- 系统调用函数最大只能有2个参数
>> 根据32位下fastall的调用协定，第一第二参数分别传入ecx和edx，超过使用堆栈 \
>> 由于目前未对syscall做过测试，因此不能保证栈平衡，如果使用2个以上的参数可能会造成意外错误
