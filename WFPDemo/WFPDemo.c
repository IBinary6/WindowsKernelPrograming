#include <ntddk.h>

/**
 * WFP��Windows Filter Platform��Windows����ƽ̨��
 * ΢��ϣ����WFP������֮ǰ��Winsock LSP��TDI�Լ�NDIS�������������
 * �����߿�����WFP���ֵĲ�ͬ�ֲ���й��ˡ��ض����޸ĵ�
 * WFP��������û�̬API���ں�̬API�����û���Ҳ���Դ����������ݰ�����Ҫѧϰ���ں˲�ʹ��
 */

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	return STATUS_SUCCESS;
}