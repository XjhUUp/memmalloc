## MEMmalloc技术文档

赛题选择：[proj28-3RMM](https://github.com/oscomp/proj28-3RMM)

队伍名称：HDU_702_OS_小分队

队员姓名：夏佳恒、周志奇、赵育淼

指导教师：贾刚勇

学校名称：杭州电子科技大学

#### 一、赛题介绍

##### 1.1 项目描述

在工业控制、航天航空等严苛的应用环境中，计算机硬件通常会受到高低温、潮湿、电磁辐射、宇宙射线等恶劣因素的影响。为了实现高可靠的计算机系统，传统上多采用硬件的升级改造，或者定制操作系统/内核，其成本异常昂贵。

内存在计算机系统中的重要性仅次于CPU，在恶劣环境下会出现单粒子翻转等问题，进而导致程序或系统崩溃。如果能通过软件的解决方案，实现一个针对严苛场景下的内存管理器，有助于节省硬件成本。

3RMM项目的目标是，实现一个可靠的、健壮的、实时的内存管理器，在Linux系统下以一个库的形式对外发布，运行在用户态。建议使用C语言开发，以便将来更易于移植到Linux内核或别的操作系统中。

Reliable 可靠：实现基本的内存管理功能，包括内存池等，在重负载、高并发、长期运行的情况，系统都能正常使用。

Robust 健壮：在内存出现单粒子翻转等异常情况下，依然能够正常运行。

Real-time 实时：使用高效的内存分配算法以及其他技术手段，加快内存分配和读写的速度。

##### 1.2 预期目标

###### 第一题：简单的内存分配器

- [x] 实现一个简单的内存分配器，需要支持malloc、free、calloc、realloc

###### 第二题：支持多线程的高效内存分配器

- [x] 在第一题的基础上实现多线程和并发

- [x] 通过benchmark测试

###### 第三题：支持内存池的内存分配器

- [ ] 支持基于处理器核心亲和度的内存池管理

###### 第四题：高可靠的内存管理器

- [x] 实现抗单粒子翻转功能

###### 第五题：3R的内存管理器

- [x] 使用benchmark进行测试，根据性能数据以及测试中占用的物理内存综合评估方案

##### 1.3 项目描述

我们在用户态实现了一个可靠、健壮、实时的内存分配器——memmalloc。除了基于核心亲密度的内存池管理外，memmalloc基本实现了赛题要求。

[技术文档](https://gitlab.eduxiji.net/xiajiaheng/project788067-126918/-/blob/master/doc/%E6%8A%80%E6%9C%AF%E6%96%87%E6%A1%A3.md)

[测试文档](https://gitlab.eduxiji.net/xiajiaheng/project788067-126918/-/blob/master/doc/TEST.md)

##### 二、使用说明

memmalloc除了基础的功能之外，还增加了日志功能；在抗单粒子翻转方面，除了三模冗余法之外，还有奇校验法。所以memmalloc具有多个版本。

进入src目录，执行：

```makefile
###需要日志功能时，需要把Makefile里的-DFILE_NAME换成可执行文件名称，例如可执行文件名称是memmalloc.out,需要把-DFILE_NAME='"log.out"换成-DFILE_NAME='"memmalloc.out"'

#标准版本，不带抗单子翻转和日志功能
make version_1

#不带抗单粒子翻转功能，带日志功能
make version_2

#采用奇校验法抗单粒子翻转，带日志功能
make version_3

#采用三模冗余法抗单粒子翻转，带日志功能
make version_4

#采用奇校验法抗单粒子翻转，不带日志功能
make version_5

#采用三模冗余法抗单粒子翻转，不带日志功能
make version_6
```



##### 三、 提交仓库目录和文件描述

* benchmark
  * images:存放生成的测试结果图片
  * test_code:
    * exp_*.c：测试程序
  * exp_*.sh:测试脚本
  * static_analy.py:生成测试图片的python脚本
* doc：项目文档目录
* include
  * lish.h：双向链表头文件
  * log.h:log头文件
  * memmalloc.h：memmalloc头文件
* log：存放日志文件
* src：
  * memmalloc.c：memmalloc的源文件
  * log.c：log源文件
  * Makefile build文件

* test：
  * *.c：测试源文件
  * Makefile build文件

* tr_malloc:上届内存分配器源代码





