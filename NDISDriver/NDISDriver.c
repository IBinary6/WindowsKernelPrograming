#include <ntddk.h>
#include <ndis.h> //�ֶ�����ndis.lib

/**
 * NDISЭ������
 *
 * ��Windows��������������������߾����������ӽ����󣬽���������Ϊ����̫����ʽ
 * ��̫�����ṹΪ��
 *		Դ��ַ��6�ֽڣ�		Ŀ���ַ��6�ֽڣ�		���ͣ�2�ֽڣ�		���������ݣ�
 * �����ַָ�Ķ�������MAC��ַ����������0x80��0x00˵���Ǹ�IP��
 *
 * �ϲ��û���Socket����TCPЭ�鷢������ʱ����������Щ���ݷ�װΪIP�����ٷ�װΪ��̫��������ȥ������ʱ����������ύ���ϲ�Ӧ��
 * ʵ��Ӧ���У�Э������������������̽��һ�㲻���ڷ���ǽ����Ϊ���Ը�Ԥ�շ���
 *
 * NDIS���������֣�Э��������С�˿��������м������
 * Э�������ϲ��ṩֱ�ӹ�Ӧ�ò�socketʹ�õ����ݴ���ӿڣ��²��С�˿ڣ�С�˿�����ֱ��������������ڷ����������̫����
 * ��ͳ�м�����������ⷽʽ����Э��������С�˿�����֮�䣬���𽥱�������������
 */

 /**
  * Э��������Ҫ��д���̣�
  *
  * ����ں�����дЭ��������Э��Ļص������б�
  * ʹ��NdisRegisterProtocolDriver���Լ�ע��ΪЭ������
  * ϵͳ���ÿ��ʵ�ʴ��ڵ�����ʵ�����ñ�Э�������ṩ�Ļص�����������ص�������Ӧ�����Ƿ��һ������
  * ���������¼�ʱ����������ĳ���������ã�Э�鿪���������о�����δ�����յ������ݰ�
  * ��Ӧ����ͼ��������ʱ�����Դ����Э�鲢��������ʹ��socket��Э���Լ��ṩ���豸�ӿڣ�
  *
  * ʾ����
  * ���ٷ�github
  * https://github.com/microsoft/Windows-driver-samples/tree/master/network/ndis
  */
VOID manual(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = STATUS_SUCCESS;
	//�����豸
	PDEVICE_OBJECT deviceObj; //������
	//Э����������
	NDIS_PROTOCOL_CHARACTERISTICS protocolChar;
	//��дЭ������
	NdisZeroMemory(&protocolChar, sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
	protocolChar.MajorNdisVersion = 5;
	protocolChar.MinorNdisVersion = 0;
	//protocolChar.OpenAdapterCompleteHandler = handler;
	//...����handlerע��
	//ע��Э��
	NDIS_HANDLE ndis_handle;
	NdisRegisterProtocol((PNDIS_STATUS)&status, &ndis_handle, &protocolChar, sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
	if (status != NDIS_STATUS_SUCCESS)
	{
		//ע��ʧ��
	}
	//ע����ַ�����
	//...
}

/**
 * ������֮��İ󶨣�Bind��
 * ���豸����֮��󶨣�Attach���ǲ�ͬ��
 * ��Э���������������������յ������ݽ��ύ�����Э�飬Э�����ʹ����������������ݰ�
 * ������һ��һ��ϵ��һ��Э�鶼�����������
 */
VOID bind()
{
	//Э�������п������ð󶨻ص�����
	NDIS_PROTOCOL_CHARACTERISTICS protocolChar;
	NdisZeroMemory(&protocolChar, sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
	//protocolChar.BindAdapterHandler = NdisProtBindAdapter;
	//protocolChar.UnbindAdapterHandler = NdisProtUnbindAdapter;
	//��Windows�ں˼�⵽��������ʱ���ͻ����ÿ��ע���Э���BindAdapterHandler����
}

/**
 * ����İ󶨹���
 * ��������Ƚϸ��ӣ�������һ����Ŀ��ndisprot
 */


NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	return STATUS_SUCCESS;
}