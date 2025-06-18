# Build a disk image for RISC-V ubuntu
This is for a full-system disk image for [gem5](https://www.gem5.org/) simulations.

A list of dependencies can be found below

## Setup env

Note: If you are starting from scratch, you can largely follow this in-order.

```
mkdir mnt
```
## Load RISC-V disk image contents with gem5 binaries, etc.
This can also be done from within a file, see gem5 resources

```
wget https://gem5resources.blob.core.windows.net/dist-gem5-org/dist/develop/images/riscv/riscv-ubuntu-22.04.gz
gzip -d riscv-ubuntu-22.04.gz
```

## Mount the image, if error, keep retrying
```
sudo python3 util/gem5img.py mount resources/binaries/riscv-ubuntu-22.04-img mnt
```

## If you need to unmount
You should unmount after you are done
```
sudo python3 util/gem5img.py umount mnt
```

## Copy appropriate system files onto image
This is important for a few files that the image will need to borrow from a real system
```
sudo cp /etc/resolv.conf mnt/etc/ --remove-destination 
sudo /bin/mount -o bind /dev/null mnt/dev/null
```

## Chroot into the image to open a shell within
Try:
```
sudo chroot mnt qemu-riscv64-static /bin/bash
```

If that does not work, you can chroot using:

```
sudo chroot mnt
```

## change permissions 
```
chmod 777 mnt
chown -R man: /var/cache/man/
```

```exit```  optional for full shell

## Update and clone repositories 

```
apt update
apt install git
```

Now, clone any repo you like

## For gem5
Note: When running your own disk image in gem5, you must first compile the [m5 utility](https://github.com/gem5/gem5/tree/stable/util/m5).

For riscv, you must also cross compile, this is done from the ~/gem5/util/m5 directory in a similar manner to how you build the gem5 binary.
```
scons riscv.CROSS_COMPILE=/home/wbuziak/../../opt/riscv/bin/riscv64-unknown-linux-gnu- build/riscv/out/m5
```

You will then want to copy this binary onto your disk image sbin/ folder, as per the [disk-image](https://www.gem5.org/documentation/general_docs/fullsystem/disks) documentation.

## Dependencies

 - A statically compiled version of [RISC-V qemu](https://risc-v-getting-started-guide.readthedocs.io/en/latest/linux-qemu.html)
 - [RISC-V Toolchain](https://github.com/riscv-collab/riscv-gnu-toolchain)
