#include "x86.h"
#include "device.h"
#include "fs.h"

#define O_WRITE 0x01
#define O_READ 0x02
#define O_CREATE 0x04
#define O_DIRECTORY 0x08

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define SYS_WRITE 0
#define SYS_FORK 1
#define SYS_EXEC 2
#define SYS_SLEEP 3
#define SYS_EXIT 4
#define SYS_READ 5
#define SYS_SEM 6
#define SYS_GETPID 7
#define SYS_OPEN 8
#define SYS_LSEEK 9
#define SYS_CLOSE 10
#define SYS_REMOVE 11

#define STD_OUT 0
#define STD_IN 1
#define SH_MEM 3

#define SEM_INIT 0
#define SEM_WAIT 1
#define SEM_POST 2
#define SEM_DESTROY 3

extern TSS tss;
extern ProcessTable pcb[MAX_PCB_NUM];
extern int current;

extern Semaphore sem[MAX_SEM_NUM];
extern Device dev[MAX_DEV_NUM];
extern File file[MAX_FILE_NUM];
extern SuperBlock sBlock;

extern int displayRow;
extern int displayCol;

extern uint32_t keyBuffer[MAX_KEYBUFFER_SIZE];
extern int bufferHead;
extern int bufferTail;

uint8_t shMem[MAX_SHMEM_SIZE];

void syscallHandle(struct TrapFrame *tf);
void syscallWrite(struct TrapFrame *tf);
void syscallRead(struct TrapFrame *tf);
void syscallFork(struct TrapFrame *tf);
void syscallExec(struct TrapFrame *tf);
void syscallSleep(struct TrapFrame *tf);
void syscallExit(struct TrapFrame *tf);
void syscallSem(struct TrapFrame *tf);
void syscallGetPid(struct TrapFrame *tf);
void syscallOpen(struct TrapFrame *tf);
void syscallLseek(struct TrapFrame *tf);
void syscallClose(struct TrapFrame *tf);
void syscallRemove(struct TrapFrame *tf);

void syscallWriteStdOut(struct TrapFrame *tf);
void syscallReadStdIn(struct TrapFrame *tf);
void syscallWriteShMem(struct TrapFrame *tf);
void syscallReadShMem(struct TrapFrame *tf);

void GProtectFaultHandle(struct TrapFrame *tf);

void timerHandle(struct TrapFrame *tf);
void keyboardHandle(struct TrapFrame *tf);

void syscallSemInit(struct TrapFrame *tf);
void syscallSemWait(struct TrapFrame *tf);
void syscallSemPost(struct TrapFrame *tf);
void syscallSemDestroy(struct TrapFrame *tf);

void irqHandle(struct TrapFrame *tf) { // pointer tf = esp
	/*
	 * 中断处理程序
	 */
	/* Reassign segment register */
	asm volatile("movw %%ax, %%ds"::"a"(KSEL(SEG_KDATA)));

	uint32_t tmpStackTop = pcb[current].stackTop;
	pcb[current].prevStackTop = pcb[current].stackTop;
	pcb[current].stackTop = (uint32_t)tf;

	switch(tf->irq) {
		case -1:
			break;
		case 0xd:
			GProtectFaultHandle(tf); // return
			break;
		case 0x20:
			timerHandle(tf);         // return or iret
			break;
		case 0x21:
			keyboardHandle(tf);      // return
			break;
		case 0x80:
			syscallHandle(tf);       // return
			break;
		default:assert(0);
	}

	pcb[current].stackTop = tmpStackTop;
}

void syscallHandle(struct TrapFrame *tf) {
	switch(tf->eax) { // syscall number
		case SYS_WRITE:
			syscallWrite(tf);
			break; // for SYS_WRITE
		case SYS_READ:
			syscallRead(tf);
			break; // for SYS_READ
		case SYS_FORK:
			syscallFork(tf);
			break; // for SYS_FORK
		case SYS_EXEC:
			syscallExec(tf);
			break; // for SYS_EXEC
		case SYS_SLEEP:
			syscallSleep(tf);
			break; // for SYS_SLEEP
		case SYS_EXIT:
			syscallExit(tf);
			break; // for SYS_EXIT
		case SYS_SEM:
			syscallSem(tf);
			break; // for SYS_SEM
		case SYS_GETPID:
			syscallGetPid(tf);
			break; // for SYS_GETPID
		case SYS_OPEN:
			syscallOpen(tf);
			break; // for SYS_OPEN
		case SYS_LSEEK:
			syscallLseek(tf);
			break; // for SYS_SEEK
		case SYS_CLOSE:
			syscallClose(tf);
			break; // for SYS_CLOSE
		case SYS_REMOVE:
			syscallRemove(tf);
			break; // for SYS_REMOVE
		default:break;
	}
}

void timerHandle(struct TrapFrame *tf) {
	uint32_t tmpStackTop;
	int i = (current + 1) % MAX_PCB_NUM;
	while (i != current) {
		if (pcb[i].state == STATE_BLOCKED) {
			pcb[i].sleepTime--;
			if (pcb[i].sleepTime == 0) {
				pcb[i].state = STATE_RUNNABLE;
			}
		}
		i = (i + 1) % MAX_PCB_NUM;
	}

	if (pcb[current].state == STATE_RUNNING &&
			pcb[current].timeCount != MAX_TIME_COUNT) {
		pcb[current].timeCount++;
		return;
	}
	else {
		if (pcb[current].state == STATE_RUNNING) {
			pcb[current].state = STATE_RUNNABLE;
			pcb[current].timeCount = 0;
		}
		i = (current + 1) % MAX_PCB_NUM;
		while (i != current) {
			if (i != 0 && pcb[i].state == STATE_RUNNABLE) {
				break;
			}
			i = (i + 1) % MAX_PCB_NUM;
		}
		if (pcb[i].state != STATE_RUNNABLE) {
			i = 0;
		}
		current = i;
		// putChar('0' + current);
		pcb[current].state = STATE_RUNNING;
		pcb[current].timeCount = 1;
		tmpStackTop = pcb[current].stackTop;
		pcb[current].stackTop = pcb[current].prevStackTop;
		tss.esp0 = (uint32_t)&(pcb[current].stackTop);
		asm volatile("movl %0, %%esp"::"m"(tmpStackTop)); // switch kernel stack
		asm volatile("popl %gs");
		asm volatile("popl %fs");
		asm volatile("popl %es");
		asm volatile("popl %ds");
		asm volatile("popal");
		asm volatile("addl $8, %esp");
		asm volatile("iret");
	}
	return;
}

void keyboardHandle(struct TrapFrame *tf) {
	// TODO in lab4
	//Put the keycode into keyBuffer
	uint32_t keyCode = getKeyCode();
	if (keyCode == 0)
		return;
	keyBuffer[bufferTail] = keyCode;
	bufferTail = (bufferTail + 1) % MAX_KEYBUFFER_SIZE;

	//Wake up a process in dev
	if(dev[STD_IN].value < 0){  //Exist a process that is blocked
		ProcessTable *pt = (ProcessTable*)((uint32_t)(dev[STD_IN].pcb.prev) - (uint32_t)&(((ProcessTable*)0)->blocked));
 		dev[STD_IN].pcb.prev = (dev[STD_IN].pcb.prev)->prev;
 		(dev[STD_IN].pcb.prev)->next = &(dev[STD_IN].pcb);

		dev[STD_IN].value++;
		pt->state = STATE_RUNNABLE;
		pt->sleepTime = 0;
	}
	
	return;
}

void syscallWrite(struct TrapFrame *tf) {
	/*switch(tf->ecx) { // file descriptor
		case STD_OUT:
			if (dev[STD_OUT].state == 1) {
				syscallWriteStdOut(tf);
			}
			break; // for STD_OUT
		case SH_MEM:
			if (dev[SH_MEM].state == 1) {
				syscallWriteShMem(tf);
			}
			break; // for SH_MEM
		default:break;
	} */
	if(tf->ecx == STD_OUT){
		if (dev[STD_OUT].state == 1) {
			syscallWriteStdOut(tf);
		}
	}
	else{  //All other file
		if(tf->ecx - MAX_DEV_NUM >= 0 && tf->ecx - MAX_DEV_NUM < MAX_FILE_NUM)
			if (file[tf->ecx - MAX_DEV_NUM].state == 1) {
				//syscallWriteShMem(tf); 
	int fd = tf->ecx;
	int flags = tf->edx;
	uint8_t write_permission = flags&O_WRITE;
	if(write_permission == 0){
		pcb[current].regs.eax=-1;
	   	return;
	}

	Inode inode;
	diskRead(&inode, sizeof(Inode), 1, file[fd-MAX_DEV_NUM].inodeOffset);

	int size = tf->ebx;
	if(size < 0){
		pcb[current].regs.eax=-1;
	   	return;
	}
	//int sel = tf->ds;
	uint8_t *buffer = (uint8_t *)tf->edx;
	//uint8_t tmp[SECTOR_NUM*SECTOR_SIZE];
	uint8_t tmp;
	int index = file[fd-MAX_DEV_NUM].inodeOffset/sBlock.blockSize;
	int cur = file[fd-MAX_DEV_NUM].inodeOffset%sBlock.blockSize;
	//uint8_t ret = 0;
	//asm volatile("movw %0, %%es"::"m"(sel));
	//asm volatile("movb %%es:(%1), %0":"=r"(*dst):"r"(buffer + cur));
	tmp = buffer[cur];
	file[0].inodeOffset++;
	writeBlock(&sBlock,&inode,index,&tmp);
	pcb[current].regs.eax = 1;
			}
	}
}

void syscallWriteStdOut(struct TrapFrame *tf) {
	int sel = tf->ds; //TODO segment selector for user data, need further modification
	char *str = (char *)tf->edx;
	int size = tf->ebx;
	int i = 0;
	int pos = 0;
	char character = 0;
	uint16_t data = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	for (i = 0; i < size; i++) {
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
		if (character == '\n') {
			displayRow++;
			displayCol = 0;
			if (displayRow == 25){
				displayRow = 24;
				displayCol = 0;
				scrollScreen();
			}
		}
		else {
			data = character | (0x0c << 8);
			pos = (80 * displayRow + displayCol) * 2;
			asm volatile("movw %0, (%1)"::"r"(data),"r"(pos + 0xb8000));
			displayCol++;
			if (displayCol == 80){
				displayRow++;
				displayCol = 0;
				if (displayRow == 25){
					displayRow = 24;
					displayCol = 0;
					scrollScreen();
				}
			}
		}
	}
	
	updateCursor(displayRow, displayCol);
	//TODO take care of return value
}

void syscallWriteShMem(struct TrapFrame *tf) {
	putString("Come into syscallWriteShMem");
	int fd = tf->ecx;
	int flags = tf->edx;
	uint8_t write_permission = flags&O_WRITE;
	if(write_permission == 0){
		pcb[current].regs.eax=-1;
	   	return;
	}

	Inode inode;
	diskRead(&inode, sizeof(Inode), 1, file[fd-MAX_DEV_NUM].inodeOffset);

	int size = tf->ebx;
	if(size < 0){
		pcb[current].regs.eax=-1;
	   	return;
	}
	int sel = tf->ds;
	uint8_t *buffer = (uint8_t *)tf->edx;
	uint8_t tmp[SECTOR_NUM*SECTOR_SIZE];
	uint8_t *dst = tmp;
	int index = file[fd-MAX_DEV_NUM].inodeOffset/sBlock.blockSize;
	int cur = file[fd-MAX_DEV_NUM].inodeOffset%sBlock.blockSize;
	int realSize = 0;
	uint8_t ret = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	while(realSize < size){
		for(; cur < SECTOR_NUM*SECTOR_SIZE; cur++){
			asm volatile("movb %%es:(%1), %0":"=r"(*dst):"r"(buffer + cur));
			dst++;
			//tmp[realSize] = buffer[cur];
			file[fd-MAX_DEV_NUM].inodeOffset++;
			realSize++;
			if(realSize == size){
				pcb[current].regs.eax = realSize;
	   			return;
			}
		}
		cur = 0;
		ret = writeBlock(&sBlock,&inode,index,tmp);
		index++;
		if(ret == -1){
			pcb[current].regs.eax=realSize;
			return;
		}
		if(index>=inode.blockCount){
			ret=allocBlock(&sBlock,&inode,file[fd-MAX_DEV_NUM].inodeOffset);
			if(ret==-1){
				pcb[current].regs.eax = realSize;
	   			return;
			}
		}
	}
	pcb[current].regs.eax = realSize;
	return;
}

void syscallRead(struct TrapFrame *tf) {
	/*switch(tf->ecx) {
		case STD_IN:
			if (dev[STD_IN].state == 1) {
				syscallReadStdIn(tf);
			}
			break;
		case SH_MEM:
			if (dev[SH_MEM].state == 1) {
				syscallReadShMem(tf);
			}
			break;
		default:
			break;
	} */
	if(tf->ecx == STD_IN){
		if (dev[STD_IN].state == 1) {
			syscallReadStdIn(tf);
		}
	}
	else{  //All other file
		if(tf->ecx - MAX_DEV_NUM >= 0 && tf->ecx - MAX_DEV_NUM < MAX_FILE_NUM){
			if (file[tf->ecx - MAX_DEV_NUM].state == 1) {
				//syscallReadShMem(tf);
				putString("Come into syscallReadShMem\n");

	Inode inode;
	diskRead(&inode, sizeof(Inode), 1, file[0].inodeOffset);
/*uint8_t *buffer = (uint8_t *)tf->edx;
	uint8_t tmp[SECTOR_NUM*SECTOR_SIZE];
	int index = file[0].inodeOffset/sBlock.blockSize;
	int cur = file[0].inodeOffset%sBlock.blockSize;
	//int sel = tf->ds;
	//asm volatile("movw %0, %%es"::"m"(sel));
	readBlock(&sBlock,&inode,index,tmp);
	//asm volatile("movb %0, %%es:(%1)"::"r"(tmp+cur),"r"(buffer));
	buffer[0] = tmp[cur];
	file[0].inodeOffset++; */
	
	}
	pcb[current].regs.eax = 1;
			
		}
			
	}
}

void syscallReadStdIn(struct TrapFrame *tf) {
	// TODO in lab4
	if(dev[STD_IN].value==0){  
		//putString("Be blocked\n");
		//Block the current process on dev[STD_IN]
		pcb[current].blocked.next = dev[STD_IN].pcb.next;
 		pcb[current].blocked.prev = &(dev[STD_IN].pcb);
 		dev[STD_IN].pcb.next = &(pcb[current].blocked);
 		(pcb[current].blocked.next)->prev = &(pcb[current].blocked);
		dev[STD_IN].value--;
		pcb[current].state = STATE_BLOCKED;
		pcb[current].sleepTime = -1; //Must be a value that <= 0, otherwise the blocked process may be woken up in TimeHandle
		asm volatile("int $0x20");

		//putString("Wake up\n");
		int sel = tf->ds;
		char *str = (char *)tf->edx;
		int size = tf->ebx;
		int i = 0;
		char character = 0;
		asm volatile("movw %0, %%es"::"m"(sel));
		for (i = 0; i < size && bufferHead!=bufferTail; i++) {
			character = getChar(keyBuffer[bufferHead]);
			putChar(character);
			bufferHead = (bufferHead + 1) % MAX_KEYBUFFER_SIZE;
			if(character!=0){
				asm volatile("movb %0, %%es:(%1)"::"r"(character),"r"(str + i));
			}
			else i--;
		}
		asm volatile("movb $0x00, %%es:(%0)"::"r"(str+i));
		pcb[current].regs.eax = i;
	}
	else{  //At one time, there is only one process can be blocked on dev[STD_IN]
		pcb[current].regs.eax = -1;
	}
	return;
}

void syscallReadShMem(struct TrapFrame *tf) {
	//putString("aaaaa");
	int fd = tf->ecx;
	int flags = tf->edx;
	uint8_t read_permission = flags&O_READ;
	if(read_permission == 0){
		pcb[current].regs.eax=-1;
	   	return;
	}

	Inode inode;
	diskRead(&inode, sizeof(Inode), 1, file[fd-MAX_DEV_NUM].inodeOffset);

	int size = tf->ebx;
	if(size < 0){
		pcb[current].regs.eax=-1;
	   	return;
	}
	if(size > inode.size-file[fd-MAX_DEV_NUM].inodeOffset){
		size = inode.size-file[fd-MAX_DEV_NUM].inodeOffset;
	}
	uint8_t *buffer = (uint8_t *)tf->edx;
	uint8_t tmp[SECTOR_NUM*SECTOR_SIZE];
	int index = file[fd-MAX_DEV_NUM].inodeOffset/sBlock.blockSize;
	int cur = file[fd-MAX_DEV_NUM].inodeOffset%sBlock.blockSize;
	int realSize = 0;
	uint8_t ret = 0;
	int sel = tf->ds;
	asm volatile("movw %0, %%es"::"m"(sel));
	while(realSize < size){
		ret = readBlock(&sBlock,&inode,index,tmp);
		index++;
		if(ret == -1){
			pcb[current].regs.eax=realSize;
			return;
		}
		for(; cur < SECTOR_NUM*SECTOR_SIZE; cur++){
			asm volatile("movb %0, %%es:(%1)"::"r"(tmp + cur),"r"(buffer + realSize));
			buffer[realSize] = tmp[cur];
			file[fd-MAX_DEV_NUM].inodeOffset++;
			realSize++;
			if(realSize == size){
				pcb[current].regs.eax = size;
	   			return;
			}
		}
		cur = 0;
	}
	pcb[current].regs.eax = realSize;
	return;
}

void syscallFork(struct TrapFrame *tf) {
	int i, j;
	for (i = 0; i < MAX_PCB_NUM; i++) {
		if (pcb[i].state == STATE_DEAD) {
			break;
		}
	}
	if (i != MAX_PCB_NUM) {
		pcb[i].state = STATE_PREPARING;

		enableInterrupt();
		for (j = 0; j < 0x100000; j++) {
			*(uint8_t *)(j + (i + 1) * 0x100000) = *(uint8_t *)(j + (current + 1) * 0x100000);
		}
		disableInterrupt();

		pcb[i].stackTop = (uint32_t)&(pcb[i].stackTop) -
			((uint32_t)&(pcb[current].stackTop) - pcb[current].stackTop);
		pcb[i].prevStackTop = (uint32_t)&(pcb[i].stackTop) -
			((uint32_t)&(pcb[current].stackTop) - pcb[current].prevStackTop);
		pcb[i].state = STATE_RUNNABLE;
		pcb[i].timeCount = pcb[current].timeCount;
		pcb[i].sleepTime = pcb[current].sleepTime;
		pcb[i].pid = i;
		pcb[i].regs.ss = USEL(2 + i * 2);
		pcb[i].regs.esp = pcb[current].regs.esp;
		pcb[i].regs.eflags = pcb[current].regs.eflags;
		pcb[i].regs.cs = USEL(1 + i * 2);
		pcb[i].regs.eip = pcb[current].regs.eip;
		pcb[i].regs.eax = pcb[current].regs.eax;
		pcb[i].regs.ecx = pcb[current].regs.ecx;
		pcb[i].regs.edx = pcb[current].regs.edx;
		pcb[i].regs.ebx = pcb[current].regs.ebx;
		pcb[i].regs.xxx = pcb[current].regs.xxx;
		pcb[i].regs.ebp = pcb[current].regs.ebp;
		pcb[i].regs.esi = pcb[current].regs.esi;
		pcb[i].regs.edi = pcb[current].regs.edi;
		pcb[i].regs.ds = USEL(2 + i * 2);
		pcb[i].regs.es = pcb[current].regs.es;
		pcb[i].regs.fs = pcb[current].regs.fs;
		pcb[i].regs.gs = pcb[current].regs.gs;
		/*XXX set return value */
		pcb[i].regs.eax = 0;
		pcb[current].regs.eax = i;
	}
	else {
		pcb[current].regs.eax = -1;
	}
	return;
}

void syscallExec(struct TrapFrame *tf) {
	int sel = tf->ds;
	char *str = (char *)tf->ecx;
	char tmp[128];
	int i = 0;
	char character = 0;
	int ret = 0;
	uint32_t entry = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
	while (character != 0) {
		tmp[i] = character;
		i++;
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
	}
	tmp[i] = 0;

	ret = loadElf(tmp, (current + 1) * 0x100000, &entry);
	if (ret == -1) {
		tf->eax = -1;
		return;
	}
	tf->eip = entry;
	return;
}

void syscallSleep(struct TrapFrame *tf) {
	if (tf->ecx == 0) {
		return;
	}
	else {
		pcb[current].state = STATE_BLOCKED;
		pcb[current].sleepTime = tf->ecx;
		asm volatile("int $0x20");
		return;
	}
	return;
}

void syscallExit(struct TrapFrame *tf) {
	pcb[current].state = STATE_DEAD;
	asm volatile("int $0x20");
	return;
}

void syscallSem(struct TrapFrame *tf) {
	switch(tf->ecx) {
		case SEM_INIT:
			syscallSemInit(tf);
			break;
		case SEM_WAIT:
			syscallSemWait(tf);
			break;
		case SEM_POST:
			syscallSemPost(tf);
			break;
		case SEM_DESTROY:
			syscallSemDestroy(tf);
			break;
		default:break;
	}
}

void syscallSemInit(struct TrapFrame *tf) {
	// TODO in lab4
	int value = tf->edx;
	int i = 0;
	for(; i<MAX_SEM_NUM; ++i){
		if(sem[i].state == 0)break;
	}
	if(i == MAX_SEM_NUM){
		pcb[current].regs.eax = -1;
	}
	else{
		sem[i].state = 1;
		sem[i].value = value;
		sem[i].pcb.next = &(sem[i].pcb);
 		sem[i].pcb.prev = &(sem[i].pcb);
		pcb[current].regs.eax = i;
	}
	return;
}

void syscallSemWait(struct TrapFrame *tf) {
	// TODO in lab4
	int wait = tf->edx;
	if(sem[wait].state == 1){
		sem[wait].value--;
		if(sem[wait].value < 0){
			pcb[current].blocked.next = sem[wait].pcb.next;
 			pcb[current].blocked.prev = &(sem[wait].pcb);
 			sem[wait].pcb.next = &(pcb[current].blocked);
 			(pcb[current].blocked.next)->prev = &(pcb[current].blocked);
			pcb[current].state = STATE_BLOCKED;
			pcb[current].sleepTime = -1; //Must be a value that <= 0, otherwise the blocked process may be woken up in TimeHandle
			asm volatile("int $0x20");
		}
		pcb[current].regs.eax = 0;
	}
	else{
		pcb[current].regs.eax = -1;
	}
	return;
}

void syscallSemPost(struct TrapFrame *tf) {
	// TODO in lab4
	int post = tf->edx;
	if(sem[post].state == 1){
		sem[post].value++;
		if(sem[post].value <= 0){
			ProcessTable *pt = (ProcessTable*)((uint32_t)(sem[post].pcb.prev) - (uint32_t)&(((ProcessTable*)0)->blocked));
			sem[post].pcb.prev = (sem[post].pcb.prev)->prev;
			(sem[post].pcb.prev)->next = &(sem[post].pcb);
			pt->state = STATE_RUNNABLE;
			pt->sleepTime = 0;
		}
		pcb[current].regs.eax = 0;
	}
	else{
		pcb[current].regs.eax = -1;
	}
	return;
}

void syscallSemDestroy(struct TrapFrame *tf) {
	// TODO in lab4
	int destory = tf->edx;
	if(sem[destory].state == 1){
		sem[destory].state = 0;
		sem[destory].value = 0;
		pcb[current].regs.eax = 0;
	}
	else{
		pcb[current].regs.eax = -1;
	}
	return;
}

void syscallGetPid(struct TrapFrame *tf) {
	pcb[current].regs.eax = current;
	return;
}

void GProtectFaultHandle(struct TrapFrame *tf){
	assert(0);
	return;
}

void syscallOpen(struct TrapFrame *tf){
	//Get the file/dir path
	int sel = tf->ds;
	char *str = (char *)tf->ecx;
	char path[128];
	int i = 0;
	uint8_t ret = 0;
	char character = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
	while (character != 0) {
		path[i] = character;
		i++;
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
	}
	path[i] = '\0';
	/*putString("path:");
	putString(path);
	putChar('\n'); */

	int flags = tf->edx;
	//uint8_t write_permission = flags&O_WRITE;
	//uint8_t read_permission = flags&O_READ;
	uint8_t create_permission = flags&O_CREATE;
	uint8_t dir_permission = flags&O_DIRECTORY;

	Inode destInode,fatherInode;
	int inodeOffset = 0, fatherOffset = 0;
	int file_exist = readInode(&sBlock,&destInode,&inodeOffset,path);

	if(file_exist==-1){  //File/dir not exist
		if(create_permission==0){
			pcb[current].regs.eax=-1;
	   		return;
		}
		//Get the father_path
		uint32_t len = stringLen(path);
		char father_path[128];
		char filename[128];
		int size = 0;
		if(path[len-1]=='/'){  //It is a dir
			if(dir_permission==0){   //Conflict
				pcb[current].regs.eax=-1;
	   			return;
			}
			len--;
		}
		stringCpy(path,father_path,len);
		stringChrR(father_path,'/',&size);
		stringCpy(path,father_path,size+1);

		//Get the filename
		stringCpy(path+size+1,filename,len-size-1);
		/*putString("filename:");
		putString(filename);
		putChar('\n');
		putString("father_path:");
		putString(father_path);
		putChar('\n'); */

		ret = readInode(&sBlock,&fatherInode,&fatherOffset,father_path);
		if(ret == -1){
			pcb[current].regs.eax=-1;
	   		return;
		}
		for(i = 0; i < MAX_FILE_NUM; i++){
			if(file[i].state==0) break;
		}
		if(i == MAX_FILE_NUM){   //The FCB is full
			pcb[current].regs.eax=-1;
	   		return;
		}
		int type;
		if(dir_permission==0){
			type = REGULAR_TYPE;
		}
		else{
			type = DIRECTORY_TYPE;
		}
		ret = allocInode(&sBlock,&fatherInode,fatherOffset,&destInode,&inodeOffset,filename,type);
		
		if(ret == -1){
			pcb[current].regs.eax=-1;
	   		return;
		}
		file[i].state = 1;
		file[i].inodeOffset = inodeOffset;
		file[i].offset = 0;
		file[i].flags = flags;

		pcb[current].regs.eax = MAX_DEV_NUM + i; //set fd
		return;
	}
	else{	//File/dir exist
		if(destInode.type == DIRECTORY_TYPE){  
			if(dir_permission == 0){	//Type is different
				pcb[current].regs.eax=-1;
	   			return;
			}
		}
		else{
			if(dir_permission != 0){	//Type is different
				pcb[current].regs.eax=-1;
	   			return;
			}
		}

		//Open device file
		for(i = 0; i < MAX_DEV_NUM; i++){
			if(dev[i].state==1&&dev[i].inodeOffset==inodeOffset){  //The file is open.
				//Like P-operation
				dev[i].value--;
				if(dev[i].value < 0){
					pcb[current].blocked.next = dev[i].pcb.next;
					pcb[current].blocked.prev = &(dev[i].pcb);
					dev[i].pcb.next = &(pcb[current].blocked);
					(pcb[current].blocked.next)->prev = &(pcb[current].blocked);
					pcb[current].state = STATE_BLOCKED;
					pcb[current].sleepTime = -1; 
					asm volatile("int $0x20");
				}
				pcb[current].regs.eax = i;
				return;
			}
		}

		for(i = 0; i < MAX_FILE_NUM; i++){
			if(file[i].state==0) break;
		}
		if(i == MAX_FILE_NUM){   //The FCB is full
			pcb[current].regs.eax=-1;
	   		return;
		}
		file[i].state = 1;
		file[i].inodeOffset = inodeOffset;
		file[i].offset = 0;
		file[i].flags = flags;

		pcb[current].regs.eax = MAX_DEV_NUM + i; //set fd
		return;
	}

	return;
}

void syscallLseek(struct TrapFrame *tf){
	int fd = tf->ecx;
	int offset = tf->edx;
	int whence = tf->ebx;
	if(fd < MAX_DEV_NUM || fd >= MAX_DEV_NUM+MAX_FILE_NUM){
		pcb[current].regs.eax=-1;
	   	return;
	}
	if (file[fd-MAX_DEV_NUM].state == 0){ //The file hasn't been opened
		pcb[current].regs.eax=-1;
	   	return;
	}

	Inode inode;
	diskRead(&inode, sizeof(Inode), 1, file[fd-MAX_DEV_NUM].inodeOffset);
	if(whence == SEEK_SET){
		if(offset>=0 && offset<inode.size){
			file[fd-MAX_DEV_NUM].offset = offset;
			pcb[current].regs.eax = offset;
			return;
		}
	}
	else if(whence == SEEK_CUR){
		if(offset+file[fd-MAX_DEV_NUM].offset>=0 && offset+file[fd-MAX_DEV_NUM].offset<inode.size){
			file[fd-MAX_DEV_NUM].offset += offset;
			pcb[current].regs.eax = file[fd-MAX_DEV_NUM].offset;
			return;
		}
	}
	else if(whence == SEEK_END){
		if(offset<=0 && offset+inode.size>=0){
			file[fd-MAX_DEV_NUM].offset = offset+inode.size;
			pcb[current].regs.eax = file[fd-MAX_DEV_NUM].offset;
			return;
		}
	}

	//other case
	pcb[current].regs.eax=-1;
	return;
}

void syscallClose(struct TrapFrame *tf){
	int fd = tf->ecx;
	if(fd<0 || fd>=MAX_DEV_NUM+MAX_FILE_NUM){
		pcb[current].regs.eax=-1;
		return;
	}
	if(fd < MAX_DEV_NUM){  //Device file
		if(file[fd].state == 1){ 
			//Like V-operation
			dev[fd].value++;
			if(dev[fd].value <= 0){
				ProcessTable *pt = (ProcessTable*)((uint32_t)(dev[fd].pcb.prev) - (uint32_t)&(((ProcessTable*)0)->blocked));
				dev[fd].pcb.prev = (dev[fd].pcb.prev)->prev;
				(dev[fd].pcb.prev)->next = &(dev[fd].pcb);
				pt->state = STATE_RUNNABLE;
				pt->sleepTime = 0;
			}
			pcb[current].regs.eax = 0;
			return;
		}
	}
	else{	//Simple file
		if(file[fd-MAX_DEV_NUM].state != 0){
			file[fd-MAX_DEV_NUM].state = 0;
			pcb[current].regs.eax = 0;
			return;
		}
	}

	//other case
	pcb[current].regs.eax=-1;
	return;
}

int remove(Inode *destInode,Inode *fatherInode,int *destInodeOffset,int *fatherInodeOffset,char *filename){  //A recursive function
	//Remove the file
	if(destInode->type != DIRECTORY_TYPE){
		return freeInode(&sBlock,fatherInode,*fatherInodeOffset,destInode,destInodeOffset,filename,destInode->type);
	}
	//Remove the directory, like the 'readInode' function
	// go deeper dir
	uint8_t buffer[BLOCK_SIZE];
	for(int i=0; i<destInode->blockCount; ++i){
		int ret = readBlock(&sBlock, destInode, i, buffer);
		if(ret == -1){
			pcb[current].regs.eax = -1;
			return -1;
		}
		DirEntry *dirEntry = (DirEntry *)buffer;
		for (int j = 0; j < sBlock.blockSize / sizeof(DirEntry); j++) {
			if (dirEntry[j].inode == 0)
				continue;
			else{
				int tmpOffset = dirEntry[j].inode;
				Inode tmpInode;
				diskRead(&tmpInode, sizeof(Inode), 1, tmpOffset);
				ret = remove(&tmpInode,destInode,&tmpOffset,destInodeOffset,dirEntry[j].name); //recursive remove
				if(ret==-1){
					return -1;
				}
			}
		}
	}
	return freeInode(&sBlock,fatherInode,*fatherInodeOffset,destInode,destInodeOffset,filename,destInode->type);
}

void syscallRemove(struct TrapFrame *tf){
	//Get the path
	int sel = tf->ds;
	char *str = (char *)tf->ecx;
	char path[128];
	int i = 0;
	uint8_t ret = 0;
	char character = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
	while (character != 0) {
		path[i] = character;
		i++;
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
	}
	path[i] = 0;

	Inode destInode,fatherInode;
	int inodeOffset = 0, fatherOffset = 0;
	int file_exist = readInode(&sBlock,&destInode,&inodeOffset,path);
	if(file_exist == -1){ //The file/dir is not exist
		pcb[current].regs.eax=-1;
		return;
	}
	
	//Get the father_path
	uint32_t len = stringLen(path);
	char father_path[128];
	char filename[128];
	int size = 0;
	if(path[len-1]=='/'){  //It is a dir
		len--;
	}
	stringCpy(path,father_path,len);
	stringChrR(father_path,'/',&size);
	stringCpy(path,father_path,size+1);

	//Get the filename
	stringCpy(path+size+1,filename,len-size-1);

	ret = readInode(&sBlock,&fatherInode,&fatherOffset,father_path);
	if(ret == -1){
		pcb[current].regs.eax = -1;
		return;
	}
	ret = remove(&destInode,&fatherInode,&inodeOffset,&fatherOffset,filename);  //Delete files in the directory recursively 
	if(ret == -1){
		pcb[current].regs.eax = -1;
		return;
	}
	pcb[current].regs.eax = ret;
	return;
}
