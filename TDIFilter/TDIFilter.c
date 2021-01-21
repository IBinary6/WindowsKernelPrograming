#include <ntddk.h>
#include <tdikrnl.h>

//TODO �������������Թ����̡����̡��ļ����ˣ���Ҫ�����������
/**
 * �������
 * TDI������̭����WFPȡ��
 * ����û��Ƕ�ѧϰһ�£�����ԭ�����вο���ֵ�ģ�����һЩ���������⣬���ܻ��õ�TDI
 * �Ӳ����Ͽ���TDI֧�ֵ�Win7����֮�󣨰���Win7���İ汾��Ӧ����WFP
 */

 /**
  * Ӧ�ò�ʹ��socket�������������Windows��ʵ��TCPЭ�������Ϊtcpip.sys����һ��NDISЭ������
  * ��TDI�ӿھ���������socket��Э���������м��
  * Э������������һ�������ֵ��豸���ܽ���һ���������ɡ����ƣ�����bind��connect��listen��accept��send��recv�ȣ�
  * ���ݰ��豸ԭ�����ǿ�������һ�������豸��Э���������豸����������ͻ��ȱ������豸�ػ�
  *
  * PS: TDIFW����һ����Դ��TDI����ǽ
  * https://sourceforge.net/projects/tdifw/files/latest/download
  */

  /**
   * TDI���
   * ����Э���TDI�豸��
   * "\Device\Tcp"  "\Device\Udp"  "\Device\RawIp"(ԭʼIP��)
   */

   //���������豸
NTSTATUS
c_n_a_device(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT* fltobj, PDEVICE_OBJECT* oldobj, wchar_t* devname)
{
	NTSTATUS status;
	UNICODE_STRING str;

	//���������豸
	status = IoCreateDevice(DriverObject,
		0,
		NULL,
		FILE_DEVICE_UNKNOWN,
		0,
		TRUE,
		fltobj);
	if (status != STATUS_SUCCESS) {
		KdPrint(("[tdi_fw] c_n_a_device: IoCreateDevice(%S): 0x%x\n", devname, status));
		return status;
	}

	(*fltobj)->Flags |= DO_DIRECT_IO;//����IO��ʽ

	RtlInitUnicodeString(&str, devname);
	//���豸
	status = IoAttachDevice(*fltobj, &str, oldobj);
	if (status != STATUS_SUCCESS) {
		KdPrint(("[tdi_fw] DriverEntry: IoAttachDevice(%S): 0x%x\n", devname, status));
		return status;
	}

	KdPrint(("[tdi_fw] DriverEntry: %S fileobj: %p\n", devname, *fltobj));

	return STATUS_SUCCESS;
}

//Э���
#define IPPROTO_IP              0
#define IPPROTO_ICMP            1
#define IPPROTO_TCP             6
#define IPPROTO_UDP             17

//��������豸����ʵ�豸Ϊȫ�ֱ���
PDEVICE_OBJECT
g_tcpfltobj = NULL,//����tcp�����豸����ָ�룬���¸�����������
g_udpfltobj = NULL,
g_ipfltobj = NULL,
g_tcpoldobj = NULL,
g_udpoldobj = NULL,
g_ipoldobj = NULL;

//���豸��
#define DEVICE_NAME_TCP L"\\Device\\Tcp"
#define DEVICE_NAME_UDP L"\\Device\\Udp"
#define DEVICE_NAME_IP L"\\Device\\RawIp"

//���ݹ����豸�����ȡ��ʵ�豸����
PDEVICE_OBJECT get_original_devobj(PDEVICE_OBJECT flt_devobj, int* proto)
{
	PDEVICE_OBJECT result;
	int ipproto;

	//���ݱ����ȫ�ֱ���һһ��Ӧ�ķ���
	if (flt_devobj == g_tcpfltobj)
	{
		result = g_tcpoldobj;
		ipproto = IPPROTO_TCP;
	}
	else if (flt_devobj == g_udpfltobj) {
		result = g_udpoldobj;
		ipproto = IPPROTO_UDP;
	}
	else if (flt_devobj == g_ipfltobj) {
		result = g_ipoldobj;
		ipproto = IPPROTO_IP;
	}
	else
	{
		KdPrint(("[tdi_fw] get_original_devobj: Unknown DeviceObject %p\n", flt_devobj));
		ipproto = IPPROTO_IP;
		result = NULL;
	}

	if (result != NULL && proto != NULL)
	{
		*proto = ipproto;
	}

	return result;
}

//ж�غ���
VOID OnUnload(PDRIVER_OBJECT DriverObject)
{
	//���ɾ��������豸
	if (g_tcpoldobj != NULL) IoDetachDevice(g_tcpoldobj);
	if (g_tcpfltobj != NULL) IoDeleteDevice(g_tcpfltobj);
	if (g_udpoldobj != NULL) IoDetachDevice(g_udpoldobj);
	if (g_udpfltobj != NULL) IoDeleteDevice(g_udpfltobj);
	if (g_ipoldobj != NULL) IoDetachDevice(g_ipoldobj);
	if (g_ipfltobj != NULL) IoDeleteDevice(g_ipfltobj);
}

//�ַ�����
NTSTATUS DeviceDispatch(PDEVICE_OBJECT DeviceObject, PIRP irp)
{
	NTSTATUS status;
	PDEVICE_OBJECT old_devobj = get_original_devobj(DeviceObject, NULL);
	if (old_devobj != NULL)
	{
		//ת����ԭ�豸
		IoSkipCurrentIrpStackLocation(irp);
		status = IoCallDriver(old_devobj, irp);
	}
	else
	{
		//�Ҳ�����Ӧ�豸������ʧ��
		status = irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
	}
	return status;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = STATUS_SUCCESS;

	//���÷ַ�����
	for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		DriverObject->MajorFunction[i] = DeviceDispatch;
	}

	//���ɹ����豸����
	status = c_n_a_device(DriverObject, &g_tcpfltobj, &g_tcpoldobj, DEVICE_NAME_TCP);
	if (status != STATUS_SUCCESS)
	{
		KdPrint(("[tdi_fw] DriverEntry: c_n_a_device: tcp: 0x%x\n", status));
		goto done;
	}
	status = c_n_a_device(DriverObject, &g_udpfltobj, &g_udpoldobj, DEVICE_NAME_UDP);
	if (status != STATUS_SUCCESS)
	{
		KdPrint(("[tdi_fw] DriverEntry: c_n_a_device: udp: 0x%x\n", status));
		goto done;
	}
	status = c_n_a_device(DriverObject, &g_ipfltobj, &g_ipoldobj, DEVICE_NAME_IP);
	if (status != STATUS_SUCCESS)
	{
		KdPrint(("[tdi_fw] DriverEntry: c_n_a_device: ip: 0x%x\n", status));
		goto done;
	}

done:
	if (status != STATUS_SUCCESS)
	{
		//ʧ�����ͷ���Դ
		OnUnload(DriverObject);
	}
	return status;
}