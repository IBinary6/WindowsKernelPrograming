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
  * �ɽ�������ӱ���Ϊ��̬�⣬����һЩ��Ҫ�ص��ӿڣ�֮��Ϳ��������������Ͻ���TDI����ǽ������
  * ����ʹ�ÿ���ȥ���¡�Windows�ں˱�̡�����Դ��tdifw_smpl
  */

  /**
   * TDI���
   * ����Э���TDI�豸��
   * "\Device\Tcp"  "\Device\Udp"  "\Device\RawIp"(ԭʼIP��)
   *
   * �ٷ��ĵ����ܣ�
   * https://docs.microsoft.com/en-us/previous-versions/windows/hardware/network/ff565094(v=vs.85)
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

//��Դ����
#include "memtrack.h"

//ip�˿�ת��
#include "sock.h"

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

//���˽������
enum {
	FILTER_ALLOW = 1,
	FILTER_DENY,
	FILTER_PACKET_LOG,
	FILTER_PACKET_BAD,
	FILTER_DISCONNECT
};

//��Ӧ�ӹ��ܺ��и�����
enum {
	IRP_CREATE = 1,
	IRP_CONTROL,
	IRP_INTERNAL_CONTROL,
	IRP_CLEANUP,
	IRP_CLOSE
};

//�����ļ�����Ķ�Ӧ��ϵ���˸��˲��ִ��룬�����ļ�TDIFW��Ŀ
#include "obj_tbl.h"

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

//������������
struct _completion {
	PIO_COMPLETION_ROUTINE	routine;
	PVOID					context;
};
typedef struct {
	TDI_ADDRESS_INFO* tai;
	PFILE_OBJECT		fileobj;
} TDI_CREATE_ADDROBJ2_CTX;
#define TDI_ADDRESS_MAX_LENGTH	TDI_ADDRESS_LENGTH_OSI_TSAP
#define TA_ADDRESS_MAX			(sizeof(TA_ADDRESS) - 1 + TDI_ADDRESS_MAX_LENGTH)
#define TDI_ADDRESS_INFO_MAX	(sizeof(TDI_ADDRESS_INFO) - 1 + TDI_ADDRESS_MAX_LENGTH)
NTSTATUS tdi_create_addrobj_complete2(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
	//��ѯ�������ɺ���
	NTSTATUS status = STATUS_SUCCESS;
	if (Irp->MdlAddress)
	{
		//�õ�mdl��ָ��ַ
		TDI_ADDRESS_INFO* tai = (TDI_ADDRESS_INFO*)MmGetSystemAddressForMdl(Irp->MdlAddress);
		//�õ�һ����ַ�ṹ
		TA_ADDRESS* addr = tai->Address.Address;
		//��ӡ��Ϣ
		KdPrint(("[tdi_fw] tdi_create_addrobj_complete2: Address: %x:%u\n",
			ntohl(((TDI_ADDRESS_IP*)(addr->Address))->in_addr),
			ntohs(((TDI_ADDRESS_IP*)(addr->Address))->sin_port)));
	}
	return status;
}
NTSTATUS tdi_create_addrobj_complete(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
	//�����������ɺ���
	NTSTATUS status = STATUS_SUCCESS;
	PIO_STACK_LOCATION irps = IoGetCurrentIrpStackLocation(Irp);
	PIRP query_irp = (PIRP)Context;
	PDEVICE_OBJECT devobj;
	devobj = get_original_devobj(DeviceObject, NULL);
	TDI_CREATE_ADDROBJ2_CTX* ctx = NULL;
	PMDL mdl = NULL;

	if (Irp->IoStatus.Status != STATUS_SUCCESS) {
		KdPrint(("[tdi_fw] tdi_create_addrobj_complete: status 0x%x\n", Irp->IoStatus.Status));
		status = Irp->IoStatus.Status;
		return status;
	}
	//�����������ڴ�
	ctx = (TDI_CREATE_ADDROBJ2_CTX*)malloc_np(sizeof(TDI_CREATE_ADDROBJ2_CTX));
	if (ctx == NULL) {
		KdPrint(("[tdi_fw] tdi_create_addrobj_complete: malloc_np\n"));
		status = STATUS_INSUFFICIENT_RESOURCES;
		return status;
	}
	ctx->fileobj = irps->FileObject;

	ctx->tai = (TDI_ADDRESS_INFO*)malloc_np(TDI_ADDRESS_INFO_MAX);
	if (ctx->tai == NULL) {
		KdPrint(("[tdi_fw] tdi_create_addrobj_complete: malloc_np!\n"));
		status = STATUS_INSUFFICIENT_RESOURCES;
		return status;
	}
	//��IoAllocateMdl ����ʵ�ִ󻺴��Ƭ��Ŀ�ģ�ͨ��һ��������MDL��ӳ�仺���һС���֣�����ӳ������������ڴ�
	//����MmBuildMdlForNonPagedPool������MDL���ڴ棬ʹ��MDL������������Ļ��洦�ڲ����û����ڴ���
	mdl = IoAllocateMdl(ctx->tai, TDI_ADDRESS_INFO_MAX, FALSE, FALSE, NULL);
	if (mdl == NULL) {
		KdPrint(("[tdi_fw] tdi_create_addrobj_complete: IoAllocateMdl!\n"));
		status = STATUS_INSUFFICIENT_RESOURCES;
		return status;
	}
	MmBuildMdlForNonPagedPool(mdl);

	//����������ɺ󣬴�����ѯ����֮����ע��һ����ѯ�������ɺ���tdi_create_addrobj_complete2
	//mdl���û�̬���ں�̬�����һ���ڴ棬����ͨ��MDL�����ڴ����ӳ�䡣��ͬһ�������ڴ�ͬʱӳ�䵽�û�̬�ռ�ͺ���̬�ռ�
	TdiBuildQueryInformation(query_irp, devobj, irps->FileObject, tdi_create_addrobj_complete2, ctx, TDI_QUERY_ADDRESS_INFO, mdl);
	//������ѯ����
	status = IoCallDriver(devobj, query_irp);
	query_irp = NULL;
	mdl = NULL;
	ctx = NULL;

	if (status != STATUS_SUCCESS) {
		KdPrint(("[tdi_fw] tdi_create_addrobj_complete: IoCallDriver: 0x%x\n", status));
		return status;
	}

	//������Դ
	if (mdl != NULL)
		IoFreeMdl(mdl);
	if (ctx != NULL) {
		if (ctx->tai != NULL)
			free(ctx->tai);
		free(ctx);
	}

	if (query_irp != NULL)
		IoCompleteRequest(query_irp, IO_NO_INCREMENT);
	Irp->IoStatus.Status = status;

	return status;
}
int deal_tdi_create(PIRP irp, PIO_STACK_LOCATION irps, struct _completion* completion, PDEVICE_OBJECT devobj, int ipproto)
{
	NTSTATUS status;
	//���ڴ˴���ȡ��ǰ����
	ULONG pid = (ULONG)PsGetCurrentProcessId();
	//����һ����ZwCreateFile����������������EaBufferָ������EA���ݣ�������������̻�ȡ
	FILE_FULL_EA_INFORMATION* ea = (FILE_FULL_EA_INFORMATION*)irp->AssociatedIrp.SystemBuffer;
	//TDI�������ָ�������ea->EaName��
	//Ԥ��������TdiTransportAddress ��ʾĿǰ���ɵ�ʱһ��������ַ��һ�����IP��
	//TdiConnectionContext ��ʾĿǰ����һ�������ն�
	//���߶���һ����Ӧ���ļ�����
	//TDI�������ӵķ�ʽ����������������һ��������ַ��������һ�������նˣ�������һ���������󽫶�����ϵ����
	if (ea != NULL)
	{
		if (ea->EaNameLength == TDI_TRANSPORT_ADDRESS_LENGTH && memcmp(ea->EaName, TdiTransportAddress, TDI_TRANSPORT_ADDRESS_LENGTH) == 0)
		{
			//���ﲶ������ַ����
			//ֻ��������󱻷��͵��²���ɺ󣬲��ܷ��Ͳ�ѯ����ѯ��IP�Ͷ˿�
			//ѯ�ʱ����ļ����������IP�Ͷ˿ڣ�ѯ����Ҫ����һ������
			PIRP query_irp;
			query_irp = TdiBuildInternalDeviceControlIrp(TDI_QUERY_INFORMATION, devobj, irps->FileObject, NULL, NULL);
			//������ɺ���һ����DISPATCH�жϼ����ã���ֻ��PASSIVE�жϼ����ܵ���TdiBuildInternalDeviceControlIrp�����ַ�����һ�����PASSIVE�жϼ�����������
			if (query_irp == NULL)
			{
				return FILTER_DENY;
			}
			//������ɺ�������������һ��ɣ��͵���������������������з���IP�Ͷ˿ڣ��˺����ں��汻����ΪIRP��ɺ���
			completion->routine = tdi_create_addrobj_complete;
			//��¼�ѷ������󣬷���ʹ��
			completion->context = query_irp;

			//�ļ����������ɵ�ַҲӦ�ö�Ӧ�ı�������
			status = ot_add_fileobj(irps->DeviceObject, irps->FileObject, FILEOBJ_ADDROBJ, ipproto, NULL);
			if (status != STATUS_SUCCESS) {
				KdPrint(("[tdi_fw] tdi_create: ot_add_fileobj: FILEOBJ_ADDROBJ 0x%x\n", status));
				return FILTER_DENY;
			}
		}
		else if (ea->EaNameLength == TDI_CONNECTION_CONTEXT_LENGTH && memcmp(ea->EaName, TdiConnectionContext, TDI_CONNECTION_CONTEXT_LENGTH) == 0)
		{
			//���������ն�����
			CONNECTION_CONTEXT conn_ctx = *(CONNECTION_CONTEXT*)(ea->EaName + ea->EaNameLength + 1);//�����ַת��Ϊָ����ȡֵ
			//һ���ն����ɺ�һ���ļ���������

			//��֮�����нػ񵽵�DeviceIoControl�ػ񵽵Ķ�ֻ���ļ���������Ӧ�����ڴ���ά��һ����ϣ����ļ���������������Ķ�Ӧ����
			status = ot_add_fileobj(irps->DeviceObject, irps->FileObject, FILEOBJ_CONNOBJ, ipproto, conn_ctx);
			if (status != STATUS_SUCCESS) {
				KdPrint(("[tdi_fw] tdi_create: ot_add_fileobj: FILEOBJ_CONNOBJ 0x%x\n", status));
				return FILTER_DENY;
			}
		}
	}

	return FILTER_ALLOW;
}
//�����ӽ����¼��������Զ���ص��ص�����
typedef struct {
	PFILE_OBJECT fileobj;
	PVOID old_handler;
	PVOID old_context; //���ǻص������ĵ�һ������
}TDI_EVENT_CONTEXT; //�Զ�����������Ϣ�ṹ
NTSTATUS my_handler(PVOID TdiEventContext, LONG RemoteAddressLength, PVOID RemoteAddress, LONG UserDataLength, PVOID UserData,
	LONG OptionsLength, PVOID Options, CONNECTION_CONTEXT* ConnectionContext, PIRP* AcceptIrp)
{
	NTSTATUS status = STATUS_SUCCESS;
	//ִ���Լ��߼�

	//����ԭ���Ļص�����
	TDI_EVENT_CONTEXT* ctx = (TDI_EVENT_CONTEXT*)TdiEventContext;
	status = ((PTDI_IND_CONNECT)(ctx->old_handler))(ctx->old_context, //תΪԭ����ָ������
		RemoteAddressLength, RemoteAddress, UserDataLength, UserData, OptionsLength, Options, ConnectionContext, AcceptIrp);
	return status;//������STATUS_CONNECTION_REFUSED�����󽫲��ܽ���
}
//��һ�������߼�/�ι��ܺţ�֮����ɱ�������
NTSTATUS
tdi_dispatch_complete(PDEVICE_OBJECT devobj, PIRP irp, int filter, PIO_COMPLETION_ROUTINE cr, PVOID context, int irp_type)
{
	PIO_STACK_LOCATION irps = IoGetCurrentIrpStackLocation(irp);
	NTSTATUS status = STATUS_SUCCESS;

	if (filter == FILTER_DENY) {
		//ǰ�淢������򲻷�������
		KdPrint(("[tdi_fw] tdi_dispatch_complete: [DROP!]"
			" major 0x%x, minor 0x%x for devobj %p; fileobj %p\n",
			irps->MajorFunction,
			irps->MinorFunction,
			devobj,
			irps->FileObject));

		if (irp->IoStatus.Status == STATUS_SUCCESS) {
			status = irp->IoStatus.Status = STATUS_ACCESS_DENIED;
		}
		else {
			status = irp->IoStatus.Status;
		}

		IoCompleteRequest(irp, IO_NO_INCREMENT);
	}
	else if (filter == FILTER_ALLOW) {

		//�����̹��˲���
		if (irp_type == IRP_CREATE)
		{
			//����������ɺ������������ע��ص�����
			IoSetCompletionRoutine(irp, cr, context, NULL, NULL, NULL);
		}
		else if (irp_type == IRP_CONTROL)
		{
			//�ں���netbt�豸����TCP����ʱ���޷���TDI������������TDI_SEND����
			//netbtʱ����TCP/IP��NetBIOSЭ�飬���ڼ�������ֽ���
			//netbt��ֱ�ӻ�ȡTCPЭ���������ڲ�����TCPSendDataָ�룬���������ֱ�ӷ������ݣ����ƹ���TDI_SEND
			//ͨ����tcpip.sys���о�����һ�����ܺ�ΪIOCTL_TDI_QUERY_DIRECT_SEND_HANDLER
			BOOLEAN bRet = irps->Parameters.DeviceIoControl.IoControlCode == IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER;
			//��������������tcpip.sys����TCPSendData����ָ�뷵�أ��������䷢������
			//����ֻҪ�Դ����⴦�������ص�TCPSendData�������棬ȡ��Ϊ�Լ��Ĺ��˺�������
			if (bRet)
			{
				VOID* buff = irps->Parameters.DeviceIoControl.Type3InputBuffer;
				if (buff != NULL)
				{
					//old_TCPSendData = *(TCPSendData_t**)buff;
					//*(TCPSendData_t**)buff = my_TCPSendData;
				}
			}
		}
		else if (irp_type == IRP_INTERNAL_CONTROL)
		{
			//������ι��ܺŵ�����

			if (irps->MinorFunction == TDI_ASSOCIATE_ADDRESS)
			{
				//TDI_ASSOCIATE_ADDRESS�����ڰѴ�����������Ӷ�����ϵ����
				//�������������У�IRP��ǰջ�ռ��FileObjcet���б�����ļ�����ָ��ʱ�����ն˵��ļ�����
				//����֮ǰ���������������ָ�룬���ļ�����ָ��Ϊ��������ȡ��
				//��������Ϣ��ϵ����֮�󣬵����ǵõ�һ�����������Ķ���ʱ������֪����ʹ�õı��ص�ַ
				//��֮��Ӧ�ģ�TDI_DISASSOCIATE_ADDRESS��ɾ����ַ���ļ�����������������֮��Ĺ�ϵ
			}

			if (irps->MinorFunction == TDI_CONNECT)
			{
				//TDI_CONNECT
				//����������ڱ�����ͼ�������ʱ��������������ʱ�����Ӿ��Ѿ������ˣ�����Ҳ�Ǽ�����簲ȫ����Ҫ�ĵط�
				//���ڴ˴����ƣ�����ʹ�ñ���ʲô��ַ��������ͼ����ʲôԶ�̵�ַ���Ƿ�������ʷ������Ƿ��¼��־��

				//��ȡԶ�̵�ַ
				PTDI_REQUEST_KERNEL_CONNECT param = (PTDI_REQUEST_KERNEL_CONNECT)(&irps->Parameters);
				TA_ADDRESS* remote_addr = ((TRANSPORT_ADDRESS*)(param->RequestConnectionInformation->RemoteAddress))->Address;

				//���ݵõ�����Ϣ����Ӧ����
			}

			//����һЩ���ܺţ�TDI_SEND��TDI_RECEIVE��TDI_SEND_DATAGRAM��TDI_RECEIVE_DATAGRAM ���������
			//���ִ��䷽ʽ����ʽ���䣨��Ӧǰ�������ܺţ��ͱ�ʽ����
			//��ʽ���䲻����ÿ�δ�����٣�ֻ���������Ϸ��͵��ܴ�С���ȷ��͵���Զ�ȵ���TCP
			//��ʽ����û�����ӣ�һ�η���һ�����ݰ������͵İ���һ�����ȷ��͵İ�֮�󱻽��գ�UDP
			//�����ڴ�����������˴���������ݡ���ⲡ�����������ݡ��޸����ݡ��ܾ����͡���������
			if (irps->MinorFunction == TDI_SEND)
			{
				//��ȡ�ļ�����
				PFILE_OBJECT fileObj = irps->FileObject;
				//find������ȡ֮ǰ��������Ӷ���

				//��ȡ���ݣ���ȡMDL�����ݼ���
				VOID* buff = MmGetSystemAddressForMdl(irp->MdlAddress);//��ȡʵ�ʻ�����λ��
				ULONG len = irp->IoStatus.Information;//��ȡ����

				//��ֹ���ͻ���գ���������󷵻ش��󼴿�
			}

			//TDI_SET_EVENT_HANDLER
			//socket����listenʱ��һ������ΪTDI_EVENT_CONNECT�����¼����󽫷��͵��²�Э�飬�²�Э��õ�һ���ص�����ָ��
			//TDI_SET_EVENT_HANDLER��һ�������¼��ص�������
			if (irps->MinorFunction == TDI_SET_EVENT_HANDLER)
			{
				//�¼�����ܶ࣬��Ҫ��ȡ����
				PTDI_REQUEST_KERNEL_SET_EVENT r = (PTDI_REQUEST_KERNEL_SET_EVENT)&irps->Parameters;
				LONG type = r->EventType;//�¼�����
				r->EventHandler;//�ص������������¼����Ͳ�ͬ���ص�����Ҳ��ͬ
				//ǰ�����TDI_CONNECT����ֻ����Ա������ⲿ
				//�����ⲿ���ӱ��أ���ͨ��TDI_EVENT_CONNECT�����¼������ûص����������ǿ������Լ��Ļص�����ȡ������������ԭ�ص�����ָ��
				//ִ�����Լ��Ļص����ٵ���ԭ���ĺ�����ʵ������
				if (type == TDI_EVENT_CONNECT && r->EventHandler != NULL)
				{
					TDI_EVENT_CONTEXT* ctx;//�Զ��������Ľṹ�洢ԭ���ص�������Ϣ
					ctx->fileobj = irps->FileObject;
					ctx->old_handler = r->EventHandler;
					ctx->old_context = r->EventContext;
					//�滻Ϊ�Լ���
					r->EventHandler = my_handler;
					r->EventContext = ctx;
				}
			}

		}
		else if (irp_type == IRP_CLOSE)
		{
		}

	}
	else {
		//δ֪���
		status = irp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
	}

	return status;
}
//�ַ�����
NTSTATUS DeviceDispatch(PDEVICE_OBJECT DeviceObject, PIRP irp)
{
	NTSTATUS status;
	int ipproto;
	PDEVICE_OBJECT old_devobj = get_original_devobj(DeviceObject, &ipproto);
	if (old_devobj != NULL)
	{
		//��ȡ��ǰջ�ռ�
		PIO_STACK_LOCATION irps;
		irps = IoGetCurrentIrpStackLocation(irp);
		//��һ���ṹ�������������е���Ϣ
		struct _completion completion = { 0 };
		int result = 0;
		//�������ܺ�
		switch (irps->MajorFunction)
		{
		case IRP_MJ_CREATE: //��������
		{
			//������������
			result = deal_tdi_create(irp, irps, &completion, old_devobj, ipproto);
			//���������溯����ɣ���ɺ�completion.routine��¼����ɺ���������
			status = tdi_dispatch_complete(DeviceObject, irp, result, completion.routine, completion.context, IRP_CREATE);
			break;
		}
		case IRP_MJ_DEVICE_CONTROL: //�豸����
			//����������IRP_MJ_INTERNAL_DEVICE_CONTROL�����ظ�������netbt�����������⣩
			status = TdiMapUserRequest(DeviceObject, irp, irps);//���Ե��øú�����IRP_MJ_DEVICE_CONTROLת��ΪIRP_MJ_INTERNAL_DEVICE_CONTROL

			status = tdi_dispatch_complete(DeviceObject, irp, FILTER_ALLOW, completion.routine, completion.context, IRP_CONTROL);

			//break; //������Բ�break��ֱ����Ϊ�ڲ����������������
		case IRP_MJ_INTERNAL_DEVICE_CONTROL: //�ڲ��豸����
			//�������ֹ��ܺ���TDI�й��ܻ�����ͬ
			status = tdi_dispatch_complete(DeviceObject, irp, FILTER_ALLOW, completion.routine, completion.context, IRP_INTERNAL_CONTROL);
			break;
		case IRP_MJ_CLEANUP://����
			//�˴�һ������ɾ����������Ӻ͵�ַ��Ϣ
			//�յ�����������Ϊ��һ���ļ�����ľ������0����������ζ���ļ���������ü���һ����Ϊ0����Ϊ���ü�����ֹ�Ǵ򿪾��ʱ����
			//�����ü�������Ϊ0ʱ���ļ���������٣���ʱ�Ż��յ�CLOSE����
			status = tdi_dispatch_complete(DeviceObject, irp, FILTER_ALLOW, completion.routine, completion.context, IRP_CLEANUP);
			break;
		case IRP_MJ_CLOSE: //�ر�
			status = tdi_dispatch_complete(DeviceObject, irp, FILTER_ALLOW, completion.routine, completion.context, IRP_CLOSE);
			break;

		default:
			break;
		}

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
//�������
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