#include <err.h>
#include <fcntl.h>
#include <linux/kvm.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#define MEM_SIZE	0x10000
#define LOAD_ADDR	0x1000

int main(int argc, char **argv)
{
	if (argc != 2)
		errx(1, "usage: %s guest.bin", argv[0]);

	int kvm = open("/dev/kvm", O_RDWR | O_CLOEXEC);
	if (kvm < 0)
		err(1, "/dev/kvm");
	if (ioctl(kvm, KVM_GET_API_VERSION, NULL) != KVM_API_VERSION)
		err(1, "unexpected KVM API version");

	int vm = ioctl(kvm, KVM_CREATE_VM, 0);
	if (vm < 0)
		err(1, "KVM_CREATE_VM");

	uint8_t *mem = mmap(NULL, MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if (mem == MAP_FAILED)
		err(1, "mmap guest memory");

	FILE *f = fopen(argv[1], "rb");
	if (!f)
		err(1, "%s", argv[1]);

	size_t n = fread(mem + LOAD_ADDR, 1, MEM_SIZE - LOAD_ADDR, f);
	fclose(f);
	if (n == 0)
		err(1, "empty guest image");

	struct kvm_userspace_memory_region region = {
		.slot = 0,
		.guest_phys_addr = 0,
		.memory_size = MEM_SIZE,
		.userspace_addr = (uint64_t) mem
	};
	if (ioctl(vm, KVM_SET_USER_MEMORY_REGION, &region) < 0)
		err(1, "KVM_SET_USER_MEMORY_REGION");

	int vcpu = ioctl(vm, KVM_CREATE_VCPU, 0);
	if (vcpu < 0)
		err(1, "KVM_CREATE_VCPU");

	int run_size = ioctl(kvm, KVM_GET_VCPU_MMAP_SIZE, NULL);
	if (run_size <= 0)
		err(1, "KVM_GET_VCPU_MMAP_SIZE");

	struct kvm_run *run = mmap(NULL, run_size, PROT_READ | PROT_WRITE, MAP_SHARED, vcpu, 0);
	if (run == MAP_FAILED)
		err(1, "mmap kvm_run");

	struct kvm_sregs sregs;
	if (ioctl(vcpu, KVM_GET_SREGS, &sregs) < 0)
		err(1, "KVM_GET_SREGS");
	sregs.cs.base = 0;
	sregs.cs.selector = 0;
	if (ioctl(vcpu, KVM_SET_SREGS, &sregs) < 0)
		err(1, "KVM_SET_SREGS");

	struct kvm_regs regs = {
		.rip = LOAD_ADDR,
		.rflags = 0x2
	};
	if (ioctl(vcpu, KVM_SET_REGS, &regs) < 0)
		err(1, "KVM_SET_REGS");

	unsigned int exits = 0;
	unsigned int io_exits = 0;
	for (;;)
	{
		if (ioctl(vcpu, KVM_RUN, NULL) < 0)
			err(1, "KVM_RUN");
		++exits;

		switch (run->exit_reason)
		{
		case KVM_EXIT_IO:
		{
			++io_exits;
			if (run->io.direction != KVM_EXIT_IO_OUT || run->io.size != 1)
				errx(1, "unsupported I/O on port 0x%x", run->io.port);
			uint8_t *data = (uint8_t *)run + run->io.data_offset;
			if (run->io.port == 0x3f8)
			{
				for (unsigned int i = 0; i < run->io.count; ++i)
					putchar(data[i]);
				fflush(stdout);
			} else if (run->io.port == 0xf4) {
				printf("[vmm] guest reported status %u\n", data[0]);
			} else {
				printf("[vmm] write to unknown port 0x%x\n", run->io.port);
			}
			break;
		}
		case KVM_EXIT_HLT:
			printf("[vmm] guest halted: %u exits total, %u I/O\n", exits, io_exits);
			return 0;
		case KVM_EXIT_FAIL_ENTRY:
			errx(1, "KVM entry failed, hw reasion 0x%llx", run->fail_entry.hardware_entry_failure_reason);
			break;
		case KVM_EXIT_INTERNAL_ERROR:
			errx(1, "KVM internal error, suberror %u", run->internal.suberror);
			break;
		default:
			errx(1, "unhandled exit reasion %u", run->exit_reason);
			break;
		}
	}
}
