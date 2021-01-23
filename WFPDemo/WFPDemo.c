#include <ntddk.h>
#include <fwpsk.h> //ʹ��fwp��ҪԤ����NDIS֧�ְ汾��NDIS_SUPPORT_NDIS6
#include <fwpmk.h>
/**
 * WFP��Windows Filter Platform��Windows����ƽ̨��
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
	FWPS_CALLOUT_CLASSIFY_FN classifyFn;//��ͬ�������в�ͬ���
	FWPS_CALLOUT_NOTIFY_FN notifyFn;//��ͬ�������в�ͬ���
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


NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	return STATUS_SUCCESS;
}