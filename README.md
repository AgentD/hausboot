# About

This repository contains a half-baked attempt at writing a boot loader in C++.
The VBR and MBR are done, implementing both boot sectors in high level C++ code.
Currently, the second stage loader only prints a message and hangs.

It is *not* intended for any real world use. The name has been chosen to reflect
the utter impracticality for the supposed purpose.

## Background

TODO: make the weird ramblings less incoherent and turn it into a blog post?

I learned to program with QBASIC and 8086 assembly under MS-DOS. I later
learned how a CPU works from the gate level up, before I learned C. Over the
years, I used C++ on-again-off-again, having to re-learn it every time
since 2011, because the standards commedy keeps changing the darn thing.

On your typical beginner forums, there were many who ambitiously wanted to
write a computer game, or an OS. For both there are many dedicated websites,
with tutorials. If you go down the OS route, you end up being told that OS
development is *all about* the nookies and crannies of the x86 architecture.
Instead of learning about scheduling, memory management or filesystems, you
learn about the intricacies of the protected mode and the A20 gate.

Among other things, you are told that boot loaders are hand written by brave
men (i.e. not you or me) in assembly. You cannot write a boot sector in C,
C is too big and FAT. Booting happens [below C level](https://source.denx.de/u-boot/u-boot).

Even after many years of C programming, a real eye-opener for me was attending
a (now sadly discontinued) masters course on compiler construction. Turns out,
compilers are *really good*. Even my toy optimizing compiler I wrote that
semester could churn out surprisingly efficient assembly. And I'm pretty sure
the gcc people are a lot smarter than me, in addition to a 30 year head start.

It changed my way of thinking about programming. Source code should first and
foremost be written *for people to read*. Source code should convey *high level
meaning*, both to people, as well as to the compiler. Bit twiddling hacks are
an utter waste of time and make the job harder for the compiler as well. Keeping
down asymptotic complexity is your job, optimizing the coefficient is the
compilers job (but you should really be thinking in terms of data structures
anyway, rather than algorithms).

When you use a programming language, you are really using a *formal language*
based *calculus* to describe that *meaning*. A compiler is essentially a
term-rewriting system and you are operating with the semantics of that
*calculus*. Adages like "C is just an assembly pre-processor" are not just
nonsense, but outright dangerous. The chasm between C semantics and machine
semantics are what gives rise to fear and misunderstanding around the dreaded
undefined-behavior-boogy-man that HackerNews loves to share second-hand camp
fire stories about so much.

Pondering further on that, I concluded that an expressive, strong type system
is really key to expressing such high level meaning and semantics. A modern,
high-level language should IMO not have bit manipulation *at all*. Flag fields
are really just composite data types in disguise, model them as such and gain
type safety! I'd go as far as claim that memory-safety issues are themselves
really just a *symptom* of flaws in the type system.

Revisiting C++ once again, it made me realize that you can model all of those
high level type semantics really well with classes. Thinking about classes as
user defined *types* is an immensely powerful concept, and C++ even allows you
to micro manage memory layout if need be. If leveraged, C++ effectively hands
you tools of Ada like proportions, you just need to use them.

On the other hand, a lot of criticism I heard about C++ over the years also
pertained to C++ programs being big, bloated and frivolous resource hogs.
Particularly from self-declared, "suckless" minimalists who love the write
broken, crippled and unreadable programs for the sake of pseudo-minimalism.
This is in part due to past short comings of the STL, in part also brings us
back to the detour about compiler optimization: Modern compilers are really
good, you won't believe how good. C++ is a "systems programming language",
lets go ahaead and implement a *boot sector* in C++, using many of the high
level concepts I just described.

It's kills several debates with one stone, and [there might be some bragging rights in being able to pull off such an utter perversion](https://www.youtube.com/watch?v=j4miM_sZU-k).

## How to Build This

You need a *very* recent version of g++, as well as the 32 bit version
of `libstdc++` if you are on x86_64 (if you are on "Apple Silicon": sorry...).

Simply run make, use `make runqemu` to fire up qemu:

```sh
make
make runqemu
```

Alternatively, you can try running it in [Bochs](https://en.wikipedia.org/wiki/Bochs):

```sh
make runbochs
```

With any luck, it might work on your machine as well :-). I have only tested it
on two Fedora installations so far.

# Boot Strapping Process

## Master Boot Record (MBR)

When you turn on an (legacy BIOS) PC, we are basically transported back to
the 1980s again. The CPU pretends to be a 16 bit mode 8086 that can address
up to 1M of memory using segmentation.

After reset, the 8086 starts execution at FFFF:0000, or at linear
address 0xFFFF0. The IBM PC hardware has a ROM chip mapped at that region
that contains the BIOS. The lower 640k of address space are left to the
operating system, including stuff like memory mapped hardware (e.g. video RAM).
This is called "conventional memory" for some reason (as opposed to
*unconventional* memory?).

The boot strap code in the BIOS simply looks for a floppy or hard drive that
has the magic signature 0xAA55 at the end of the first sector. This magic value
indicates a bootable drive. The BIOS simply copies the first sector to the
magic address 0x7C00 and jumps into it.

This would typically contain an OS boot loader in hand written assembly that
fetches a bigger executable from the same disk. This would possibly be a
second stage loader that can read the file system and load the OS kernel.

Eventually somebody figured, it would be a jolly idea to have partitioned
hard disks on the IBM too! This was implemented by storing a partition table
at the end of the first sector. Instead of the original boot strap program,
the first sector now contains the "Master Boot Record" that is
loaded & executed by the BIOS. It relocates itself (typically to `0x0600`;
that's what Microsoft went with), inspects the partition table and chain
loads the actual boot loader from the first sector of a partition marked
as bootable.

The BIOS code was left untouched, and doesn't need to know about partitions.
It's completely an OS feature, but the partition boot loader needs to be aware
of it, hence we started calling those "Volume Boot Record" (VBR).

The "standard" layout of the partition table is simply what DOS et al settled
on, using 4 entries, each 16 byte. Later schemes like GPT with UUID partition
labels and additional, *extended* partitions are built on top of that in a
backwards compatible way, which is why you can still only have up to 4 regular,
non-extended partitions there.

But with a typical, old DOS MBR, this leaves us with 446 bytes of payload
area for the MBR program. For comparison: if we used 80-column IBM cards,
that would be roughly 6 or 7 punched cards (depending on the format).

In the directory `mbr`, there is C++ source for a program that fits in this
space and does the task described above. The bulk of the implementation is
actually in C++ classes defined in headers in the `include` directory.

## Volume Boot Record (VBR)

The volume boot record sits in the first sector of the partition and is
chain-loaded by the MBR.

It typically shares the sector with the super block of a filesystem. For
instance, FAT32 uses the first 90 bytes for filesystem information, with
the first 3 bytes holding a jump instruction, so we don't accidentally
try to execute the filesystem information.

Subtracting the super block and 2 byte boot signature, that leaves a FAT32 VBR
with 420 bytes of payload for a program (6 punched cards).

So what the FAT32 VBR does, is chain-load a much larger program, stored in
reserved sectors between the super block and the first FAT. We are guaranteed
at least 32 reserved sectors, but this count includes the first sector itself,
the special FS information sector, and a backup copy of the first sector (can
be disabled).

If we disable the backup copy and relocate the FS information sector, we can
max that out to 30 sectors, or a *whopping 15k* for our second stage
boot loader.

In the `vbr` directory, there is a C++ program that does should fit into
that 420 byte region, and chain loads the second stage. The `installfat`
directory contains C++ source for a program that modifies a FAT image as
described and installs the VBR, as well as the second stage binaries.

## Second Stage Boot Loader

This has currently not been implemented yet. The binary prints out a dummy
hello message and halts the system.

What this *would* do is:

 - interpret the FS data structures to locate a file containing our OS kernel
 - load that into memory
 - setup environment that the kernel expects
 - execute it

It would be really cool to actually implement the Linux boot protocol, so we
could load a Linux kernel with an initramfs image.

This currently requires more work. The main purpose, showing that a
boot *sector* can be implemented in C++, is done.

# Minor Things I Learned Along the Way

## Building 16 bit code with gcc

gcc supports 16 bit code in a really passive aggressive way. Turns out, you can
use 32 bit registers from 16 bit code by using the `0x66` op-code prefix. Once
you are in 32 bit mode, the meaning of the prefix is inverted (i.e. use it to
access only the lower 16 bit instead of the full register).

When told to generate 16 bit code, gcc will basically generate 32 bit code with
instructions prefixed to run in 16 bit real mode. You need to take this into
account, when calling from assembly into gcc generated code or vice versa.

This also means that current gcc theoretically cannot target anything pre i386.

Also, gcc will happily poop out XMM/SSE instructions with a `0x66` prefix when
it needs to move data around. Set the proper `-march=i386` flag, to make sure
it doesn't do that.

Furthermore, accessing memory past 64k is a PITA. There is an `0x67` opcode
prefix that allows you to load/store using a 32 bit register. On Qemu, this
works just fine, but on real hardware, this this will trigger a protection
fault if you try to access memory past the 64k current segment boundary.

## Computed Goto

gcc allows you to do this:

```C++
somelabel:
	auto *wtf = &&somelabel;

	goto *wtf;
```

the intended use is to hack together jump tables for state machines by hand.
But it also allows pointer arithmetic on the, um "label pointer" and even
funkier stuff:

```C++
	goto *((void *)0x7c00);
```

I initially had the MBR written in C++ entirely, including the self relocating,
but ultimately replace the entry/exit code with an assembly stub instead. It
was very fidgety and relied on the compiler generating *specific* code, e.g.
not turning an absolute jump to a relative one. This is also, why the reloacing
code couldn't just *call* main either. The compiler would prefer a more
compact, relative call, or if main was static, just inline it.

## Disassembling a raw Binary

I had to resort to this a few times to double check:

```sh
objdump -D -m i8086 -b binary ./mbr/mbr.bin
```

## Debugging Low Level Code with gdb and Qemu

Qemu has built in support for the gdbserver protocol:

```sh
qemu-system-i386 -drive format=raw,file=./disk.img -S -s
```

You can then connect to it from within gdb:

```sh
(gdb) target remote localhost:1234
...bla bla bla...
(gdb) set architecture i8086
The target architecture is set to "i8086".
(gdb) break *0x7c00
Breakpoint 1 at 0x7c00
(gdb) c
Continuing.

Breakpoint 1, 0x00007c00 in ?? ()
```

You can dump registers and disassemble memory ranges, and so on.

## Debugging Low Level Code with Bochs

Bochs needs to be compiled with debugger mode enabled for this. Typical Linux
Distros have a separate package for that, with a separate binary that can
launch you into a debug shell:

```sh
$ bochs-debugger -q -f bochsrc.txt
... bla bla bla ...
(0) [0x0000fffffff0] f000:fff0 (unk. ctxt): jmpf 0xf000:e05b          ; ea5be000f0
<bochs:1> lbreak 0x7c00
<bochs:2> c
... bla bla bla ...
(0) Breakpoint 1, 0x0000000000007c00 in ?? ()
Next at t=16300062
(0) [0x000000007c00] 0000:7c00 (unk. ctxt): xor ax, ax                ; 31c0
<bochs:3>
```

You can single step with `s`, an optional number argument steps over several
instructions, disassembly and address are shown.

For break points, `break` expects a `segment:offset` address, while `lbreak`
expects a linear address, `c` continues until the next break point.

You can dump registers with `r`:

```sh
<bochs:3> r
CPU0:
rax: 00000000_0000aa55
rbx: 00000000_00000000
rcx: 00000000_00090000
rdx: 00000000_00000080
rsp: 00000000_0000ffd6
rbp: 00000000_00000000
rsi: 00000000_000e0000
rdi: 00000000_0000ffac
r8 : 00000000_00000000
r9 : 00000000_00000000
r10: 00000000_00000000
r11: 00000000_00000000
r12: 00000000_00000000
r13: 00000000_00000000
r14: 00000000_00000000
r15: 00000000_00000000
rip: 00000000_00007c00
eflags 0x00000082: id vip vif ac vm rf nt IOPL=0 of df if tf SF zf af pf cf
```

or inspect/hexdump memory regions with `x /<count><unit> <address>`:

```sh
<bochs:12> x /16b 0x7df0
[bochs]:
0x0000000000007df0 <bogus+       0>:	0x00	0x00	0x00	0x00	0x00	0x00	0x00	0x00
0x0000000000007df8 <bogus+       8>:	0x00	0x00	0x00	0x00	0x00	0x00	0x55	0xaa
```
