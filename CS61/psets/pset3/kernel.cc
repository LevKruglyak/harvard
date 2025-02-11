#include "kernel.hh"

#include <atomic>

#include "k-apic.hh"
#include "k-vmiter.hh"

// kernel.cc
//
//    This is the kernel.

// INITIAL PHYSICAL MEMORY LAYOUT
//
//  +-------------- Base Memory --------------+
//  v                                         v
// +-----+--------------------+----------------+--------------------+---------/
// |     | Kernel      Kernel |       :    I/O | App 1        App 1 | App 2
// |     | Code + Data  Stack |  ...  : Memory | Code + Data  Stack | Code ...
// +-----+--------------------+----------------+--------------------+---------/
// 0  0x40000              0x80000 0xA0000 0x100000             0x140000
//                                             ^
//                                             | \___ PROC_SIZE ___/
//                                      PROC_START_ADDR

#define PROC_SIZE 0x40000  // initial state only

proc ptable[NPROC];  // array of process descriptors
                     // Note that `ptable[0]` is never used.
proc* current;       // pointer to currently executing proc

#define HZ 100                            // timer interrupt frequency (interrupts/sec)
static std::atomic<unsigned long> ticks;  // # timer interrupts so far

// Memory state - see `kernel.hh`
physpageinfo physpages[NPAGES];

[[noreturn]] void schedule();
[[noreturn]] void run(proc* p);
void exception(regstate* regs);
uintptr_t syscall(regstate* regs);
void memshow();

// kernel_start(command)
//    Initialize the hardware and processes and start running. The `command`
//    string is an optional string passed from the boot loader.

static void process_setup(pid_t pid, const char* program_name);

void print_memory() {
    for (int i = 0; i < NPAGES; i++) {
        switch (physpages[i].refcount) {
            case 0:
                log_printf(" ");
                break;
            case 1:
                log_printf("P");
                break;
            case 0xFF:
                log_printf("K");
                break;
            default:
                continue;
        }

        if (i % 64 == 0 && i != 0) {
            log_printf("\n");
        }
    }

    log_printf("\n");
}

int num_free = 386;

int get_num_free() {
    int count = 0;

    for (int i = 0; i < NPAGES; i++) {
        if (physpages[i].refcount == 0) {
            count++;
        }
    }

    return count;
}

void kernel_start(const char* command) {
    // initialize hardware
    init_hardware();
    log_printf("Starting WeensyOS\n");

    ticks = 1;
    init_timer(HZ);

    // clear screen
    console_clear();

    // (re-)initialize kernel page table
    for (vmiter it(kernel_pagetable); it.va() < MEMSIZE_PHYSICAL; it += PAGESIZE) {
        if (it.va() == 0) {
            // nullptr is inaccessible even to the kernel
            it.map(it.va(), 0);
        } else if (it.va() >= PROC_START_ADDR || it.va() == CONSOLE_ADDR) {
            it.map(it.va(), PTE_P | PTE_W | PTE_U);
        } else {
            // Must be kernel memory
            it.map(it.va(), PTE_P | PTE_W);
        }

        if (!allocatable_physical_address(it.pa())) {
            physpages[it.va() / PAGESIZE].refcount = 0xFF;
        }
    }

    // set up process descriptors
    for (pid_t i = 0; i < NPROC; i++) {
        ptable[i].pid = i;
        ptable[i].state = P_FREE;
    }

    if (command && program_image::program_number(command) > 0) {
        process_setup(1, command);
    } else {
        process_setup(1, "allocator");
        process_setup(2, "allocator2");
        process_setup(3, "allocator3");
        process_setup(4, "allocator4");
    }

    log_printf("total free pages: %d\n", get_num_free());

    // Switch to the first process using run()
    run(&ptable[1]);
}

// kalloc(sz)
//    Kernel physical memory allocator. Allocates at least `sz` contiguous bytes
//    and returns a pointer to the allocated memory, or `nullptr` on failure.
//    The returned pointer’s address is a valid physical address, but since the
//    WeensyOS kernel uses an identity mapping for virtual memory, it is also
//    a valid virtual address that the kernel can access or modify.
//
//    The allocator selects from physical pages that can be allocated for
//    process use (so not reserved pages or kernel data), and from physical
//    pages that are currently unused (so `physpages[I].refcount == 0`).
//
//    On WeensyOS, `kalloc` is a page-based allocator: if `sz > PAGESIZE`
//    the allocation fails; if `sz < PAGESIZE` it allocates a whole page
//    anyway.
//
//    The handout code returns the next allocatable free page it can find.
//    It checks all pages. (You could maybe make this faster!)
//
//    The returned memory is initially filled with 0xCC, which corresponds to
//    the x86 instruction `int3`. This may help you debug.

void* kalloc(size_t sz) {
    if (sz > PAGESIZE) {
        return nullptr;
    }

    // print_num_free();

    for (uintptr_t pa = 0; pa != MEMSIZE_PHYSICAL; pa += PAGESIZE) {
        if (physpages[pa / PAGESIZE].refcount == 0) {
            physpages[pa / PAGESIZE].refcount = 1;
            memset((void*)pa, 0xCC, PAGESIZE);
            // log_printf("alloc %p\n", pa);
            num_free--;
            return (void*)pa;
        }
    }

    return nullptr;
}

// kfree(kptr)
//    Free `kptr`, which must have been previously returned by `kalloc`.
//    If `kptr == nullptr` does nothing.
void kfree(void* kptr) {
    if (kptr != nullptr) {
        uintptr_t ptr = reinterpret_cast<uintptr_t>(kptr);

        if (physpages[ptr / PAGESIZE].refcount == 1) {
            physpages[ptr / PAGESIZE].refcount = 0;
            num_free++;
        } else {
            // log_printf("tried to free invalid memory\n");
        }
    }
}

uintptr_t virtual_kalloc(x86_64_pagetable* pt, uintptr_t vm) {
    void* pa = kalloc(PAGESIZE);

    if (pa != nullptr) {
        // Clear memory
        memset(pa, 0, PAGESIZE);

        // Create virtual memory mapping
        int r = vmiter(pt, vm).try_map(pa, PTE_P | PTE_W | PTE_U);
        if (r != 0) {
            log_printf("failed allocating %p at %p\n", vm, pa);
            print_memory();
            return 0;
        }

        return reinterpret_cast<uintptr_t>(pa);
    }

    return 0;
}

// process_setup(pid, program_name)
//    Load application program `program_name` as process number `pid`.
//    This loads the application's code and data into memory, sets its
//    %rip and %rsp, gives it a stack page, and marks it as runnable.
int allocated_memory = 0;
void process_setup(pid_t pid, const char* program_name) {
    init_process(&ptable[pid], 0);
    allocated_memory = 0;

    // initialize process page table
    ptable[pid].pagetable = kalloc_pagetable();

    // copy kernel permissions to process pagetable
    vmiter ke(kernel_pagetable);
    for (vmiter it(ptable[pid].pagetable); it.va() < PROC_START_ADDR; it += PAGESIZE) {
        int r = it.try_map(it.va(), ke.perm());

        if (r != 0) {
            print_memory();
        }

        ke += PAGESIZE;
    }

    // obtain reference to the program image
    program_image pgm(program_name);

    // allocate and map global memory required by loadable segments
    for (auto seg = pgm.begin(); seg != pgm.end(); ++seg) {
        for (uintptr_t a = round_down(seg.va(), PAGESIZE);
             a < seg.va() + seg.size();
             a += PAGESIZE) {
            uintptr_t pm = virtual_kalloc(ptable[pid].pagetable, a);
            assert((void*)pm != nullptr);
            allocated_memory++;
        }
    }

    for (auto seg = pgm.begin(); seg != pgm.end(); ++seg) {
        vmiter pg(ptable[pid].pagetable, seg.va());
        memset((void*)pg.pa(), 0, seg.size());
        memcpy((void*)pg.pa(), seg.data(), seg.data_size());
    }

    // mark entry point
    ptable[pid].regs.reg_rip = pgm.entry();

    // allocate and map stack segment
    // Compute process virtual address for stack page
    uintptr_t stack_addr = MEMSIZE_VIRTUAL - PAGESIZE;
    // map stack segment
    uintptr_t pm = virtual_kalloc(ptable[pid].pagetable, stack_addr);
    assert((void*)pm != nullptr);
    allocated_memory++;
    ptable[pid].regs.reg_rsp = stack_addr + PAGESIZE;

    // mark process as runnable
    ptable[pid].state = P_RUNNABLE;

    log_printf("allocated %d for %d\n", allocated_memory, current->pid);
}

// exception(regs)
//    Exception handler (for interrupts, traps, and faults).
//
//    The register values from exception time are stored in `regs`.
//    The processor responds to an exception by saving application state on
//    the kernel's stack, then jumping to kernel assembly code (in
//    k-exception.S). That code saves more registers on the kernel's stack,
//    then calls exception().
//
//    Note that hardware interrupts are disabled when the kernel is running.

void exception(regstate* regs) {
    // Copy the saved registers into the `current` process descriptor.
    current->regs = *regs;
    regs = &current->regs;

    // It can be useful to log events using `log_printf`.
    // Events logged this way are stored in the host's `log.txt` file.
    /* log_printf("proc %d: exception %d at rip %p\n",
                current->pid, regs->reg_intno, regs->reg_rip); */

    // Show the current cursor location and memory state
    // (unless this is a kernel fault).
    console_show_cursor(cursorpos);
    if (regs->reg_intno != INT_PF || (regs->reg_errcode & PTE_U)) {
        memshow();
    }

    // If Control-C was typed, exit the virtual machine.
    check_keyboard();

    // Actually handle the exception.
    switch (regs->reg_intno) {
        case INT_IRQ + IRQ_TIMER:
            ++ticks;
            lapicstate::get().ack();
            schedule();
            break; /* will not be reached */

        case INT_PF: {
            // Analyze faulting address and access type.
            uintptr_t addr = rdcr2();
            const char* operation = regs->reg_errcode & PTE_W
                                        ? "write"
                                        : "read";
            const char* problem = regs->reg_errcode & PTE_P
                                      ? "protection problem"
                                      : "missing page";

            if (!(regs->reg_errcode & PTE_U)) {
                panic("Kernel page fault on %p (%s %s)!\n",
                      addr, operation, problem);
            }
            console_printf(CPOS(24, 0), 0x0C00,
                           "Process %d page fault on %p (%s %s, rip=%p)!\n",
                           current->pid, addr, operation, problem, regs->reg_rip);
            current->state = P_FAULTED;
            break;
        }

        default:
            panic("Unexpected exception %d!\n", regs->reg_intno);
    }

    // Return to the current process (or run something else).
    if (current->state == P_RUNNABLE) {
        run(current);
    } else {
        schedule();
    }
}

int syscall_page_alloc(uintptr_t addr);
int syscall_fork(regstate* regs);
void syscall_exit(regstate* regs);

// syscall(regs)
//    System call handler.
//
//    The register values from system call time are stored in `regs`.
//    The return value, if any, is returned to the user process in `%rax`.
//
//    Note that hardware interrupts are disabled when the kernel is running.
uintptr_t syscall(regstate* regs) {
    // Copy the saved registers into the `current` process descriptor.
    current->regs = *regs;
    regs = &current->regs;

    // It can be useful to log events using `log_printf`.
    // Events logged this way are stored in the host's `log.txt` file.
    // log_printf("proc %d: syscall %d at rip %p\n",
    //            current->pid, regs->reg_rax, regs->reg_rip);

    // Show the current cursor location and memory state.
    console_show_cursor(cursorpos);
    memshow();

    // If Control-C was typed, exit the virtual machine.
    check_keyboard();

    // Actually handle the exception.
    switch (regs->reg_rax) {
        case SYSCALL_PANIC:
            user_panic(current);  // does not return

        case SYSCALL_GETPID:
            return current->pid;

        case SYSCALL_YIELD:
            current->regs.reg_rax = 0;
            schedule();  // does not return

        case SYSCALL_PAGE_ALLOC:
            return syscall_page_alloc(current->regs.reg_rdi);

        case SYSCALL_FORK:
            return syscall_fork(regs);

        case SYSCALL_EXIT:
            syscall_exit(regs);
            schedule();

        default:
            panic("Unexpected system call %ld!\n", regs->reg_rax);
    }

    panic("Should not get here!\n");
}

void syscall_exit(regstate* regs) {
    // Mark process as free
    current->state = P_FREE;
    log_printf("exiting %d\n", current->pid);

    int num_frees = 0;

    for (vmiter it(ptable[current->pid].pagetable); it.va() < MEMSIZE_VIRTUAL; it += PAGESIZE) {
        if (it.perm() & PTE_U) {
            if (physpages[it.pa() / PAGESIZE].refcount == 1) {
                num_frees++;
            }
            kfree((void*)it.pa());
        }
    }

    log_printf("freed %d for process %d\n", num_frees, current->pid);
}

int syscall_fork(regstate* regs) {
    for (int pid = 1; pid < NPAGES; pid++) {
        if (ptable[pid].state == P_FREE) {
            // Copy page table
            ptable[pid].pagetable = kalloc_pagetable();

            // Copy from parent process
            vmiter ba(ptable[current->pid].pagetable);
            for (vmiter it(ptable[pid].pagetable); it.va() < MEMSIZE_VIRTUAL; it += PAGESIZE) {
                if (it.va() < PROC_START_ADDR) {
                    int r = it.try_map(ba.pa(), ba.perm());
                } else {
                    if (ba.perm() & PTE_U) {
                        uintptr_t pm = virtual_kalloc(ptable[pid].pagetable, it.va());

                        if (pm == 0) {
                            // Go back and free everything
                            while (it.va() >= PROC_START_ADDR) {
                                kfree((void*)it.pa());
                                it -= PAGESIZE;
                            }

                            return -1;
                        }

                        memcpy((void*)pm, (void*)ba.pa(), PAGESIZE);
                    }
                }
                ba += PAGESIZE;
            }

            ptable[pid].regs = *regs;
            ptable[pid].state = P_RUNNABLE;

            // log_printf("forked process into %d\n", pid);
            //  Return zero to child process
            ptable[pid].regs.reg_rax = 0;

            return pid;
        }
    }

    return -1;
}

// syscall_page_alloc(addr)
//    Handles the SYSCALL_PAGE_ALLOC system call. This function
//    should implement the specification for `sys_page_alloc`
//    in `u-lib.hh` (but in the handout code, it does not).
int syscall_page_alloc(uintptr_t addr) {
    if (virtual_kalloc(current->pagetable, addr) == 0) {
        return -1;
    }
    return 0;
}

// schedule
//    Pick the next process to run and then run it.
//    If there are no runnable processes, spins forever.

void schedule() {
    pid_t pid = current->pid;
    for (unsigned spins = 1; true; ++spins) {
        pid = (pid + 1) % NPROC;
        if (ptable[pid].state == P_RUNNABLE) {
            run(&ptable[pid]);
        }

        // If Control-C was typed, exit the virtual machine.
        check_keyboard();

        // If spinning forever, show the memviewer.
        if (spins % (1 << 12) == 0) {
            memshow();
            // log_printf("%u\n", spins);
        }
    }
}

// run(p)
//    Run process `p`. This involves setting `current = p` and calling
//    `exception_return` to restore its page table and registers.

void run(proc* p) {
    assert(p->state == P_RUNNABLE);
    current = p;

    // Check the process's current pagetable.
    check_pagetable(p->pagetable);

    // This function is defined in k-exception.S. It restores the process's
    // registers then jumps back to user mode.
    exception_return(p);

    // should never get here
    while (true) {
    }
}

// memshow()
//    Draw a picture of memory (physical and virtual) on the CGA console.
//    Switches to a new process's virtual memory map every 0.25 sec.
//    Uses `console_memviewer()`, a function defined in `k-memviewer.cc`.

void memshow() {
    static unsigned last_ticks = 0;
    static int showing = 0;

    // switch to a new process every 0.25 sec
    if (last_ticks == 0 || ticks - last_ticks >= HZ / 2) {
        last_ticks = ticks;
        showing = (showing + 1) % NPROC;
    }

    proc* p = nullptr;
    for (int search = 0; !p && search < NPROC; ++search) {
        if (ptable[showing].state != P_FREE && ptable[showing].pagetable) {
            p = &ptable[showing];
        } else {
            showing = (showing + 1) % NPROC;
        }
    }

    console_memviewer(p);
    if (!p) {
        console_printf(CPOS(10, 29), 0x0F00,
                       "VIRTUAL ADDRESS SPACE\n"
                       "                          [All processes have exited]\n"
                       "\n\n\n\n\n\n\n\n\n\n\n");
    }
}
