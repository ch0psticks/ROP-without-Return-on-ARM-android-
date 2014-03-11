

ROP在Windows、Linux以及MAC OS X平台上的exploitation中已经很普遍了，在iOS的越狱中也经常用到。在android平台怎么样呢？从本质来看，Android上的NativeCode以ARM指令进行执行的，所以只要基于ARM的ROP没问题，Android上NativeCode层的ROP也不会有问题。

本文主要根据Lucas Davi发表于2010年的一个报告[《Return-Oriented-Programming without Returns on ARM》](#ROP_without_Return_on_arm)，对其中的实验进行了重现。

## 一. 原理

### 1.1分支指令

在ARM中程序可以通过分支指令B，BL，BLX，BX等进行分支跳转(`B-->Branch`)。其中BLX和BX进行跳转时可根据标志位进行指令集的切换（ARM指令集和Thumb指令集）。如：

	adds	r0, r7, #0
	adds	r1, r6, #0
	blx	r5


对于`blx Rn`或`bx Rn`，当`Rn`的最低位[0]为0时，则指令集切换到ARM指令集状态；若最低位为[0]为1时，则指令集切换到Thumb指令集状态。

此外，分支指令的目标不一定通过寄存器Rn来传递，也可以跳转指定偏移，如`blx label`，其中label是程序相对便宜表达式，*采用label时则指令集切换到ARM状态*。

**注意**：

* 指令不能转移到当前指令+/-4MB以为的地址；
* Rn的位0被清0，则位1也应当为0（ARM指令集状态下，4字节寻址，故最低2位均为0）

指令的其它具体介绍看参考文献[《Thumb指令集》](http://wenku.baidu.com/link?url=T9llkhuwBDpF7hQyVKLIT1nEq6xJVpBw97VFoKuY8tqSy-xHuMYlA0QUFifk3pSmfP1uSuvV4VDHiuClxN3PNDmHFkLviK2HwnDUZpeqTOe###)。

### <h3 id=1.2>gadget的组织</h3>

根据对ARM下blx分支指令的介绍，对于指令`blx Rn`，若攻击者能够控制Rn寄存器则可以获得控制流，这有点类似于x86平台上的栈异常，控制返回地址后，即可劫持控制流。

如果可以如同x86的ROP一样，将各包含ret指令的地址布局到堆栈上，巧妙的串联起来，则可实现特定的功能。

在文中[《Return-Oriented-Programming without Returns on ARM》](#ROP_without_Return_on_arm)，作者对ROP的组织的关键思想是——功能性gadget之间通过一个称之为connector的gadget进行联接，即所谓的ULB，update-load-branch。ULB gadget所实现的功能就是调整一个“SP”指针，使得跳转到不同的功能性gadget。一个connector示例如下：

	ADDS R6, #4
	LDR R5, [R6, #124]
	BLX R5
	
可以看到每次R6调整4个字节，使得R5每次值都不同，跳转到不同的地址。在控制R6的情况下，可以让程序跳转到R6+fixed_offset指向的地址。因此，在connector中担任R6角色的寄存器可称之为**Rja**，jump address register。

为了将每个功能性gadget串联起来，需要尽量将每个gadget执行后跳转到ULB上，即connector上。那么最后一条指令则通常是`blx Rn`。Rn都是相同的。由于该Rn指向ULB，故称之为**Rulb**。

Gadget1->connector->Gadget2->connector->Gadget3...

*注： 在执行blx之前，r0~r3通常被用作目标寄存器，因此建议采用r4~r12的寄存器作为Rja；*

为了证明的ROP攻击的有效性，在文章[《Return-Oriented-Programming without Returns on ARM》](#ROP_without_Return_on_arm)中，对这种攻击方法进行了图灵完整性(Turing Completeness)的证明，即证明能有效的模拟实现图灵机中的基本操作（内存操作，数据处理，循环、分支等控制流，函数调用等），从而说明可实现计算机中普通程序一样的的任何功能

Rja, Rsp, 

## <h2 id=implementation>二. 实现</h4>

### 验证程序

**验证程序结构**
构造大致流程如下的测试程序。
	
	typedef struct foo{
		char buf[200];
		jmp_buf jb;
	}FOO, *PFOO;
	
	...

	PFOO f;
	int size=0;
	size = sizeof(FOO);
	f = (PFOO)malloc(size);

	...
	
	int i;
	//保存上下文
	i = setjmp(f->jb);
	
	if (i!=0)
		return 0;
	
	//存在覆盖的可能
	fgets(f->buf, 456, sFile);
	//恢复上下文
	longjmp(f->jb, 2);
	
利用setjmp和longjmp函数特性，在setjmp之后，恰当覆盖jmp_buf中保存的寄存器信息，即可在longjmp时恢复寄存器，从而在`longjmp+76`处，通过指令`bx lr`触发执行。

**longjmp流程**

在后面进行jmp_buf覆盖时，会涉及到longjmp，所以有必要先看看longjmp的基本流程。

	d0c51c in epoll_wait () from libc.so
	(gdb) disassemble longjmp
	Dump of assembler code for function longjmp:
	0xafd0d104 <+0>:	ldr	r2, [pc, #-12]; 0xafd0d100 <setjmp+48>
	0xafd0d108 <+4>:	ldr	r3, [r0]
	0xafd0d10c <+8>:	teq	r2, r3
	0xafd0d110 <+12>:	bne	0xafd0d158 <longjmp+84>
	0xafd0d114 <+16>:	ldr	r2, [r0, #4]
	0xafd0d118 <+20>:	push	{r0, r1, lr}
	0xafd0d11c <+24>:	sub	sp, sp, #4
	0xafd0d120 <+28>:	mov	r0, r2
	0xafd0d124 <+32>:	blx	0xafd17a1c <sigsetmask>
	0xafd0d128 <+36>:	add	sp, sp, #4
	0xafd0d12c <+40>:	pop	{r0, r1, lr}
	0xafd0d130 <+44>:	add	r2, r0, #76; 0x4c
	0xafd0d134 <+48>:	ldm	r2, {r4, r5, r6, r7, r8, r9, r10, r11, r12, sp, lr}
	0xafd0d138 <+52>:	teq	sp, #0
	0xafd0d13c <+56>:	teqne	lr, #0
	0xafd0d140 <+60>:	beq	0xafd0d158 <longjmp+84>
	0xafd0d144 <+64>:	mov	r0, r1
	0xafd0d148 <+68>:	teq	r0, #0
	0xafd0d14c <+72>:	moveq	r0, #1
	0xafd0d150 <+76>:	bx	lr	
	0xafd0d154 <+80>:	mov	pc, lr
	0xafd0d158 <+84>:	blx	0xafd1d3bc <longjmperror>
	0xafd0d15c <+88>:	blx	0xafd15e8c <abort>
	0xafd0d160 <+92>:	b	0xafd0d158 <longjmp+84>
	
	End of assembler dump.

可以看到：大致是验证jmp_buf有木有被破坏(利用起始字节)，从offset=0x4c处恢复r4~lr的11个寄存器，验证sp和lr是否为0；最后longjmp+76处，跳转到lr所指向的位置。

所以通过上述longjmp的流程，我们可以得到如下大致思路：

1. 覆盖lr，从而首先获得执行流，lr值保存于jmp_buf+0x4c+4*10处；
2. 通过覆盖r4~r12,sp等寄存器实现参数的布局和gadget的串联；
3. 如果涉及r0~r3的寄存器，则需要通过其他gadget实现r0~r3的初始化。

### gadget选择和组合

从模块中选择gadget指令块，可利用objdump手工寻找。当然更高效的办法就是利用[ropshell.com](http://www.ropshell.com)网站提供的在线搜索功能。可上传需要搜索的模块文件，然后即可搜索其中的指令。

根据上述分析，如果要只是实现单个函数调用，则只需很简单的一个或两个gadget指令即可，无需前文所说的`connector` gadget。但考虑到实现复杂功能的需求，则仍然使用`conector`。

**connector**

	add	sp, #8
	pop	{r4}
	pop {r3}
	add	sp, #8
	bx	r3

可以看到，每次执行该gadget可将sp增加24字节，从而实现gadget指令块的切换。加入当前sp为`0xaaaabbbb`,则gadget_1的地址徐写入到`0xaaaabbbb+12`,gadget_2的地址需写入到`0xaaaabbbb+12+24`处。

**参数放入r0**

	adds	r0, r7, #0
	adds	r1, r6, #0
	blx	r5

如上，可将参数直接覆盖到r7中，而r5则用connector的地址覆盖，从而使得继续执行connector，从而跳转到下一gadget_2的执行。

**执行函数调用**

以调用system函数执行命令为例，则gadget_2为system函数，所以gadaget_2，为system函数地址，故`0xaaaabbbb+12+24`处覆盖为
system函数地址。

总的流程就是

connector-->gdaget_1-->connector-->gadget_2

如果对于复杂功能的实现，则只需延长上述gadget chain即可。

connector-->gdaget_1-->connector-->gadget_2-->....-->connector-->gadget_n
	

### 关于gadget地址的修正说明

前面已经说明了，对于`blx Rn`，Rn的最低位决定是按照ARM指令集还是按照Thumb指令集执行。考虑到libc.so中大部分为thumb指令集，Rn中放置的必然是个奇数地址(ROPshell网站搜到的基本都是奇数地址，即无需修正)，当gadget地址或函数地址不为奇数时，**需要进行+1操作，进行修正，使其以thumb指令状态执行**。如 system地址为`0xafd17fd8`,使用时则应使用`0xafd17fd8+1`。否则会出现无端的执行错误。（被坑了很久很久-_-）

## 三. 总结

### 实际的可操作性

大家可能会看到这里仅仅是一个构造的漏洞程序，而且很多信息都是硬编码的，那么在真实场景中如何解决随机化？其实这点，利用信息泄露，获取其中一个变量或函数的实际地址，则有可能根据偏移推测出内存布局，从而绕过ASLR，在文章[《Jekyll on iOS》](#Jekyll_on_iOS)中有介绍和应用。

### ROP的后续

这篇文章，从现在来看，已然比较早，但是针对Android的ROP并未出现太新的进展，那么就有两种可能：1）不值得去做；2）太难。姑且认为是后者吧，这是一个不存在什么不可能的师姐和时代，永远无法超越的是思想，而非步伐。

### 其它

在Android2.3.3上测试过，代码放在[github](https://github.com/ch0psticks/ROP-without-Return-on-ARM-android-)上。

gadget用的是libc.so中的指令。


## 参考

1. <a id="Elementary_on_ARM"></a>Claud.[Elementary-ARM-for-Reversing](http://www.claudxiao.net/2011/07/elementary-arm-for-reversing/). 2011
2. <a id="ARM_Reference"></a>ARM. [ARM® and Thumb®-2 Instruction Set Quick Reference Card](http://infocenter.arm.com/help/topic/com.arm.doc.qrc0001l/QRC0001_UAL.pdf).
3. <a id="Thumb_instruction_sets"></a>百度文库. [T
humb指令集](http://wenku.baidu.com/link?url=T9llkhuwBDpF7hQyVKLIT1nEq6xJVpBw97VFoKuY8tqSy-xHuMYlA0QUFifk3pSmfP1uSuvV4VDHiuClxN3PNDmHFkLviK2HwnDUZpeqTOe###)
4.  <a name="ROP_without_Return_on_arm"></a>Davi L, Dmitrienko A, Sadeghi A R, et al. [Return-oriented programming without returns on ARM](http://www.trust.informatik.tu-darmstadt.de/fileadmin/user_upload/Group_TRUST/PubsPDF/ROP-without-Returns-on-ARM.pdf)[J]. System Security Lab-Ruhr University Bochum, Tech. Rep, 2010.
5.  <a id="ROP_without_return"></a>Checkoway S, Davi L, Dmitrienko A, et al. [Return-Oriented Programming without Returns](https://www.cs.jhu.edu/~s/papers/noret_ccs2010/noret_ccs2010.pdf)[C]//Proceedings of the 17th ACM conference on Computer and communications security. ACM, 2010: 559-572.
6.  <a name="Jekyll_on_iOS"></a>Wang T, Lu K, Lu L, et al. [Jekyll on iOS: when benign apps become evil](https://www.usenix.org/system/files/conference/usenixsecurity13/sec13-paper_wang-updated-8-23-13.pdf)[C]//Presented as part of the 22nd USENIX Security Symposium}. USENIX}, 2013: 559-572.
