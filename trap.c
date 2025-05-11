#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;
  case T_PGFLT:
      myproc()->pending[11] = 1;
      break;
  case T_ILLOP:
      myproc()->pending[4] = 1;
      break;

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  for (int i=1;i<32;i++){
    if (myproc()==0){
      break;
    }
    if (myproc()->pending[i]==1 && myproc()->blocked[i]==0){
      if (i==19){
        myproc()->pending[i] = 0;
        myproc()->state = SLEEPING;
      }else{
        if (myproc()->handlers[i].flag ==1){
          myproc()->pending[i] = 0;
          copy_trapframe(myproc()->tf, myproc()); 
          tf -> esp = tf-> esp - 4;
          *(int *)(tf->esp+4) = i;
          tf -> eip = myproc()->handlers[i].functionptr;
        }
        else{
          myproc()->pending[i] = 0;
          myproc()->handlers[i].functionptr(i);
        }
      }
    }
  }

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}


void copy_trapframe(struct trapframe * tf, struct proc * p){
  p->backup.cs = tf->cs;
  p->backup.ds = tf->ds;
  p->backup.eax = tf->eax;
  p->backup.ebp = tf->ebp;
  p->backup.ebx = tf->ebx;
  p->backup.ecx = tf->ecx;
  p->backup.edi = tf->edi;
  p->backup.edx = tf->edx;
  p->backup.eflags = tf->eflags;
  p->backup.eip = tf->eip;
  p->backup.err = tf->err;
  p->backup.es = tf->es;
  p->backup.esi = tf->esi;
  p->backup.esp = tf->esp;
  p->backup.fs = tf->fs;
  p->backup.gs = tf->gs;
  p->backup.oesp = tf->oesp;
  p->backup.padding1 = tf->padding1;
  p->backup.padding2 = tf->padding2;
  p->backup.padding3 = tf->padding3;
  p->backup.padding4 = tf->padding4;
  p->backup.padding5 = tf->padding5;
  p->backup.padding6 = tf->padding6;
  p->backup.ss = tf->ss;
  p->backup.trapno = tf->trapno;
  return ;

}