# About

This repository contains a half-baked attempt at writing a boot loader in C++.
The VBR and MBR are done, implementing both boot sectors in high level C++ code.
Currently, the second stage loader only prints a message and hangs.

It is *not* intended for any real world use. The name has been chosen to reflect
the utter impracticality for the supposed purpose.

## Background

TODO: make the weird ramblings less incoherent and turn it into a blog post?

I learned to program as with QBASIC and 8086 assembly under MS-DOS. I later
learned how a CPU works from the gate level up, before I learned C. Over the
years, I used C++ on-again-off-again, having to re-learn it every time
since 2011, because the standards commedy keeps changing the darn thing.

On your typical beginner forums, there were many who ambitiously want to write
a computer game, or an OS. For both there are many dedicated websites,
with tutorials. If you go down the OS route, you end up being told that OS
development is *all about* the nookies and crannies of the x86 architecture.
Instead of learning about scheduling, memory management or filesystems, you
instead learn about the intricacies of the protected mode and the A20 gate.

Among other things you are told, that boot loaders, are hand written by brave
men (i.e. not you or me) in assembly. You cannot write a boot sector in C,
C is too big and FAT. Booting happens [below C level](https://source.denx.de/u-boot/u-boot).

Even after many years of C programming, a real eye-opener for me was attending
a (now sadly discontinued) masters course on compiler construction. Turns out,
compilers are *really good*. Even my toy optimizing compiler I wrote that
semester could churn out surprisingly efficient assembly.

It changed my way of thinking about programming. Source code should first and
foremost be written *for people to read*. Source code should convey *high level
meaning*, both to people, as well as to the compiler. Bit twiddling hacks are
an utter waste of time and make the job harder for the compiler as well. Keeping
down asymptotic complexity is your job, optimizing the coefficient is the
compilers job (but you should really be thinking in terms of data structures
anyway, rather than algorithms).

When you use a programming language, you are really using a *formal language* to
describe that *meaning*. A compiler is essentially a term-rewriting system and
you are operating with the semantics of that language. Adages like "C is just an
assembly pre-processor" are dangerous nonsense. The chasm between C semantics
and machine semantics gives rise to fear and misunderstanding around the dreaded
undefined-behavior-boogy-man that HackerNews loves to share second-hand camp
fire stories about so much.

Pondering further on that, I concluded that an expressive, strong type system
is really key to expressing such high level meaning and semantics. A modern,
high-level language should IMO not have bit manipulation *at all*. Flag fields
are really just composite data types in disguise, model them as such and gain
type safety! I'd go as far as claim that memory-safety issues are themselves
really just a *symptom* of flaws in the type system.

Revisiting C++ once again, it made me realize that you can model all of those
type semantics with classes. Thinking about classes as user defined *types*
is an immensely powerful concept, and C++ even allows you to micro manage memory
layout if need be. If leveraged, C++ effectively hands you tools of Ada like
proportions, you just need to use them.

Growing up, I heard lots of criticism about C++ programs being big, bloated
and frivolous resource hogs.

This finally lead me to this idea: C++ is a "systems programming language",
lets implement a *boot sector* in C++, using many of the high level concepts
I just described. It's kills several debates with one stone,
and [there might be some bragging rights in being able to pull off such an utter perversion](https://www.youtube.com/watch?v=j4miM_sZU-k).

## How to Build This

You need a *very* recent version of g++, as well as the 32 bit version
of `libstdc++` if you are on x86_64 (if you are on "Apple Silicon": sorry...).

Simply run make, use `run.sh` to fire up qemu:

```sh
make
./run.sh
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

The boot strap code in the BIOS simply looks for a floppy or hard drive that
has the magic signature 0xAA55 at the end of the first sector. This magic value
indicates a bootable drive. The BIOS simply copies the first sector to the
magic address 0x7C00 and jumps into it.

This would typically contain an OS boot loader in hand written assembly that
fetches a bigger executable from the same disk. This would possibly be a
second stage boot that can read the file system and load the OS kernel.

Eventually somebody figured, it would be a jolly idea to have partitioned
hard disks. This was implemented by storing a partition table at the end of
the first sector. Instead of the original boot strap program, the first sector
now contains the "Master Boot Record" that is loaded & executed by the BIOS.
It relocates itself (typically to `0x0600`; that's what Microsoft went with),
inspects the partition table and chain loads the actual boot loader from the
first sector of a partition marked as bootable.

The BIOS code was left untouched, and doesn't need to know about partitions.
It's completely an OS feature, but the partition boot loader needs to be aware
of it, hence we started calling those "Volume Boot Record" (VBR).

The "standard" layout of the partition table is simply what DOS et al settled
on, using 4 entries, each 16 byte. This leaves us with 446 bytes of payload
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

So what the FAT32 VBR does, is chain-load a much larger program stored in
reserved sectors between the super block and the first FAT. We are guaranteed
at least 32 of them, but one holds special FS information, and there is
typically also a backup copy of the first sector (can be disabled).

If we max that out, we can have some whopping 30k for our second stage
boot loader.

In the `vbr` directory, there is a C++ program that does this, again using
common headers. The `installfat` directory contains C++ source for a program
that modifies a FAT image to include our VBR and second stage.

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
propper instruction prefix. You need to take this into account, when calling
from assembly into gcc generated code or vice versa.

## Computed Goto

gcc allows you to do this:

```C++
somelabel:
	auto *wtf = &&somelabel;

	goto *wtf;
```

or even funkier:

```C++
	goto ((void *)0x7c00);
```

I initially had the MBR written in C++ entirely, including the self relocating,
but ultimately replace the entry/exit code with an assembly stub instead.

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
