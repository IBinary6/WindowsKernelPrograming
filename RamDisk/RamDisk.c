#include <ntddk.h>
#include <ntdddisk.h>
#include <wdf.h> //��Ҫ���ð���Ŀ¼����������������½���Ŀʱʹ��KMDF��Ŀ���
#include <ntstrsafe.h>

/**
 * �������⼼��
 *
 * ʹ�÷Ƿ�ҳ�ڴ����Ĵ��̴洢�ռ䣬��������һ������������ʽ��¶���û�
 *
 * ʹ����WDF���Ƕ�WDM�ķ�װ
 * ����Ե�Դ����ͼ��弴�������ĳ��á����ӵĴ��������з�װ�����Ը�����Ľ��п���
 */

#define NT_DEVICE_NAME                  L"\\Device\\Ramdisk"
#define DOS_DEVICE_NAME                 L"\\DosDevices\\"

#define RAMDISK_TAG                     'DmaR'  // "RamD"
#define DOS_DEVNAME_LENGTH              (sizeof(DOS_DEVICE_NAME)+sizeof(WCHAR)*10)
#define DRIVE_LETTER_LENGTH             (sizeof(WCHAR)*10)

#define DRIVE_LETTER_BUFFER_SIZE        10
#define DOS_DEVNAME_BUFFER_SIZE         (sizeof(DOS_DEVICE_NAME) / 2) + 10

#define RAMDISK_MEDIA_TYPE              0xF8
#define DIR_ENTRIES_PER_SECTOR          16

#define DEFAULT_DISK_SIZE               (1024*1024)     // 1 MB
#define DEFAULT_ROOT_DIR_ENTRIES        512
#define DEFAULT_SECTORS_PER_CLUSTER     2
#define DEFAULT_DRIVE_LETTER            L"Z:"

typedef struct _DISK_INFO {
	ULONG   DiskSize;           //���̴�С���ֽڼ��㣨ULONG���ֻ��4GB��
	ULONG   RootDirEntries;     //�����ϸ��ļ�ϵͳ����ڵ�
	ULONG   SectorsPerCluster;  //����ÿ�����ɶ����������
	UNICODE_STRING DriveLetter; //�����̷�
} DISK_INFO, * PDISK_INFO;

typedef struct _DEVICE_EXTENSION {
	PUCHAR              DiskImage;                  //ָ��һ���ڴ棬��Ϊ�ڴ���ʵ�����ݴ洢�ռ�
	DISK_GEOMETRY       DiskGeometry;               // Drive parameters built by Ramdisk
	DISK_INFO           DiskRegInfo;                //�Զ��������Ϣ
	UNICODE_STRING      SymbolicLink;               //���̷���������
	WCHAR               DriveLetterBuffer[DRIVE_LETTER_BUFFER_SIZE];//DiskRegInfo��DriverLetter�Ĵ洢�ռ䣬���û���ע�����ָ�����̷�
	WCHAR               DosDeviceNameBuffer[DOS_DEVNAME_BUFFER_SIZE];//SymbolicLink�Ĵ洢�ռ�
} DEVICE_EXTENSION, * PDEVICE_EXTENSION;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_EXTENSION, DeviceGetExtension)

typedef struct _QUEUE_EXTENSION {
	PDEVICE_EXTENSION DeviceExtension;
} QUEUE_EXTENSION, * PQUEUE_EXTENSION;
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_EXTENSION, QueueGetExtension)


#pragma pack(1)

typedef struct  _BOOT_SECTOR
{
	UCHAR       bsJump[3];          // x86 jmp instruction, checked by FS
	CCHAR       bsOemName[8];       // OEM name of formatter
	USHORT      bsBytesPerSec;      // Bytes per Sector
	UCHAR       bsSecPerClus;       // Sectors per Cluster
	USHORT      bsResSectors;       // Reserved Sectors
	UCHAR       bsFATs;             // Number of FATs - we always use 1
	USHORT      bsRootDirEnts;      // Number of Root Dir Entries
	USHORT      bsSectors;          // Number of Sectors
	UCHAR       bsMedia;            // Media type - we use RAMDISK_MEDIA_TYPE
	USHORT      bsFATsecs;          // Number of FAT sectors
	USHORT      bsSecPerTrack;      // Sectors per Track - we use 32
	USHORT      bsHeads;            // Number of Heads - we use 2
	ULONG       bsHiddenSecs;       // Hidden Sectors - we set to 0
	ULONG       bsHugeSectors;      // Number of Sectors if > 32 MB size
	UCHAR       bsDriveNumber;      // Drive Number - not used
	UCHAR       bsReserved1;        // Reserved
	UCHAR       bsBootSignature;    // New Format Boot Signature - 0x29
	ULONG       bsVolumeID;         // VolumeID - set to 0x12345678
	CCHAR       bsLabel[11];        // Label - set to RamDisk
	CCHAR       bsFileSystemType[8];// File System Type - FAT12 or FAT16
	CCHAR       bsReserved2[448];   // Reserved
	UCHAR       bsSig2[2];          // Originial Boot Signature - 0x55, 0xAA
}   BOOT_SECTOR, * PBOOT_SECTOR;

typedef struct  _DIR_ENTRY
{
	UCHAR       deName[8];          // File Name
	UCHAR       deExtension[3];     // File Extension
	UCHAR       deAttributes;       // File Attributes
	UCHAR       deReserved;         // Reserved
	USHORT      deTime;             // File Time
	USHORT      deDate;             // File Date
	USHORT      deStartCluster;     // First Cluster of file
	ULONG       deFileSize;         // File Length
}   DIR_ENTRY, * PDIR_ENTRY;

#pragma pack()

#define DIR_ATTR_READONLY   0x01
#define DIR_ATTR_HIDDEN     0x02
#define DIR_ATTR_SYSTEM     0x04
#define DIR_ATTR_VOLUME     0x08
#define DIR_ATTR_DIRECTORY  0x10
#define DIR_ATTR_ARCHIVE    0x20

VOID
RamDiskEvtDeviceContextCleanup(
	IN WDFDEVICE Device
)
/*++

Routine Description:

   EvtDeviceContextCleanup event callback cleans up anything done in
   EvtDeviceAdd, except those things that are automatically cleaned
   up by the Framework.

   In the case of this sample, everything is automatically handled.  In a
   driver derived from this sample, it's quite likely that this function could
   be deleted.

Arguments:

	Device - Handle to a framework device object.

Return Value:

	VOID

--*/
{
	PDEVICE_EXTENSION pDeviceExtension = DeviceGetExtension(Device);

	PAGED_CODE();

	if (pDeviceExtension->DiskImage) {
		ExFreePool(pDeviceExtension->DiskImage);
	}
}

BOOLEAN
RamDiskCheckParameters(
	IN PDEVICE_EXTENSION devExt,
	IN LARGE_INTEGER ByteOffset,
	IN size_t Length
)

{
	//
	// Check for invalid parameters.  It is an error for the starting offset
	// + length to go past the end of the buffer, or for the length to
	// not be a proper multiple of the sector size.
	//
	// Others are possible, but we don't check them since we trust the
	// file system.
	//

	if (devExt->DiskRegInfo.DiskSize < Length ||
		ByteOffset.QuadPart < 0 || // QuadPart is signed so check for negative values
		((ULONGLONG)ByteOffset.QuadPart > (devExt->DiskRegInfo.DiskSize - Length)) ||
		(Length & (devExt->DiskGeometry.BytesPerSector - 1))) {

		//
		// Do not give an I/O boost for parameter errors.
		//

		KdPrint((
			"Error invalid parameter\n"
			"ByteOffset: %x\n"
			"Length: %d\n",
			ByteOffset,
			Length
			));

		return FALSE;
	}

	return TRUE;
}

/**
 * �Թ�ע����������Զ���ص�
 * �ص���������Ҫ�����²�����һ�֣�
 * �����Ŷӣ����Խ����������һ������
 * ������󣬶�������һЩ����������
 * �������󣬻ص���������Ҫ����������
 * ת�����󣬿���ת���������豸
 * ���ܺ��Ը�����
 *
 * ��������д���Ƕ�������������������
 * ��RamDiskEvtDeviceAdd���������Ĵ������Ժܷ����ͨ�����ж����ȡ���豸��չ��������ȡ������ز���
 * ��������ϵͳ�ṩ�������Ŷ��������ݻ���Ҫд�������
 * ��ȡ��Ӧ������ֻ��Ҫ���ڴ澵�����ʵ��ص�ͳ��ȶ�ȡ��д�뼴��
 */
VOID
RamDiskEvtIoDeviceControl(
	IN WDFQUEUE Queue,
	IN WDFREQUEST Request,
	IN size_t OutputBufferLength,
	IN size_t InputBufferLength,
	IN ULONG IoControlCode
)
/*++

Routine Description:
	����һЩ��Ҫ�Ŀ�������
	This event is called when the framework receives IRP_MJ_DEVICE_CONTROL
	requests from the system.

Arguments:

	Queue - Handle to the framework queue object that is associated
			with the I/O request.
	Request - Handle to a framework request object.

	OutputBufferLength - length of the request's output buffer,
						if an output buffer is available.
	InputBufferLength - length of the request's input buffer,
						if an input buffer is available.

	IoControlCode - the driver-defined or system-defined I/O control code
					(IOCTL) that is associated with the request.


Return Value:

	VOID

--*/
{

	NTSTATUS          Status = STATUS_INVALID_DEVICE_REQUEST;
	ULONG_PTR         information = 0;//��ŷ��ص�DeviceIoControl����Ҫ�����ݳ���
	size_t            bufSize;
	PDEVICE_EXTENSION devExt = QueueGetExtension(Queue)->DeviceExtension;//ͨ�����ж����ȡ�豸��չ
	//�����Ǳ�׼��Windows�������Կ��Բ����ĳ�������
	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);
	//�ж��������ͣ�һ����̾���Ҫ֧�ִ�����������������ӵ����߸��ݾ����ܽ���һЩ��Ҫ�Ĵ���
	switch (IoControlCode) {
		//��ȡ��ǰ������Ϣ������
	case IOCTL_DISK_GET_PARTITION_INFO:
	{
		//����һ�����������ָ��
		PPARTITION_INFORMATION outputBuffer;
		PBOOT_SECTOR bootSector = (PBOOT_SECTOR)devExt->DiskImage;//������Ϣ�󲿷���DBR�л�ȡ��������Ҫ��DBRָ��
		//������Ϣ�ĳ��ȣ��ᱻ�ϲ㷢��������豸�յ�
		information = sizeof(PARTITION_INFORMATION);
		//ͨ����ܺ�����ȡ�������Я�������������
		Status = WdfRequestRetrieveOutputBuffer(Request, sizeof(PARTITION_INFORMATION), &outputBuffer, &bufSize);
		if (NT_SUCCESS(Status)) {
			//��ȡ�������ɹ����������Ϣ����
			outputBuffer->PartitionType = (bootSector->bsFileSystemType[4] == '6') ? PARTITION_FAT_16 : PARTITION_FAT_12;
			outputBuffer->BootIndicator = FALSE;
			outputBuffer->RecognizedPartition = TRUE;
			outputBuffer->RewritePartition = FALSE;
			outputBuffer->StartingOffset.QuadPart = 0;
			outputBuffer->PartitionLength.QuadPart = devExt->DiskRegInfo.DiskSize;
			outputBuffer->HiddenSectors = (ULONG)(1L);
			outputBuffer->PartitionNumber = (ULONG)(-1L);

			Status = STATUS_SUCCESS;
		}
		break;
	}

	//�򻺳�����д������Ϣ
	case IOCTL_DISK_GET_DRIVE_GEOMETRY:
	{

		PDISK_GEOMETRY outputBuffer;

		information = sizeof(DISK_GEOMETRY);

		Status = WdfRequestRetrieveOutputBuffer(Request, sizeof(DISK_GEOMETRY), &outputBuffer, &bufSize);
		if (NT_SUCCESS(Status)) {

			RtlCopyMemory(outputBuffer, &(devExt->DiskGeometry), sizeof(DISK_GEOMETRY));
			Status = STATUS_SUCCESS;
		}

		break;
	}
	//��֤���úͿ�д��ֱ�ӷ��سɹ�������Ҫ������Ϣ
	case IOCTL_DISK_CHECK_VERIFY:
	case IOCTL_DISK_IS_WRITABLE:
		Status = STATUS_SUCCESS;
		break;
	}
	//�������
	WdfRequestCompleteWithInformation(Request, Status, information);
}

VOID
RamDiskEvtIoRead(
	IN WDFQUEUE Queue,
	IN WDFREQUEST Request,
	IN size_t Length
)
{
	//ͨ�����л�ȡ�豸��չ
	PDEVICE_EXTENSION      devExt = QueueGetExtension(Queue)->DeviceExtension;
	NTSTATUS               Status = STATUS_INVALID_PARAMETER;
	WDF_REQUEST_PARAMETERS Parameters;//���ڻ�ȡ��������ı���
	LARGE_INTEGER          ByteOffset;//���ڻ�ȡ��ʼ��ַ����
	WDFMEMORY              hMemory;//�������ڴ���
	//��ʼ����������
	WDF_REQUEST_PARAMETERS_INIT(&Parameters);
	//����������л�ȡ��Ϣ
	WdfRequestGetParameters(Request, &Parameters);
	//����������ж�����ʼλ��
	ByteOffset.QuadPart = Parameters.Parameters.Read.DeviceOffset;
	//�����������ڶ�ȡ��Χ���ܳ������̾����С���ұ����������룬������Ҫ��飬��ʧ���������Դ�������������
	if (RamDiskCheckParameters(devExt, ByteOffset, Length)) {
		//����������ж�ȡ���������ڴ���
		Status = WdfRequestRetrieveOutputMemory(Request, &hMemory);
		if (NT_SUCCESS(Status)) {
			//����֮ǰ��ȡ���Ĳ��������ڴ濽������д������Ļ�����
			Status = WdfMemoryCopyFromBuffer(hMemory, // Destination
				0, // Offset into the destination
				devExt->DiskImage + ByteOffset.LowPart, // source
				Length);
		}
	}
	//������������Ҫ����ȡ������Ϊ������Ϣ
	WdfRequestCompleteWithInformation(Request, Status, (ULONG_PTR)Length);
}

VOID
RamDiskEvtIoWrite(
	IN WDFQUEUE Queue,
	IN WDFREQUEST Request,
	IN size_t Length
)
{
	//д�Ͷ�����һ��
	PDEVICE_EXTENSION      devExt = QueueGetExtension(Queue)->DeviceExtension;
	NTSTATUS               Status = STATUS_INVALID_PARAMETER;
	WDF_REQUEST_PARAMETERS Parameters;
	LARGE_INTEGER          ByteOffset;
	WDFMEMORY              hMemory;

	WDF_REQUEST_PARAMETERS_INIT(&Parameters);

	WdfRequestGetParameters(Request, &Parameters);

	ByteOffset.QuadPart = Parameters.Parameters.Write.DeviceOffset;

	if (RamDiskCheckParameters(devExt, ByteOffset, Length)) {

		Status = WdfRequestRetrieveInputMemory(Request, &hMemory);
		if (NT_SUCCESS(Status)) {

			Status = WdfMemoryCopyToBuffer(hMemory, // Source
				0, // offset in Source memory where the copy has to start
				devExt->DiskImage + ByteOffset.LowPart, // destination
				Length);
		}

	}

	WdfRequestCompleteWithInformation(Request, Status, (ULONG_PTR)Length);
}

VOID
RamDiskQueryDiskRegParameters(
	__in PWSTR RegistryPath,
	__in PDISK_INFO DiskRegInfo
)

/*++

Routine Description:

	This routine is called from the DriverEntry to get the debug
	parameters from the registry. If the registry query fails, then
	default values are used.

Arguments:

	RegistryPath    - Points the service path to get the registry parameters

Return Value:

	None

--*/

{

	RTL_QUERY_REGISTRY_TABLE rtlQueryRegTbl[5 + 1];  // Need 1 for NULL
	NTSTATUS                 Status;
	DISK_INFO                defDiskRegInfo;

	PAGED_CODE();

	ASSERT(RegistryPath != NULL);

	// Set the default values

	defDiskRegInfo.DiskSize = DEFAULT_DISK_SIZE;
	defDiskRegInfo.RootDirEntries = DEFAULT_ROOT_DIR_ENTRIES;
	defDiskRegInfo.SectorsPerCluster = DEFAULT_SECTORS_PER_CLUSTER;

	RtlInitUnicodeString(&defDiskRegInfo.DriveLetter, DEFAULT_DRIVE_LETTER);

	RtlZeroMemory(rtlQueryRegTbl, sizeof(rtlQueryRegTbl));

	//
	// Setup the query table
	//

	rtlQueryRegTbl[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
	rtlQueryRegTbl[0].Name = L"Parameters";
	rtlQueryRegTbl[0].EntryContext = NULL;
	rtlQueryRegTbl[0].DefaultType = (ULONG_PTR)NULL;
	rtlQueryRegTbl[0].DefaultData = NULL;
	rtlQueryRegTbl[0].DefaultLength = (ULONG_PTR)NULL;

	//
	// Disk paramters
	//

	rtlQueryRegTbl[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
	rtlQueryRegTbl[1].Name = L"DiskSize";
	rtlQueryRegTbl[1].EntryContext = &DiskRegInfo->DiskSize;
	rtlQueryRegTbl[1].DefaultType = REG_DWORD;
	rtlQueryRegTbl[1].DefaultData = &defDiskRegInfo.DiskSize;
	rtlQueryRegTbl[1].DefaultLength = sizeof(ULONG);

	rtlQueryRegTbl[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
	rtlQueryRegTbl[2].Name = L"RootDirEntries";
	rtlQueryRegTbl[2].EntryContext = &DiskRegInfo->RootDirEntries;
	rtlQueryRegTbl[2].DefaultType = REG_DWORD;
	rtlQueryRegTbl[2].DefaultData = &defDiskRegInfo.RootDirEntries;
	rtlQueryRegTbl[2].DefaultLength = sizeof(ULONG);

	rtlQueryRegTbl[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
	rtlQueryRegTbl[3].Name = L"SectorsPerCluster";
	rtlQueryRegTbl[3].EntryContext = &DiskRegInfo->SectorsPerCluster;
	rtlQueryRegTbl[3].DefaultType = REG_DWORD;
	rtlQueryRegTbl[3].DefaultData = &defDiskRegInfo.SectorsPerCluster;
	rtlQueryRegTbl[3].DefaultLength = sizeof(ULONG);

	rtlQueryRegTbl[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
	rtlQueryRegTbl[4].Name = L"DriveLetter";
	rtlQueryRegTbl[4].EntryContext = &DiskRegInfo->DriveLetter;
	rtlQueryRegTbl[4].DefaultType = REG_SZ;
	rtlQueryRegTbl[4].DefaultData = defDiskRegInfo.DriveLetter.Buffer;
	rtlQueryRegTbl[4].DefaultLength = 0;


	Status = RtlQueryRegistryValues(
		RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
		RegistryPath,
		rtlQueryRegTbl,
		NULL,
		NULL
	);

	if (NT_SUCCESS(Status) == FALSE) {

		DiskRegInfo->DiskSize = defDiskRegInfo.DiskSize;
		DiskRegInfo->RootDirEntries = defDiskRegInfo.RootDirEntries;
		DiskRegInfo->SectorsPerCluster = defDiskRegInfo.SectorsPerCluster;
		RtlCopyUnicodeString(&DiskRegInfo->DriveLetter, &defDiskRegInfo.DriveLetter);
	}

	KdPrint(("DiskSize          = 0x%lx\n", DiskRegInfo->DiskSize));
	KdPrint(("RootDirEntries    = 0x%lx\n", DiskRegInfo->RootDirEntries));
	KdPrint(("SectorsPerCluster = 0x%lx\n", DiskRegInfo->SectorsPerCluster));
	KdPrint(("DriveLetter       = %wZ\n", &(DiskRegInfo->DriveLetter)));

	return;
}


NTSTATUS
RamDiskFormatDisk(
	IN PDEVICE_EXTENSION devExt
)
/**
 * ��ʽ������ΪFAT12/16
 *
 * ����������أ�
 * �����Ƕ�д�Ļ�����λ��Ӳ����������ϣ�һ�����ٶ�дһ��������һ��Ϊ512�ֽڣ�
 * �����ϱ���Ϊ���ͬ��Բ��������ŵ���ÿ���ŵ�������Ϊͬ����Ŀ������
 * ���������ͬλ�õĴŵ������һ�����棬����Խ��ŵ�Խϸ
 * ��Щ����������ṹ
 *
 * ����ϵͳ�Դ��̹�����һ���߼��ϵĽṹ��ͨ���ļ�ϵͳ��ʵ��
 * ΢����ļ�ϵͳ������FAT12��FAT16��FAT32��NTFS��
 *
 * ��FAT12/16ϵͳ�У�
 * MBR����������¼��λ���������̵ĵ�һ����������СΪһ��������С
 * ��ʼ��Ϊһ�γ�����BIOS����ִ�е����ʱ���Ὣ��γ�����ص��ڴ���ִ��
 * �������ʱһ��Ӳ�̷�������¼������Ϣ
 * ����������ֻ����һ�����ô��̾�����Ҫ�����������Բ�����MBR����
 * DBR������ϵͳ������¼��������MBR�������м�¼����ʼλ��ָ��ĵ�һ������
 * ��������Ч���������򡢳��̱�־��������������
 * �ʼ��һ����תָ���תλ��Ϊ�������򴦣����̱�־λOEM����������������ΪBPB���ݿ飬��¼�˷�������Ϣ
 * ���ļ�ϵͳ���������κ�һ�����̾�ʱ��DBR��Ϣ������ȡ
 * FAT���ļ������λ��DBR֮��һʽ������ʽ�������棬��һ������ÿ�������Ŷ���һ����
 * ��0�͵�1��ʱ�����ģ��ӵ�2�ʼ������¼ĳ���ļ�����λ��
 * FAT�Ƕ���������壬����ÿ��������һ���ļ���������ʼ�����ȷ��
 * Ҫ�õ���Ŀ¼��ڵ㣬�����Ŀ¼��ڵ��γ�һ���������FAT��洢�����ÿһ������Ŀ¼�µ�һ���ļ���Ŀ¼����¼�˺ܶ���Ϣ
 */
{

	PBOOT_SECTOR bootSector = (PBOOT_SECTOR)devExt->DiskImage; //ָ���������������ָ��
	PUCHAR       firstFatSector;//ָ���һ��FAT��ָ��
	ULONG        rootDirEntries;//��¼��Ŀ¼��ڵ�����
	ULONG        sectorsPerCluster;//��¼ÿ�����ж�������
	USHORT       fatType;        //FAT�ļ�ϵͳ����
	USHORT       fatEntries;     //��¼FAT�����ж��ٱ���
	USHORT       fatSectorCnt;   //Ҳ��FAT����Ҫռ�ö�������
	PDIR_ENTRY   rootDir;        //ָ���һ����Ŀ¼��ڵ�

	PAGED_CODE();//ȷ�Ͽ��Դ�ȡ��ҳ�ڴ�
	ASSERT(sizeof(BOOT_SECTOR) == 512);//ȷ��������������СΪ512
	ASSERT(devExt->DiskImage != NULL);//ȷ�ϲ����Ĵ��̾������
	//��0�����̣������
	RtlZeroMemory(devExt->DiskImage, devExt->DiskRegInfo.DiskSize);
	//�����������
	devExt->DiskGeometry.BytesPerSector = 512;
	devExt->DiskGeometry.SectorsPerTrack = 32;     // Using Ramdisk value
	devExt->DiskGeometry.TracksPerCylinder = 2;    // Using Ramdisk value
	//����������
	devExt->DiskGeometry.Cylinders.QuadPart = devExt->DiskRegInfo.DiskSize / 512 / 32 / 2;
	//���̽�������
	devExt->DiskGeometry.MediaType = RAMDISK_MEDIA_TYPE;

	KdPrint((
		"Cylinders: %ld\n TracksPerCylinder: %ld\n SectorsPerTrack: %ld\n BytesPerSector: %ld\n",
		devExt->DiskGeometry.Cylinders.QuadPart, devExt->DiskGeometry.TracksPerCylinder,
		devExt->DiskGeometry.SectorsPerTrack, devExt->DiskGeometry.BytesPerSector
		));
	//�����û�ָ����ֵ�Ը�Ŀ¼�����Ŀ���г�ʼ��
	rootDirEntries = devExt->DiskRegInfo.RootDirEntries;
	//�����û�ָ����ֵ��ÿ�����ж��ٸ��������г�ʼ��
	sectorsPerCluster = devExt->DiskRegInfo.SectorsPerCluster;
	//��Ŀ¼��ڵ�ֻ����32���ֽڣ�������ռ��һ������
	if (rootDirEntries & (DIR_ENTRIES_PER_SECTOR - 1)) {
		//�û�ָ����Ŀ������ʱ������ʹ�����ռ�������
		rootDirEntries =
			(rootDirEntries + (DIR_ENTRIES_PER_SECTOR - 1)) &
			~(DIR_ENTRIES_PER_SECTOR - 1);
	}

	KdPrint((
		"Root dir entries: %ld\n Sectors/cluster: %ld\n",
		rootDirEntries, sectorsPerCluster
		));
	//�Կ�ʼ����תָ���Ա����Ӳ����ָ��
	bootSector->bsJump[0] = 0xeb;
	bootSector->bsJump[1] = 0x3c;
	bootSector->bsJump[2] = 0x90;
	//OEM��Ա����
	bootSector->bsOemName[0] = 'R';
	bootSector->bsOemName[1] = 'a';
	bootSector->bsOemName[2] = 'j';
	bootSector->bsOemName[3] = 'u';
	bootSector->bsOemName[4] = 'R';
	bootSector->bsOemName[5] = 'a';
	bootSector->bsOemName[6] = 'm';
	bootSector->bsOemName[7] = ' ';
	//ÿ���������ٸ��ֽڣ�ȡ�Գ�ʼ���Ĵ�����Ϣ���ݽṹ
	bootSector->bsBytesPerSec = (SHORT)devExt->DiskGeometry.BytesPerSector;
	bootSector->bsResSectors = 1;//ֻ��һ��������������DBR����
	bootSector->bsFATs = 1;//ֻ���һ��FAT��
	bootSector->bsRootDirEnts = (USHORT)rootDirEntries;//������ڵ㣬��֮ǰ�ļ���ó�
	//�������������ɴ��̴�С��ÿ��������С����õ�
	bootSector->bsSectors = (USHORT)(devExt->DiskRegInfo.DiskSize /
		devExt->DiskGeometry.BytesPerSector);
	bootSector->bsMedia = (UCHAR)devExt->DiskGeometry.MediaType;//���������ɳ�ʼ����Ϣ�õ�
	bootSector->bsSecPerClus = (UCHAR)sectorsPerCluster;//ÿ���ص�����������֮ǰ����õ�
	//FAT��ı�����Ŀʹ��������ȥ�������������ټ�ȥ��Ŀ¼��ڵ���ռ�õ���������֮�����ÿ�������������ս��+2�����ڵ�0�͵�1���
	fatEntries =
		(bootSector->bsSectors - bootSector->bsResSectors -
			bootSector->bsRootDirEnts / DIR_ENTRIES_PER_SECTOR) /
		bootSector->bsSecPerClus + 2;
	//������Ŀ�������ļ�ϵͳ����
	if (fatEntries > 4087) {
		fatType = 16;
		//����
		fatSectorCnt = (fatEntries * 2 + 511) / 512;
		fatEntries = fatEntries + fatSectorCnt;
		fatSectorCnt = (fatEntries * 2 + 511) / 512;
	}
	else {
		fatType = 12;
		fatSectorCnt = (((fatEntries * 3 + 1) / 2) + 511) / 512;
		fatEntries = fatEntries + fatSectorCnt;
		fatSectorCnt = (((fatEntries * 3 + 1) / 2) + 511) / 512;
	}
	//��ʼ��FAT����ռ������
	bootSector->bsFATsecs = fatSectorCnt;
	bootSector->bsSecPerTrack = (USHORT)devExt->DiskGeometry.SectorsPerTrack;//��ʼ��DBR��ÿ���ŵ�������
	bootSector->bsHeads = (USHORT)devExt->DiskGeometry.TracksPerCylinder;//��ʼ����ͷ��
	bootSector->bsBootSignature = 0x29;//��ʼ������ǩ��
	bootSector->bsVolumeID = 0x12345678;//���дһ����ID
	//���þ��
	bootSector->bsLabel[0] = 'R';
	bootSector->bsLabel[1] = 'a';
	bootSector->bsLabel[2] = 'm';
	bootSector->bsLabel[3] = 'D';
	bootSector->bsLabel[4] = 'i';
	bootSector->bsLabel[5] = 's';
	bootSector->bsLabel[6] = 'k';
	bootSector->bsLabel[7] = ' ';
	bootSector->bsLabel[8] = ' ';
	bootSector->bsLabel[9] = ' ';
	bootSector->bsLabel[10] = ' ';
	//����֮ǰ��������д�ļ�ϵͳ����
	bootSector->bsFileSystemType[0] = 'F';
	bootSector->bsFileSystemType[1] = 'A';
	bootSector->bsFileSystemType[2] = 'T';
	bootSector->bsFileSystemType[3] = '1';
	bootSector->bsFileSystemType[4] = '?';
	bootSector->bsFileSystemType[5] = ' ';
	bootSector->bsFileSystemType[6] = ' ';
	bootSector->bsFileSystemType[7] = ' ';
	bootSector->bsFileSystemType[4] = (fatType == 16) ? '6' : '2';
	//ǩ��DBR����־λ0x55AA
	bootSector->bsSig2[0] = 0x55;
	bootSector->bsSig2[1] = 0xAA;
	//��λ��FAT����ʼ�㣬��λ��ʽ������DBRֻ��һ��������һ����
	firstFatSector = (PUCHAR)(bootSector + 1);
	//��д���ʱ�ʶ
	firstFatSector[0] = (UCHAR)devExt->DiskGeometry.MediaType;
	firstFatSector[1] = 0xFF;
	firstFatSector[2] = 0xFF;
	//�����FAT16��ÿ������Ϊ4�ֽ�
	if (fatType == 16) {
		firstFatSector[3] = 0xFF;
	}
	//��ڵ����FAT���������׶�λ
	rootDir = (PDIR_ENTRY)(bootSector + 1 + fatSectorCnt);
	//������
	rootDir->deName[0] = 'M';
	rootDir->deName[1] = 'S';
	rootDir->deName[2] = '-';
	rootDir->deName[3] = 'R';
	rootDir->deName[4] = 'A';
	rootDir->deName[5] = 'M';
	rootDir->deName[6] = 'D';
	rootDir->deName[7] = 'R';

	rootDir->deExtension[0] = 'I';
	rootDir->deExtension[1] = 'V';
	rootDir->deExtension[2] = 'E';
	//��ڵ���������Ϊ�������
	rootDir->deAttributes = DIR_ATTR_VOLUME;
	//��ʽ�����
	return STATUS_SUCCESS;
}

NTSTATUS
RamDiskEvtDeviceAdd(
	IN WDFDRIVER Driver, //WDF��������
	IN PWDFDEVICE_INIT DeviceInit //WDF����ģ���Զ������һ�����ݽṹ��ר�Ŵ��ݸ�EvtDriverDeviceAdd���������������豸
)
{
	//�ûص������������ڼ��弴�ù������������豸ʱ������豸�����г�ʼ��������
	//�κ�֧��PnP������������Ӧ���������ĺ���������WDM������AddDevice�ص��ķ��棩
	//��DriverEntryִ����Ϻ�����������ֻ���������������ϵͳ������ϵ��

	//�ֽ⿴�±����д˺����Ĺ���

	//����һЩ����
	NTSTATUS status = STATUS_SUCCESS;
	WDF_OBJECT_ATTRIBUTES   deviceAttributes;//��Ҫ�������豸�������������
	WDFDEVICE               device;//��Ҫ�������豸
	WDF_OBJECT_ATTRIBUTES   queueAttributes;//����������������
	WDF_IO_QUEUE_CONFIG     ioQueueConfig;//�������ñ���
	PDEVICE_EXTENSION       pDeviceExtension;//�豸��չ��ָ��
	PQUEUE_EXTENSION        pQueueContext = NULL;//������չ��
	WDFQUEUE                queue;//��Ҫ�����Ķ���
	DECLARE_CONST_UNICODE_STRING(ntDeviceName, NT_DEVICE_NAME);//����һ��UNICODE_STRING��������ʼ��
	//��֤����������Բ���paged�ڴ�
	PAGED_CODE();	//#define PAGED_CODE() PAGED_ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
	UNREFERENCED_PARAMETER(Driver);//�Բ�ʹ�õĲ�������������������뾯��

	//�����豸����
	status = WdfDeviceInitAssignName(DeviceInit, &ntDeviceName);//Ϊ�豸ָ������
	if (!NT_SUCCESS(status)) {
		return status;
	}
	//���û�������
	WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_DISK);//�����豸����
	WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoDirect);//�ڽ���д��DeviceIoControl��IRP�����͵�����豸ʱ��IRP��Я���Ļ��������Ա�ֱ��ʹ��
	WdfDeviceInitSetExclusive(DeviceInit, FALSE);//��ʾ�豸���Զ�δ�
	//ָ���豸������չ
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_EXTENSION);
	//ָ���豸����ص�����
	deviceAttributes.EvtCleanupCallback = RamDiskEvtDeviceContextCleanup;
	//׼��������ɣ������豸���������豸ͨ�����������������ڱ�����
	status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);
	if (!NT_SUCCESS(status)) {
		return status;
	}
	//�����½����豸���豸��չָ��
	pDeviceExtension = DeviceGetExtension(device);

	//�������豸������
	//���õķ�ʽ�ǣ����Լ�ʵ�ֵĻص���������Ϊ�豸�Ĺ��ַܷ����������罫��д����ʵ��Ϊ��д�ڴ棬������򵥵��ڴ��̣�һ����Ҫ��������
	//WDF�����ֱ���ṩ�������Ĵ������
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
		&ioQueueConfig,
		WdfIoQueueDispatchSequential
	);//���������ñ�����ʼ��ΪĬ��ֵ
	//�����ĵ����������Ϊ�Զ��崦����������ʹ��Ĭ��ֵ
	ioQueueConfig.EvtIoDeviceControl = RamDiskEvtIoDeviceControl;
	ioQueueConfig.EvtIoRead = RamDiskEvtIoRead;
	ioQueueConfig.EvtIoWrite = RamDiskEvtIoWrite;
	//ָ�����ж�����չ
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&queueAttributes, QUEUE_EXTENSION);
	//׼�������������������ж��󣬽�֮ǰ�������豸��Ϊ���еĸ������豸����ʱ����Ҳ��������
	status = WdfIoQueueCreate(device,
		&ioQueueConfig,
		&queueAttributes,
		&queue);
	if (!NT_SUCCESS(status)) {
		return status;
	}
	//��������ɵĶ�����չ
	pQueueContext = QueueGetExtension(queue);
	//��ʼ��������չ�е�DeviceExtension��Ϊ�ս������豸����չ�����������������ɻ�ȡ���ж�Ӧ���豸���豸��չ
	pQueueContext->DeviceExtension = pDeviceExtension;

	//�û����ó�ʼ��
	//�豸�����������豸�Ķ��ж��������ˣ���������ʼ�����ڴ�����ص����ݽṹ
	//�������豸���豸��չ����Ӧ��UNICODE_STRING��ʼ��
	pDeviceExtension->DiskRegInfo.DriveLetter.Buffer =
		(PWSTR)&pDeviceExtension->DriveLetterBuffer;
	pDeviceExtension->DiskRegInfo.DriveLetter.MaximumLength =
		sizeof(pDeviceExtension->DriveLetterBuffer);
	//��ϵͳΪ�������ṩ��ע�����л�ȡ������Ҫ����Ϣ
	RamDiskQueryDiskRegParameters(
		WdfDriverGetRegistryPath(WdfDeviceGetDriver(device)), //��ȡ�������󲢻�ȡע���·��
		&pDeviceExtension->DiskRegInfo	//��ñ�������д��Ҫ��ֵ
	);
	//��ȡ�����󣬷���һ����С���ڴ�����Ϊģ����̣���С��ע����еĴ��̴�С����ָ�������ռ䱻��Ϊ���̾���
	pDeviceExtension->DiskImage = ExAllocatePoolWithTag(
		NonPagedPool, //�Ƿ�ҳ�ڴ棬��Զ���ڴ��У����ᱻ���������ϣ����Կɷ���Ľ���
		pDeviceExtension->DiskRegInfo.DiskSize,
		RAMDISK_TAG
	);
	//�����ڴ�ɹ�֮�󣬴��̾����˿ռ䣬����û�з�������ʽ���Ȳ���
	if (pDeviceExtension->DiskImage) {

		UNICODE_STRING deviceName;
		UNICODE_STRING win32Name;
		//���ڴ���ʵĴ��̸�ʽ��
		RamDiskFormatDisk(pDeviceExtension);
		//��������Ҫ�����̱�¶��Ӧ�ò��Թ�ʹ��
		status = STATUS_SUCCESS;
		//��ʼ��һЩ������ַ�����
		RtlInitUnicodeString(&win32Name, DOS_DEVICE_NAME);
		RtlInitUnicodeString(&deviceName, NT_DEVICE_NAME);
		//׼���������洢�����������ı���
		pDeviceExtension->SymbolicLink.Buffer = (PWSTR)
			&pDeviceExtension->DosDeviceNameBuffer;
		pDeviceExtension->SymbolicLink.MaximumLength =
			sizeof(pDeviceExtension->DosDeviceNameBuffer);
		pDeviceExtension->SymbolicLink.Length = win32Name.Length;
		//��������������ͷ����Ϊ��\\DosDevices\\�����������з������ӹ��е�ǰ׺
		RtlCopyUnicodeString(&pDeviceExtension->SymbolicLink, &win32Name);
		RtlAppendUnicodeStringToString(&pDeviceExtension->SymbolicLink,
			&pDeviceExtension->DiskRegInfo.DriveLetter);//ƴ�Ӵ��û������ж�������ָ���̷�
		//����WDFģ���ṩ�ĺ�����Ϊ֮ǰ���ɵ��豸������������
		status = WdfDeviceCreateSymbolicLink(device,
			&pDeviceExtension->SymbolicLink);
	}
	//���ˣ������豸�Ѿ������������ӵ���Ӧ�ò�
	return status;
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

/**
 * �����װ��
 * ��Ҫ���ɲ���sys�ļ��Լ�inf�ļ�������WDK��װĿ¼/redist/wdf�����WdfCoInstaller01007.dll
 * ����������ҵ����Ӳ�����ֶ��Ӵ��̰�װ��ѡ��inf�ļ�������������һ����
 *
 * ��װ�򵼶�ȡinf�ļ���sys�ļ��ᱻ������System32/driversĿ¼��
 * ���һ���ע������½�һЩ��
 * ��1����\HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Enum\Root\UNKNOWN����
 * �����ڰ�װ֮�������ӵ�һ����������֮�»���һ������0000���Ӽ���
 * �����ڹ���Ա�û�����regedit����鿴����������ݡ�
 * ��Windowsϵͳ����ʱ��PnP��������ö�١�\HKEY_ LOCAL_MACHINE\SYSTEM\CurrentControlSet\ Enum\Root\���µ����м���
 * ������ݼ��е���ϢΪÿ���豸����һ��PDO�������豸���󣩡�
 * �ڡ�\HKEY_LOCAL_MACHINE\SYSTEM\ CurrentControlSet\Enum\ Root\���µ����м���������ϵͳ��װʱ��¼�Ļ���ͨ�����Ӳ������ӵ�һ��Ӳ����Ϣ��
 * Windows������һ�������߲��Ұ���ЩӲ���ҽ������档PnP���������PDO�Ľ���֮�����б�׼��PnP������
 * ��PDO���Ӧ���������ᱻ���ء��ڡ�\HKEY_LOCAL_ MACHINE\SYSTEM\CurrentControlSet\Enum\Root\ UNKNOWN\0000�����¿��Կ�����Ramdisk��ص���Ϣ��
 * ������һ��ֵ��Service��ϵͳ����Ѱ�����ֵָ�������������м��غ͵��á����⣬����������滹��һ��ֵ��ClassGUID��
 * ���ֵ�����ݽ���Ϊ��һ��������������
 * ��2����\HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Ramdisk����
 * ϵͳ��ȥ����Service��ָ����������
 * ���ڡ�\HKEY_LOCAL_MACHINE\SYSTEM\ CurrentControlSet\Enum\Root\UNKNOWN\0000���е�Serviceֵ����������Ramdisk��
 * ����ϵͳ�ͻ��ڡ�\HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\������Ѱ���������Ramdisk�ķ���
 * ��������������ȥ���ռ�������������
 * ��\HKEY_LOCAL_MACHINE\ SYSTEM\CurrentControlSet\Services\Ramdisk����һ����׼�ķ�����������
 * ���������Ҫ��һ�����ָ����ramdisk.sys�ļ���λ�á�
 * ��3����\HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\ClassGUID����
 * ע�����������ClassGUIDʵ������һ�������֣����ֵ������ڡ�\HKEY_LOCAL_ MACHINE\SYSTEM\CurrentControlSet\Enum\Root\UNKNOWN\0000������ClassGUIDֵ�С�
 * �����˵���˾���ͬ��Class��������ͨ�õ�һЩ��Ϣ����Ramdisk�����������û��̫����ô���
 * ��������������������������������������У������ȴ���ŷǳ���Ҫ�����á�
 */