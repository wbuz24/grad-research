# Build a disk image for RISC-V ubuntu
This is for a full-system disk image

A list of dependencies can be found below

## setup env
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
sudo python3 util/gem5img.py mount riscv-ubuntu-22.04.img mnt
```

## If you need to unmount
You should unmount after you are done
```
sudo python3 util/gem5img.py umount mnt
```

## copy appropriate system files onto image
This is important for a few files that the image will need to borrow from a real system
```
sudo cp /etc/resolv.conf mnt/etc/ --remove-destination 
sudo /bin/mount -o bind /dev/null mnt/dev/null
```
## install files from within disk image
```
sudo chroot mnt qemu-riscv64-static /bin/bash
chmod 777 mnt
chown -R man: /var/cache/man/
```

```exit```  optional for full shell

## clone repository / transfer files
```
apt update
sudo apt install git
```
Now, clone any repo you like

## Dependencies

 - A statically compiled version of [RISC-V qemu](https://risc-v-getting-started-guide.readthedocs.io/en/latest/linux-qemu.html)
 - [RISC-V Toolchain](https://github.com/riscv-collab/riscv-gnu-toolchain)
