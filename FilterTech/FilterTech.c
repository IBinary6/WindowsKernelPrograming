#include <ntddk.h>

/**
 * ���˼���
 * ϵͳ�кܶ���˻��ƣ��������̹��ˡ����̹��ˡ��ļ����ˡ�������˵�
 */

 /**
  * ����һ�����ڹ�������ѧϰ���������������Ҫ��
  * ��������Ҫ�ķ�����ͨ���������һ�������豸���󣬲��󶨵�һ����ʵ�豸��
  * �󶨺�ϵͳ���͸���ʵ�豸������ͻ��ȴ���������豸����һ��
  * ʹ��IoAttachDevice�󶨵��豸������Ҫ������
  * ���һ�����ֶ�Ӧ���豸�����ˣ���ô������һ���һ���豸����Ϊ�豸ջ
  * IoAttachDevice���ǻ���豸ջ�������Ǹ��豸
  * ����һ��API���Ը����豸����ָ��󶨣�IoAttachDeviceToDeviceStack(Safe)
  */
VOID Test_Filter()
{
	//���ɹ����豸����
	//���豸ǰ��Ӧ���豸�������������ó���Ŀ�����һ��
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	return STATUS_SUCCESS;
}