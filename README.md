# linux-kvm-lab

Small C projects exploring Linux kernel internals and KVM.

| Project | Description |
|---|---|
| [`hello-module/`](hello-module/) | Minimal loadable kernel module |
| [`counter-module/`](counter-module/) | Kernel module exposing a read/write counter at `/proc/hello` |
| [`mini-vmm/`](mini-vmm/) | Minimal hypervisor using the KVM API directly |

## Environment

Everything runs on Windows through WSL2 with nested virtualization enabled:

```
Windows / Hyper-V
  └─ WSL2 Ubuntu            runs KVM, QEMU and mini-vmm
       └─ Ubuntu Server VM  QEMU/KVM guest used to build and load the modules
```

## hello-module

Logs a message on load and unload.

```bash
make
sudo insmod hello.ko && sudo dmesg | tail -1
sudo rmmod hello
```

## counter-module

Each read of `/proc/hello` increments and returns a counter; writing a number resets it. Uses `proc_ops`, `copy_from_user` and an atomic counter.

```bash
make && sudo insmod counter.ko
cat /proc/hello                  # 1
echo 100 | sudo tee /proc/hello
cat /proc/hello                  # 101
sudo rmmod counter
```

## mini-vmm

Creates a VM through `/dev/kvm`, loads a 16-bit real-mode guest into memory and runs it. The guest prints a string through I/O port `0x3f8` and halts; the VMM handles each I/O and HLT exit itself. Each byte printed causes one VM exit, which is why paravirtualized I/O such as virtio exists.

Run in WSL2 (requires `/dev/kvm`):

```bash
make
./vmm guest.bin
```
