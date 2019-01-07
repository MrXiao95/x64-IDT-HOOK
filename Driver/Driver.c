/*
	IDT HOOK ----By:kanren
	PS:处理函数自己写，只是写个框架
	注意事项:
	1.IDTR与IDTENTRY的结构体，WRK中有写，x86与x64不同，需注意结构体字节对齐 详细原因自行调试
	2.IDTENTRY属于只读页面，需Cr0 WP位置0才可以写入
	3.IDTENTRY中的成员说明:OffsetHigh(地址的高32位), OffsetMiddle(地址低32位的高16位), OffsetLow(地址的低16位)
	4.sidt获取当前CPU核心的IDTR,多核处理代码已经在下面了
	5.IDT属于PG保护范围，若想安全使用，自行准备PassPG
	源码仅供参考，如有问题，给我留言
*/

#include <ntifs.h>
#include <windef.h>
#include "Tools.h"
IDTR idtr;

VOID DriverUnload(IN PDRIVER_OBJECT pDriverObj)
{
	DbgPrint("DriverUnload\n");
}

KIRQL WPOFFx64()
{
	KIRQL  irql = KeRaiseIrqlToDpcLevel();
	UINT64  cr0 = __readcr0();
	cr0 &= 0xfffffffffffeffff;
	__writecr0(cr0);
	_disable();
	return  irql;
}

void WPONx64(KIRQL irql)
{
	UINT64 cr0 = __readcr0();
	cr0 |= 0x10000;
	_enable();
	__writecr0(cr0);
	KeLowerIrql(irql);
}

ULONG64 GetIdtAddr(ULONG64 IdtBaseAddr, UCHAR Index)
{
	PIDT_ENTRY Pidt = (PIDT_ENTRY)(IdtBaseAddr);
	Pidt = Pidt + Index;
	ULONG64 OffsetHigh, OffsetMiddle, OffsetLow, ret;
	OffsetHigh = (ULONG64)Pidt->idt.OffsetHigh << 32;
	OffsetMiddle = Pidt->idt.OffsetMiddle << 16;
	OffsetLow = Pidt->idt.OffsetLow;
	ret = OffsetHigh + OffsetMiddle + OffsetLow;
	return ret;
}

ULONG64 SetIdtAddr(ULONG64 IdtBaseAddr, UCHAR Index, ULONG64 NewAddr)
{
	PIDT_ENTRY Pidt = (PIDT_ENTRY)(IdtBaseAddr);
	Pidt = Pidt + Index;
	ULONG64 OffsetHigh, OffsetMiddle, OffsetLow, ret;
	OffsetHigh = (ULONG64)Pidt->idt.OffsetHigh << 32;
	OffsetMiddle = Pidt->idt.OffsetMiddle << 16;
	OffsetLow = Pidt->idt.OffsetLow;
	ret = OffsetHigh + OffsetMiddle + OffsetLow;
	Pidt->idt.OffsetHigh = NewAddr >> 32;
	Pidt->idt.OffsetMiddle = NewAddr << 32 >> 48;
	Pidt->idt.OffsetLow = NewAddr << 48 >> 48;
	return ret;
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObj, IN PUNICODE_STRING pRegistryString)
{	
	DbgPrint("DriverEntry\n");
	pDriverObj->DriverUnload = DriverUnload;
	for (int i = 0; i < KeNumberProcessors; i++)
	{
		KeSetSystemAffinityThread((KAFFINITY)(1 << i));
		__sidt(&idtr);
		DbgPrint("CPU[%d] IDT Base:%llX\n", i, idtr.Base);
		KIRQL IRQL = WPOFFx64();
		DbgPrint("CPU[%d] Old IDT[0xEA]:%llX\n", i, SetIdtAddr(idtr.Base, 0xEA, 0x1234567887654321));
		WPONx64(IRQL);
		KeRevertToUserAffinityThread();
	}
	return STATUS_SUCCESS;
}