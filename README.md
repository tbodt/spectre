# Spectre Cross-Process Read Demo

I've looked online for spectre POCs, and all I could find were copies of the one in the paper. That one demonstrates reading from your own process's memory, which is boring because you don't need spectre to read from your own memory. So I made my own that reads from other process's memory.

This demo attacks a simple TCP server. You can open a connection, send two whitespace separated numbers, and get the sum back. To have something interesting to read, this server requires a secret to start up, which is copied into a known location into its memory.

To run the demo, run `make`, then open two terminals and run `./target [port] [secret]` in one terminal. Then run `./attack.sh [port]`. You should see the secret you gave the target program slowly emerge. If it doesn't, I'll have you know it

![works on my machine](https://blog.codinghorror.com/content/images/uploads/2007/03/6a0120a85dcdae970b0128776ff992970c-pi.png)

My machine:

```
$ uname -a
Linux [redacted] 2.6.32-696.3.2.el6.x86_64 #1 SMP Tue Jun 20 01:26:55 UTC 2017 x86_64 x86_64 x86_64 GNU/Linux
$ cat /etc/centos-release
CentOS release 6.9 (Final)
$ lscpu
Architecture:          x86_64
CPU op-mode(s):        32-bit, 64-bit
Byte Order:            Little Endian
CPU(s):                8
On-line CPU(s) list:   0-7
Thread(s) per core:    2
Core(s) per socket:    4
Socket(s):             1
NUMA node(s):          1
Vendor ID:             GenuineIntel
CPU family:            6
Model:                 60
Model name:            Intel(R) Core(TM) i7-4790 CPU @ 3.60GHz
Stepping:              3
CPU MHz:               800.000
BogoMIPS:              7183.40
Virtualization:        VT-x
L1d cache:             32K
L1i cache:             32K
L2 cache:              256K
L3 cache:              8192K
NUMA node0 CPU(s):     0-7
```

## Caveats

  - Currently 64-bit only, though it could easily be made to also work on 32-bit.
  - The attack relies on the target program's code segment not being affected by ASLR, i.e. the target must not be compiled as a PIE (position independent executable). The Makefile includes the `-no-pie` to make sure of that.
  - The target program needs a few things inserted into it, namely a bit of assembler code and an unused readonly 64 KiB region. Both of these things could be instead found in the C library or any other dynamic library used by the target program.
  - It's slow as hell. I timed it and it took about a minute to read 11 bytes from the target.
  - It's kinda unreliable. Something like 10% of bytes you read are going to come through completely garbled. You can fix this by trying repeatedly, but that doesn't help with the "slow as hell" part.

## Writeup

Not yet written. I should get around to it tomorrow.
