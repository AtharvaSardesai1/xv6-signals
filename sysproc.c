#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_Kill(void)
{
  int pid;
  int signum;
  if(argint(0, &pid) < 0)
    return -1;
  if(argint(1, &signum) < 0)
    return -1;
  return Kill(pid, signum);
}

int sys_signal(void)
{
  int signum;
  sighandler_t handler;
  void * ptr;
  if(argint(0, &signum) < 0)
    return -1;
  if (argptr(1, (void *)&ptr, sizeof(void *)) < 0)
    return -1;
  handler = (sighandler_t )ptr;
  return signal(signum, handler);
}

int sys_sigprocmask(void)
{
  int how;
  int * signal_set;
  void * ptr;
  if(argint(0, &how) < 0)
    return -1;
  if (argptr(1, (void *)&ptr, sizeof(void *)) < 0) 
    return -1;
  signal_set = (int *) ptr;
  return sigprocmask(how, signal_set);
}

int sys_pause(void){
  return pause();
}

int sys_sigreturn(void){
  struct proc *p = myproc();
  *p->tf = p->backup;
  return 0;
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
