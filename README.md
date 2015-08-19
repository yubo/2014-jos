### Links
```
http://pdosnew.csail.mit.edu/6.828/2014/labs/lab6/

Clone the IAP 6.828 QEMU git repository git clone https://github.com/geofft/qemu.git -b 6.828-1.7.0
On Linux, you may need to install the SDL development libraries to get a graphical VGA window. On Debian/Ubuntu, this is the libsdl1.2-dev package.
Configure the source code
Linux: ./configure --disable-kvm [--prefix=PFX] [--target-list="i386-softmmu x86_64-softmmu"]
OS X: ./configure --disable-kvm --disable-sdl [--prefix=PFX] [--target-list="i386-softmmu x86_64-softmmu"] The prefix argument specifies where to install QEMU; without it QEMU will install to /usr/local by default. The target-list argument simply slims down the architectures QEMU will build support for.
Run make && make install
```

##### Install the 6.828 version of QEMU.

QEMU is a modern and fast PC emulator. QEMU version 1.7.0 is set up on Athena for x86 machines in the 6.828 locker (add -f 6.828)

Unfortunately, QEMU's debugging facilities, while powerful, are somewhat immature, so we highly recommend you use our patched version of QEMU instead of the stock version that may come with your distribution. The version installed on Athena is already patched. To build your own patched version of QEMU:

  - Clone the IAP 6.828 QEMU git repository `git clone https://github.com/geofft/qemu.git -b 6.828-1.7.0`
  - On Linux, you may need to install the SDL development libraries to get a graphical VGA window. On Debian/Ubuntu, this is the libsdl1.2-dev package.
  - Configure the source code
    - Linux: ./configure --disable-kvm [--prefix=PFX] [--target-list="i386-softmmu x86_64-softmmu"]
    - OS X: ./configure --disable-kvm --disable-sdl [--prefix=PFX] [--target-list="i386-softmmu x86_64-softmmu"] The prefix argument specifies where to install QEMU; without it QEMU will install to /usr/local by default. The target-list argument simply slims down the architectures QEMU will build support for.
  - Run make && make install
