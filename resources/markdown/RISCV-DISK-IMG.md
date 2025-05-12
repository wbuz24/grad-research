# Build GAPBS for RISC-V

## setup env
```
mkdir mnt
```
## Load RISC-V disk image contents with gem5 binaries, etc.
## This can also be done from within a file, see gem5 resources
```
wget https://gem5resources.blob.core.windows.net/dist-gem5-org/dist/develop/images/riscv/riscv-ubuntu-22.04.gz
gzip -d riscv-ubuntu-22.04.gz
```

## If error, keep retrying
```
sudo python3 util/gem5img.py mount riscv-ubuntu-22.04.img mnt
```
## If you need to unmount
```
sudo python3 util/gem5img.py umount mnt
```
## copy appropriate system files onto image
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
