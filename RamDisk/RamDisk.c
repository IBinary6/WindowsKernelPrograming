#include <ntddk.h>
#include <wdf.h> //��Ҫ���ð���Ŀ¼����������������½���Ŀʱʹ��KMDF��Ŀ���

/**
 * �������⼼��
 *
 * ʹ�÷Ƿ�ҳ�ڴ����Ĵ��̴洢�ռ䣬��������һ������������ʽ��¶���û�
 *
 * ʹ����WDF���Ƕ�WDM�ķ�װ
 * ����Ե�Դ����ͼ��弴�������ĳ��á����ӵĴ��������з�װ�����Ը�����Ľ��п���
 */

NTSTATUS
RamDiskEvtDeviceAdd(
	IN WDFDRIVER Driver,
	IN PWDFDEVICE_INIT DeviceInit
)
{
	//�ûص������������ڼ��弴�ù������������豸ʱ������豸�����г�ʼ��������
	//�κ�֧��PnP������������Ӧ���������ĺ���������WDM������AddDevice�ص��ķ��棩
	//��DriverEntryִ����Ϻ�����������ֻ���������������ϵͳ������ϵ��
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegistryPath)
{
	//��������Ŀ������һ�����EvtDriverDeviceAdd��EvtDriverUnload�ص���������ڵ�ַ��������ʼ��ʱ�ı�־�ͷ����ڴ���ʹ�õ�tagֵ
	WDF_DRIVER_CONFIG config;

	KdPrint(("Windows Ramdisk Driver - Driver Framework Edition.\n"));
	KdPrint(("Built %s %s\n", __DATE__, __TIME__));

	//��ʼ������ʱ���Ὣ�û��Զ����EvtDriverDeviceAdd�ص������������У�����ʼ����������
	WDF_DRIVER_CONFIG_INIT(&config, RamDiskEvtDeviceAdd);

	//��ԭ������������һ�ΰ�װ�����ݲ����Ի������г�ʼ����������������������
	return WdfDriverCreate(
		pDriverObject,	//��ں�������������
		pRegistryPath,
		WDF_NO_OBJECT_ATTRIBUTES, //��ʾ����Ҫ��������
		&config,
		WDF_NO_HANDLE	//��Ϊ����������������WDF��������������
	);//���˽�config�������ҹ������й����У�PnP�������ͻ������Ҫ���ûص�����
}