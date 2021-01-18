#include "ntddk.h"
#include "ntstrsafe.h"

/**
 * ��Windows�ں˱�̡�
 * ��Ŀ¼�Ļ�������Windows�ڿͱ�̼�����⡷�󲿷������ص�
 * ��һ�鿴������ʲô��ͬ������Ҳ����һ��֮ǰ��ѧϰ
 * 
 * PS:ʹ������VS�Դ���git����������Զ�̿⣬ֱ�����ɵ�.gitignore�ǳ����㣬ֱ�ӰѸú��ԵĶ�д����
 */

/**
 * ��ڿͼ������ܵĸ�������ֱ��Ӧ�ò�ͬ
 * ��Ŀ�ͷ�����˺ܶ������㿪����������֪ʶ
 */

/**
 * CPU���л�����:
 * x86������У�CPU���ĸ���Ȩ���� ring0����Ȩ���~ring3
 * ������ں˶�ֻʹ��0��3��������
 * �ں�Ӧ��������ring0���û�Ӧ��������ring3
 */

/**
 * IRQL�жϼ���
 * ��IRQL��������жϵ�IRQL����ִ�й���
 * �����г�������Ϊ0~2
 */

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
	if (DriverObject != NULL)
	{
		DbgPrint("[%ws]Driver Upload,Driver Object Address:%p", __FUNCTIONW__, DriverObject);
	}
	//KeBugCheckEx(0x0, 0x0, 0x0, 0x0, 0x0); // ��������
	return;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	//��ȡ��ǰ�������ڵĽ���ID��IRQL���𣬴˴�Ӧ������ֵ��Ϊ0
	DbgPrint("[%ws]Hello Kernel World, CurrentProcessId = 0x%p , CurrentIRQL = 0x%u\n", __FUNCTIONW__, PsGetCurrentProcessId(), KeGetCurrentIrql());
	if (RegistryPath != NULL)
	{
		DbgPrint("[%ws]Driver RegistryPath:%wZ\n", __FUNCTIONW__, RegistryPath);
	}

	if (DriverObject != NULL)
	{
		DbgPrint("[%ws]Driver Object Address:%p\n", __FUNCTIONW__, DriverObject);
		DriverObject->DriverUnload = DriverUnload;
	}

	//�ں˲����У��󲿷����ʹ��Unicode
	WCHAR strBuf[128] = { 0 };
	//һ�㲻ֱ��ʹ��WCHAR������UNICODE_STRING���ṹ����bufferָ����ָ��������β��һ������'\0'
	//������������־��Ϊ��ȫ��������Ч��ֹ������������ǵ�\0���µ�Υ������
	UNICODE_STRING uFirstString = { 0 };
	RtlInitEmptyUnicodeString(&uFirstString, strBuf, sizeof(strBuf));
	RtlUnicodeStringCopyString(&uFirstString, L"Hello,Kernel\n"); //ֻ����PASSIVE_LEVEL��ʹ��
	DbgPrint("String:%wZ", &uFirstString);

	return STATUS_SUCCESS;
}