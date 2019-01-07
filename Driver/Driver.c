/*
	IDT HOOK ----By:kanren
	PS:�������Լ�д��ֻ��д�����
	ע������:
	1.IDTR��IDTENTRY�Ľṹ�壬WRK����д��x86��x64��ͬ����ע��ṹ���ֽڶ��� ��ϸԭ�����е���
	2.IDTENTRY����ֻ��ҳ�棬��Cr0 WPλ��0�ſ���д��
	3.IDTENTRY�еĳ�Ա˵��:OffsetHigh(��ַ�ĸ�32λ), OffsetMiddle(��ַ��32λ�ĸ�16λ), OffsetLow(��ַ�ĵ�16λ)
	4.sidt��ȡ��ǰCPU���ĵ�IDTR,��˴�������Ѿ���������
	5.IDT����PG������Χ�����밲ȫʹ�ã�����׼��PassPG
	Դ������ο����������⣬��������
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