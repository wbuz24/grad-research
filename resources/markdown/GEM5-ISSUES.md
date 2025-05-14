# Issues encountered with gem5

## Full-system
## Disk-Image
When running FS-mode, gem5 does not automatically point to the correct filesystem to search for the mounted disk image (Seen in both RISC-V & X86).

Within ```src/python/gem5/components/boards/riscv_board.py```, change the ```get_disk_device()``` function to point to ```/dev/vda1``` instead of "/dev/vda":
```
    @overrides(KernelDiskWorkload)
    def get_disk_device(self):
        return "/dev/vda"
```
