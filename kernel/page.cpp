#include "page.h"
#include "Utils.h"
#include "video.h"
#include "malloc.h"
#include "process.h"





DWORD gPageAllocLock = FALSE;





LPMEMALLOCINFO isPageIdxExist(DWORD addr,int size) {

	LPMEMALLOCINFO info = (LPMEMALLOCINFO)PAGE_ALLOC_LIST;
	do
	{
		if (info->addr == addr)
		{
			return info;
		}
		else if ( (info->addr < addr) && (info->addr + info->size > addr) ) 
		{
			return info;
		}
		else {
			info = (LPMEMALLOCINFO)info->list.next;
		}
	} while (info != (LPMEMALLOCINFO)PAGE_ALLOC_LIST);

	return 0;
}

LPMEMALLOCINFO getFreePageIdx() {

	LPMEMALLOCINFO info = (LPMEMALLOCINFO)(LPMEMALLOCINFO)PAGE_ALLOC_LIST;

	int cnt = PAGE_ALLOC_LIST_SIZE / sizeof(MEMALLOCINFO);
	for (int i = 0; i < cnt; i++)
	{
		if (info[i].size == 0 && info[i].addr == 0 && info[i].pid == 0 && info[i].vaddr == 0)
		{
			return &info[i];
		}
	}
	return 0;
}


int resetPageIdx(LPMEMALLOCINFO pde) {
	DWORD size = pde->size;
	removelist((LPLIST_ENTRY)&pde->list);
	pde->addr = 0;
	pde->size = 0;
	pde->vaddr = 0;
	pde->pid = 0;
	return size;
}



int insertPageIdx(LPMEMALLOCINFO info, DWORD addr, int size,int pid,DWORD vaddr ) {
	info->size = size;
	info->addr = addr;
	LPPROCESS_INFO tss = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
	info->pid = tss->pid;

	addlistTail( &((LPMEMALLOCINFO)PAGE_ALLOC_LIST)->list, (LPLIST_ENTRY)&info->list);
	return TRUE;
}



extern "C"  __declspec(dllexport) DWORD __kPageAlloc(int size) {
	DWORD res = 0;

	if (size % PAGE_SIZE)
	{
		return FALSE;
	}

	int factor = 1;
	DWORD addr = PAGE_TABLE_BASE + size*factor;
	if (addr + size > PAGE_TABLE_BASE + PAGE_TABLE_SIZE)
	{
		return FALSE;
	}

	LPPROCESS_INFO tss = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;

	__enterSpinlock(&gPageAllocLock);

	LPMEMALLOCINFO info = isPageIdxExist(addr,size);
	if (info == 0)
	{
		info = getFreePageIdx();
		if (info)
		{
			insertPageIdx(info, addr, size, tss->pid, addr);

			res = addr;
		}
		else {
			res = -1;
		}
	}
	else {
		while (1)
		{
			if (factor > 1 && info->size <= size)
			{
				for (int i = 0; i < factor - 1; i++)
				{
					addr += size;
					if (addr + size > PAGE_TABLE_BASE + PAGE_TABLE_SIZE)
					{
						res = -1;
						break;
					}

					info = isPageIdxExist(addr,size);
					if (info == 0)
					{
						info = getFreePageIdx();
						if (info)
						{
							insertPageIdx(info, addr, size, tss->pid, addr);
							res = addr;
						}
						else {
							res = -1;
						}
						break;
					}
					else {
						break;
					}
				}
			}
			else {
				//noting to do
			}

			if (res == 0) {
				factor = (factor << 1);
				addr = PAGE_TABLE_BASE + size * factor;
			}
			else {
				break;
			}
		}
	}

	__leaveSpinlock(&gPageAllocLock);

	if (res == -1) {
		res = 0;
	}

	return res;
}






extern "C"  __declspec(dllexport) int __kFreePage(DWORD addr) {

	__enterSpinlock(&gPageAllocLock);

	LPMEMALLOCINFO info = isPageIdxExist(addr,0);
	if (info)
	{
		DWORD size = resetPageIdx(info);
	}
	else {
		char szout[1024];
		int len = __printf(szout, "__kFreePage not found address:%x\n", addr);
	}
	__leaveSpinlock(&gPageAllocLock);

	return FALSE;
}



//make sure the first in the list is not to be deleted,or else will be locked
void freeProcessPages(int pid) {

	__enterSpinlock(&gPageAllocLock);
	
	//LPPROCESS_INFO tss = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;

	LPMEMALLOCINFO info = (LPMEMALLOCINFO)(LPMEMALLOCINFO)PAGE_ALLOC_LIST;
	do
	{
		if (info->pid == pid)
		{
			resetPageIdx(info);
		}

		info = (LPMEMALLOCINFO)info->list.next;

	} while (info != (LPMEMALLOCINFO)PAGE_ALLOC_LIST);

	__leaveSpinlock(&gPageAllocLock);
}


//R/W--λ1�Ƕ�/д��Read/Write����־���������1����ʾҳ����Ա�����д��ִ�С����Ϊ0����ʾҳ��ֻ�����ִ�С�
//�������������ڳ����û���Ȩ��������0��1��2��ʱ����R/Wλ�������á�ҳĿ¼���е�R/Wλ������ӳ�������ҳ�������á�
//U/S--λ2���û�/�����û���User / Supervisor����־�����Ϊ1����ô�������κ���Ȩ���ϵĳ��򶼿��Է��ʸ�ҳ�档
//���Ϊ0����ôҳ��ֻ�ܱ������ڳ����û���Ȩ����0��1��2���ϵĳ�����ʡ�ҳĿ¼���е�U / Sλ������ӳ�������ҳ�������á�

void linearMapping() {

	DWORD* idx = (DWORD*)PTE_ENTRY_VALUE;
	DWORD* entry = (DWORD*)PDE_ENTRY_VALUE;

	DWORD buf = PAGE_PRESENT | PAGE_READWRITE| PAGE_USERPRIVILEGE;
	for (int i = 0; i < ITEM_IN_PAGE; i++) {
		entry[i] = (DWORD)idx | (PAGE_PRESENT | PAGE_READWRITE | PAGE_USERPRIVILEGE);

		for (int j = 0; j < ITEM_IN_PAGE; j++) {
			idx[j] = buf;
			buf += PAGE_SIZE;
		}
		idx += ITEM_IN_PAGE;
	}
}

/*
 �����������Ỻ����ЩPλ��1����Щҳ������ң�TLB�Ĺ�����CR3�Ĵ�����PCDλ��PWT���޹صġ�
 ����ҳ������޸Ĳ���ͬʱ��ӳ��TLB�У�һ��Ҫˢ��TLB����Ȼ��ҳ������þ�����Ч�ġ�
 TLB���������ֱ�ӷ��ʵģ�ֻ��ͨ����ʽˢ��CR3�����������л���ʽˢ��TLB��
 ����Ҫע����ǣ�������ˢ�·���������Щ���Ϊȫ�֣�G=1����ҳ����Ч��

 TLB�����Ե���ˢ�£�����invlpg���invalidate TLB Entry����
 invlpg�ĸ�ʽΪinvlpg m32����ִ������ָ���ʱ�򣬴��������ø��������Ե�ַ����TLB���ҵ��Ǹ���Ŀ��Ȼ����ڴ������¼��������ݵ���Ӧ��TLBҳ��������
 invlpg����Ȩָ�����Ҫ��CPLΪ��Ȩ0��ִ�У���ָ�Ӱ���κα�־λ��
*/

void initPaging() {

	linearMapping();

	__asm {
		mov eax, PDE_ENTRY_VALUE
		mov cr3,eax

		mov eax,cr0
		or eax,0x80000000
		mov cr0,eax
	}

	LPMEMALLOCINFO pageList = (LPMEMALLOCINFO)PAGE_ALLOC_LIST;
	initListEntry(&pageList->list);
	pageList->addr = 0;
	pageList->size = 0;
	pageList->vaddr = 0;
	pageList->pid = 0;
}