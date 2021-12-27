#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
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

uint64
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


#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  // lab pgtbl: your code here.
	uint64 fva;
	int pnum;
	uint64 abits;
	int res = 0;	
	pte_t* pte_addr;
	pte_t pte;
	// read the arguments from the stack
	if(argaddr(0, &fva) < 0) {
		return -1;
	}
	if(argint(1, &pnum) < 0) {
		return -1;
	}
	if(argaddr(2, &abits) < 0) {
		return -1;
	}

	pagetable_t pagetable = myproc()->pagetable;
	for(int i = 0; i < pnum; ++i) {
		pte_addr = walk(pagetable, fva, 0);
		pte = *pte_addr;
		if(pte & PTE_A) {
			(*pte_addr) = pte & ~(PTE_A);
			res |= (1 << i);
		}
		fva += PGSIZE;
	}
	if(copyout(pagetable, abits, (char *)&res, sizeof(res))<0) {
		return -1;
	}	
  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

