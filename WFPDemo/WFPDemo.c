#include <ntddk.h>
#include <fwpsk.h> //ʹ��fwp��ҪԤ����NDIS֧�ְ汾��NDIS_SUPPORT_NDIS6
#include <fwpmk.h> //������ӿ�����fwpkclnt.lib ��һ�㻹��Ҫuuid.lib��
/**
 * WFP��Windows Filter Platform��Windows����ƽ̨��
 * �ٷ��ĵ���
 * https://docs.microsoft.com/zh-cn/windows-hardware/drivers/network/windows-filtering-platform-callout-drivers2
 * ΢��ϣ����WFP������֮ǰ��Winsock LSP��TDI�Լ�NDIS�������������
 * �����߿�����WFP���ֵĲ�ͬ�ֲ���й��ˡ��ض����޸ĵ�
 * ͨ��WFP������ʵ�ַ���ǽ�����ּ�⡢������ӡ��������Ƶ�
 * WFP��������û�̬API���ں�̬API�����û���Ҳ���Դ����������ݰ�����Ҫѧϰ���ں˲�ʹ��
 *
 * WFP��ܽṹ��
 *
 *				�׽���Ӧ�ó���						����������ǽ					Windows����ǽ					��ͳIPSec���Է���
 *			   (Ws2_32.dllģ��)														   (mpssvc)						  (Policyagent)
 *			      --------------------------------------------------------------------------------------------------------------
 *																		|
 *																C����API(fwpuclnt.dll)
 *																		|
 *																		|
 *																		|
 *		RPC����Ӧ�ó���
 *		  RPC����ʱ    ----  ����API(fwpuclnt.dll) -------->	RPC�ӿ�---------------------------------------------------------
 *		 (rpcrt4.dll)											   |														   |
 * 																   |										�û�̬��������	   |
 * 																   |														   |
 * 																   |														   |
 * 																   |						������������					   |
 * 																   |														   | <----> IKEЭ��|AuthIPЭ��
 * 																   |	�û�̬RPC��											   |
 * 																   |										IKE�Լ�IPSec��	   |
 * 																   |														   |
 * 												|----------------> |														   |<---------------|
 * 												|				   |														   |				|
 * 												|				   -------------------------------------------------------------				|
 * �û�̬										|									 |															|
 * =============================================|====================================|==========================================================|==============
 * �ں�̬										|									 |															|
 *												|									 |															|
 * ---------TCP/IPЭ��ջ					IPSec���				|--------������ƽӿ�(IOCTL)------|					|---������---|		    |
 * 		    (tcpip.sys) |											|								  |					| 			 |			|
 * 					    |											|								  |					| 			 |			|
 * 				�������ֲ��Ƭ										|		  ��/���� ���ݷֲ�		  |					| ������	 |			|
 * 					    |											|								  |					| 			 |			|
 * 					    |											|								  |					| 			 |			|
 * 				ALE�������ӹ���									    |		����/���� ALE�ֲ�		  |					| ���п���	 |			|
 * 					    |											|								  |					| 			 |			|
 * 					    |					<---->			����API	|								  |	�����ӿ�API  <->| 			 |<->WFP�ں�̬�ͻ���
 * 						|											|								  |					| 			 |	 (fwpkclnt.sys)
 * 				����ֲ��Ƭ TCP/UDP								|		����/���� ����ֲ�		  |					| ���ּ��	 |
 * 					    |											|								  |					| 			 |
 * 					    |											|								  |					| IPSec		 |
 * 					    |											|								  |					| 			 |
 * 				����ֲ��Ƭ IPv4/IPv6							    |		����/���� IP�ֲ�		  |					| NAT		 |
 * 					    |											|								  |					| 			 |
 * ----------------------											-----------------------------------					--------------
 *																				�ں˹�������
 *
 */

 /**
  * �û�̬�ӿ�ͨ�����������������ջ����ں�̬�������潻��
  * �ں�̬����Ϊ���壬��ͬ�ֲ��������Э���ض��㣬ÿһ���п������Ӳ�͹�����
  * �ں���������������ݣ��Ƿ����й���������Rule������������ִ��ָ��������Action��
  * ����һ�������Ƿ��л������أ�һ�������¼��������ж���ֲ��ж���Ӳ�Ķ������������
  * Ϊ�˼�����˶�����WFP�й����ٲ�ģ�飬��������˶����󽻸��ں˹������棬��������չ��˽����������Ƭ
  */

  /**
   * ��Ƭ��Ϊһ�������ں�ģ�飬������ϵͳ������Э��ջ�в�ͬ�㣬��ͬ����Ի�ȡ��ͬ����
   * ���˻�ȡ���ݴ��ݸ��������棬��һ�������ǰ��ں˹�������Ĺ��˽��������Э��ջ
   * �Ǹ���WFP������Դ�Լ�ִ����������/�������ն���������������������������ע
   * ������ϣ������߿�����Ҫ�������������ݰ��Ĵ�����
   */

   /**
	* �����ӿڣ�Callout����һϵ�лص�����
	* ������GUIDֵ��Ψһ��ʶ��һ�������ӿ�
	* �ڲ�ͬ�ı��뻷����FWPS_CALLOUT�ṹ�屻�궨��Ϊ��ͬ�ı�ţ��ں���ͬ�ĳ�Ա
	* ����ÿһ���ֲ㣬��Ψһ�ı�ʶ
	*/
typedef struct _my_fwps_callout
{
	GUID calloutKey;
	UINT32 flags;
	FWPS_CALLOUT_CLASSIFY_FN classifyFn;//��ͬ�������в�ͬ��� ʹ�ú�������Ľṹ���������������ϵͳ�������ֵ�
	FWPS_CALLOUT_NOTIFY_FN notifyFn;
	FWPS_CALLOUT_FLOW_DELETE_NOTIFY_FN flowDeleteFn;//Ŀǰ����������ͬһ��
}my_fwps_callout;

/**
 * ����WFP���ֺõķֲ㣬�����߿��Ի����Ӳ㲢����Ȩ��
 * Ȩ��Խ�����ȼ�Խ��
 */
typedef struct _my_fwpm_sublayer
{
	GUID subLayerKey;//Ψһ��ʶ
	FWPM_DISPLAY_DATA displayData;//��������ͬ
	UINT16 flags;//����
	GUID* providerKey;
	FWP_BYTE_BLOB providerData;
	UINT16 weight;//Ȩ��
}my_fwpm_sublayer;

/**
 * ������
 * ��һ�׹���Ͷ����ļ��ϣ�ָ��Ҫ������Щ����������й���ʱ��ִ����Ӧ����
 * ͬһ���ֲ��ڣ���ͬ������Ȩ�ز�����ͬ�����Կ���ָ������һ���Ӳ㣬ֻҪ�Ӳ�Ȩ�ز�ͬ����
 * ��Ҫ���и��ӷ�������ʱ�������Թ��������ӿڣ�Callout�ص�ִ���꣬��������ص�WFP
 */
typedef struct _my_fwpm_filter
{
	GUID filterKey;//������Ψһ��ʶ����0����Զ�����һ��
	FWPM_DISPLAY_DATA displayData;//����������ֺ�����
	UINT32 flags;//
	GUID* providerKey;
	FWP_BYTE_BLOB providerData;
	GUID layerKey;//�ֲ�GUID
	GUID subLayerKey;//�Ӳ�GUID
	FWP_VALUE weight;//Ȩ�أ��ȷֲ��Ȩ�ظ��Ӻܶ࣬�Ǹ��ṹ�� ��Ҫʹ��type��uint64�ֱ��ʾȨ�ط�Χ�;���Ȩ��ֵ
	UINT32 numFilterConditions;//������������
	FWPM_FILTER_CONDITION* filterCondition;//�����������ṹ���ڱ������������ݰ���ʶ��ƥ�������Լ�����������ֵ
	FWPM_ACTION action;//���й�������ȫ������ʱִ�ж����������������ͣ�����/����/�ɻص������ӿں����پ��������������ͺ�callout��ʶ�����ڻص���Ӧ������
	union
	{
		UINT64 rawContext;
		GUID providerContextKey;
	};
	GUID* reserved;
	UINT64 filterId;
	FWP_VALUE effectiveWeight;
}my_fwpm_filter;

/**
 * �����ӿڻص�����
 * ��Ҫ��notifyFn��classifyFn��flowDeleteFn
 */
VOID NTAPI my_classifyFn(
	IN CONST FWPS_INCOMING_VALUES* inFixedValues, //����������ṹ���ڰ������������ݰ���Ϣ�����غ�Զ�̵�ַ���˿�
	IN CONST FWPS_INCOMING_METADATA_VALUES* inMetaValues, //Ԫ����ֵ���������������Ϣ������ID������������ȣ�����Ա�ܶ൫��������Ч����currentMetadataValues����
												//��һ������Է����ѯ�Ƿ����ĳ�������ʶ����ȷ�ϳ�Ա�Ƿ���Ч��FWPS_IS_METADATA_FIELD_PRESENT�����ط�0��ʾ��Ч
	IN OUT VOID* layerData, //�����˵�ԭʼ�������ݣ�����Щ�ֲ��п���ΪNULL
	IN OPTIONAL CONST VOID* classifyContext, //������ӿ�����������������
	IN CONST FWPS_FILTER* filter, //������ָ��
	IN UINT64 flowContext, //�������������������
	OUT FWPS_CLASSIFY_OUT* classifyOut //���˽���������������͡���һЩϵͳ����ֵ
)
{
	//�޷���ֵ
}
NTSTATUS NTAPI my_notifyFn(
	IN FWPS_CALLOUT_NOTIFY_TYPE notifyType, //֪ͨ���ͣ������˴λص���ԭ��
	IN CONST GUID* filterKey, //��������ʶ��ʹ��ǰ��Ҫ�пգ���Ϊֻ������ΪFWPS_CALLOUT_NOTIFY_ADD_FILTERʱ����ֵ�ŷǿ�
	IN CONST FWPS_FILTER* filter //������ָ�룬��ʶ��Ҫ����ӻ�ɾ���Ĺ�����
)
{
	//����ֵ��ʾ�Ƿ��������¼������缴����ӹ����������ش��������ʾ��������������ӣ���ɾ��һ����ɾ��
}
VOID NTAPI my_flowDeleteFn(
	IN UINT16 layerId, //�ֲ��ʶ
	IN UINT32 calloutId, //�����ӿڱ�ʶ
	IN UINT64 flowContext //������������
)
{
	//��һ��������Ҫ����ֹʱ���˺����ᱻ�ص���ֻ���������Ҫ��ֹ��������������������£��Żᱻ���ã�
}

/**
 * WFPʹ�û�������
 * ��������ӿڣ�������ע��ӿ�
 * ���ע��Ľӿڵ��������棨ע��������������ͬ������
 * ���һ�������Ӳ㣬��ӵ��ֲ���
 * ��ƹ��������ѽӿڡ��Ӳ㡢�ֲ�͹��������������������������ӹ�����
 */
VOID manual(PDRIVER_OBJECT DriverObject)
{
	NTSTATUS status;
	FWPS_CALLOUT callout = { 0 };//�ں�������������һ���Զ����Ψһ��ʶ
	UINT32 calloutId;//�������ɵ�Ψһ��ʶ�����Զ����ʶ��ͬ����WFP����ģ����Գ�Ϊ����ʱ��ʶ
	status = FwpsCalloutRegister(DriverObject, &callout, &calloutId);//ע��
	//FwpsCalloutUnregisterById(calloutId);//ͨ�����������ʱidж��
	//FwpsCalloutUnregisterByKey(&callout.calloutKey);//ͨ���Զ���GUID��ʶж��

	//�ɹ����Ѷ��壬����������ش�����
	if (status == STATUS_SUCCESS || status == STATUS_FWP_ALREADY_EXISTS)
	{
		//ע��ɹ���򿪹�������
		HANDLE hEngine = NULL;
		NTSTATUS status = STATUS_SUCCESS;
		FWPM_SESSION session = { 0 };//WFP��API���ڻỰ
		session.flags = FWPM_SESSION_FLAG_DYNAMIC;
		status = FwpmEngineOpen(NULL,//������NULL
			RPC_C_AUTHN_WINNT,//ֻ��Ϊ��ֵ��RPC_C_AUTHN_DEFAULT
			NULL,
			&session,//����ΪNULLҲ����
			&hEngine);//���ش򿪵Ĺ���������
		//FwpmEngineClose(hEngine); //�ر�����

		//��ע��õĽӿ���ӵ�����������
		UINT32 callout_id;
		status = FwpmCalloutAdd(hEngine, &callout, NULL, &callout_id);//״̬����ע��ʱ����һ��
		//FwpmCalloutDeleteById(callout_id); //ͨ�����ʱ���ص�id����ɾ��

		//����Ӳ�
		FWPM_SUBLAYER subLayer = { 0 };
		PSECURITY_DESCRIPTOR sd = NULL;//��ȫ�����������Լ򵥴�NULL
		status = FwpmSubLayerAdd(hEngine, &subLayer, sd);
		//FwpmSubLayerDeleteByKey(hEngine,&subLayer.subLayerKey); //ɾ���Ӳ�

		//��ӹ�����
		FWPM_FILTER filter = { 0 };//��Ҫ���õĳ�Ա��Խ϶�
		ULONG64 filterid;
		status = FwpmFilterAdd(hEngine, &filter, sd, &filterid);
		//FwpmFilterDeleteById(hEngine,filterid); //ɾ��������

		//���˽��ص��������Ӳ㡢������������������������
	}

}

//===============================================
//		һ���������ӣ����ض���80�˿ڵķ���
//===============================================

#include "WFPSample.h"
#include "Rule.h"

PDEVICE_OBJECT g_pDeviceObj = NULL;
#define WFP_DEVICE_NAME		L"\\Device\\wfp_sample_device"
#define WFP_SYM_LINK_NAME	L"\\DosDevices\\wfp_sample_device"

//�����豸
PDEVICE_OBJECT	CreateDevice(PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING	uDeviceName = { 0 };
	UNICODE_STRING	uSymbolName = { 0 };
	PDEVICE_OBJECT	pDeviceObj = NULL;
	NTSTATUS nStatsus = STATUS_UNSUCCESSFUL;
	RtlInitUnicodeString(&uDeviceName, WFP_DEVICE_NAME);
	RtlInitUnicodeString(&uSymbolName, WFP_SYM_LINK_NAME);
	nStatsus = IoCreateDevice(DriverObject, 0, &uDeviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &pDeviceObj);
	if (pDeviceObj != NULL)
	{
		pDeviceObj->Flags |= DO_BUFFERED_IO;
	}
	IoCreateSymbolicLink(&uSymbolName, &uDeviceName);
	return pDeviceObj;
}

//ɾ���豸
VOID DeleteDevice()
{
	UNICODE_STRING uSymbolName = { 0 };
	RtlInitUnicodeString(&uSymbolName, WFP_SYM_LINK_NAME);
	IoDeleteSymbolicLink(&uSymbolName);
	if (g_pDeviceObj != NULL)
	{
		IoDeleteDevice(g_pDeviceObj);
	}
	g_pDeviceObj = NULL;
}

//����ж�أ�������Դ
VOID  DriverUnload(PDRIVER_OBJECT DriverObject)
{
	UninitWfp();
	DeleteDevice();
	UninitRuleInfo();
	return;
}

//IRP����ַ�����
NTSTATUS WfpSampleIRPDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	NTSTATUS nStatus = STATUS_SUCCESS;
	ULONG	ulInformation = 0;
	UNREFERENCED_PARAMETER(DeviceObject);
	do
	{
		PIO_STACK_LOCATION	IrpStack = NULL;
		PVOID pSystemBuffer = NULL;
		ULONG uInLen = 0;
		if (Irp == NULL)
		{
			break;
		}
		pSystemBuffer = Irp->AssociatedIrp.SystemBuffer;
		IrpStack = IoGetCurrentIrpStackLocation(Irp);
		if (IrpStack == NULL)
		{
			break;
		}
		uInLen = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
		if (IrpStack->MajorFunction != IRP_MJ_DEVICE_CONTROL)
		{
			break;
		}
		// ��ʼ����DeivceIoControl�����
		switch (IrpStack->Parameters.DeviceIoControl.IoControlCode)
		{
		case IOCTL_WFP_SAMPLE_ADD_RULE:
		{
			BOOLEAN bSucc = FALSE;
			bSucc = AddNetRuleInfo(pSystemBuffer, uInLen);
			if (bSucc == FALSE)
			{
				nStatus = STATUS_UNSUCCESSFUL;
			}
			break;
		}
		default:
		{
			ulInformation = 0;
			nStatus = STATUS_UNSUCCESSFUL;
		}
		}
	} while (FALSE);
	if (Irp != NULL)
	{
		Irp->IoStatus.Information = ulInformation;
		Irp->IoStatus.Status = nStatus;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}
	return nStatus;
}

//�������
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	do
	{
		if (DriverObject == NULL)
		{
			break;
		}
		//����ַ�����
		DriverObject->MajorFunction[IRP_MJ_CREATE] = WfpSampleIRPDispatch;
		DriverObject->MajorFunction[IRP_MJ_CLOSE] = WfpSampleIRPDispatch;
		DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = WfpSampleIRPDispatch;
		//��ʼ������
		if (FALSE == InitRuleInfo())
		{
			break;
		}
		//�����豸
		g_pDeviceObj = CreateDevice(DriverObject);
		if (g_pDeviceObj == NULL)
		{
			break;
		}
		//��ʼ��WFP
		status = InitWfp(g_pDeviceObj);
		if (status != STATUS_SUCCESS)
		{
			break;
		}
		//��������ж�غ���
		DriverObject->DriverUnload = DriverUnload;
		status = STATUS_SUCCESS;
	} while (FALSE);

	if (status != STATUS_SUCCESS)
	{
		//������Դ��ɾ���豸
		UninitWfp();
		DeleteDevice();
	}

	return status;
}