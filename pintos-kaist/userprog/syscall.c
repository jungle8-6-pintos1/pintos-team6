#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "threads/init.h"
#include "filesys/filesys.h"
#include "filesys/file.h"



void syscall_entry (void);
void syscall_handler (struct intr_frame *);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

///////////// - System Call - /////////////////
void sys_halt (void);
void sys_exit (int status);
tid_t sys_fork (const char *thread_name);
int sys_exec (const char *cmd_line);
int sys_wait (tid_t pid);
bool sys_create (const char *file, unsigned initial_size);
bool sys_remove (const char *file);
int sys_open (const char *file);
int sys_filesize (int fd);
int sys_read (int fd, void *buffer, unsigned size);
int sys_write (int fd, const void *buffer, unsigned size);
void sys_seek (int fd, unsigned position);
unsigned sys_tell (int fd);
void sys_close (int fd);
///////////// - System Call - /////////////////
void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);
	
	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

/* The main system call interface */
// 인수는 %rdi, %rsi, %rdx, %r10, %r8, %r9 순서로 전달 //
void
syscall_handler (struct intr_frame *f UNUSED) {
    // TODO: Your implementation goes here.
    switch (f->R.rax) {
        case 0:  // SYS_HALT
            sys_halt();
            break;
        case 1:  // SYS_EXIT
            sys_exit(f->R.rdi);
            break;
        case 2:  // SYS_FORK
            f->R.rax = sys_fork(f->R.rdi);
            break;
        case 3:  // SYS_EXEC
            f->R.rax = sys_exec(f->R.rdi);
            break;
        case 4:  // SYS_WAIT
            f->R.rax = sys_wait(f->R.rdi);
            break;
        case 5:  // SYS_CREATE
            f->R.rax = sys_create(f->R.rdi, f->R.rsi);
            break;
        case 6:  // SYS_REMOVE
            f->R.rax = sys_remove(f->R.rdi);
            break;
        case 7:  // SYS_OPEN
            f->R.rax = sys_open(f->R.rdi);
            break;
        case 8:  // SYS_FILESIZE
            f->R.rax = sys_filesize(f->R.rdi);
            break;
        case 9:  // SYS_READ
            f->R.rax = sys_read(f->R.rdi, f->R.rsi, f->R.rdx);
            break;
        case 10: // SYS_WRITE
            f->R.rax = sys_write(f->R.rdi, f->R.rsi, f->R.rdx);
            break;
        case 11: // SYS_SEEK
            sys_seek(f->R.rdi, f->R.rsi);
            break;
        case 12: // SYS_TELL
            f->R.rax = sys_tell(f->R.rdi);
            break;
        case 13: // SYS_CLOSE
            sys_close(f->R.rdi);
            break;
        case 14: // SYS_MMAP
            break;
        case 15: // SYS_MUNMAP
            break;
        case 16: // SYS_CHDIR
            break;
        case 17: // SYS_MKDIR
            break;
        case 18: // SYS_READDIR
            break;
        case 19: // SYS_ISDIR
            break;
        case 20: // SYS_INUMBER
            break;
        case 21: // SYS_SYMLINK
            break;
        case 22: // SYS_DUP2
            break;
        case 23: // SYS_MOUNT
            break;
        case 24: // SYS_UMOUNT
            break;
        default:
            // Unknown system call
            break;
    }
}

// SYS_HALT
void sys_halt (void){
	power_off();
}

// SYS_EXIT
void sys_exit (int status){
	printf("%s: exit(%d)\n",thread_current()->name ,status);
	thread_exit ();
}
// SYS_FORK
tid_t sys_fork (const char *thread_name){
	return;
}			
// SYS_EXEC
int sys_exec (const char *cmd_line)	{
	return;
}
// SYS_WAIT
int sys_wait (tid_t pid){
	return;
}	
// SYS_CREATE
bool sys_create (const char *file, unsigned initial_size){
	return;
}			
// SYS_REMOVE
bool sys_remove (const char *file){
	return;
}

// SYS_OPEN
int sys_open(const char *file) {
	if (file == NULL) return -1;

	struct file *f = filesys_open(file);
	if (f == NULL) return -1;

	struct thread *cur = thread_current();
	struct file **fdt = cur->fdt;

	for (int fd = 2; fd < 128; fd++) {
		if (fdt[fd] == NULL) {
			fdt[fd] = f;
			return fd;
		}
	}

	file_close(f);
	return -1;
}	

// SYS_FILESIZE
int sys_filesize (int fd){
	return;
}		
// SYS_READ
int sys_read (int fd, void *buffer, unsigned size){
    struct thread *cur = thread_current();
    struct file **fdt = cur->fdt;
    // 표준 입력(Standard Input) //
    if(fd == 0){
        return input_getc();
    }
    struct  file *f = cur->fdt[fd];

    if(f==NULL){
        return -1;
    }
    int read_size = file_read(f, buffer, size);
    if(read_size != size){
        return 0;
    }
    return read_size;
}	
// SYS_WRITE
int sys_write (int fd, const void *buffer, unsigned size){
	if(fd==1){
		putbuf(buffer, size);
		return size;
	}
	return -1;
}
// SYS_SEEK
void sys_seek (int fd, unsigned position){
	return;
}
// SYS_TELL
unsigned sys_tell (int fd){
	return;
}
// SYS_CLOSE
void sys_close (int fd){
	return;
}







