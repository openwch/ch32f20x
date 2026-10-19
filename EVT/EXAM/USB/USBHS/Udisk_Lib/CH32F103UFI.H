/* 2014.09.09
*****************************************
**   Copyright  (C)  W.ch  1999-2019   **
**   Web:      http://wch.cn           **
*****************************************
**  USB-flash File Interface for CH103 **
**  KEIL423, ARM Compiler 4.1          **
*****************************************
*/
/* CH103 UHost file system interface for USB drives; supports FAT12/FAT16/FAT32 */

//#include "ch32f10x.h"

#ifndef __CH103UFI_H__
#define __CH103UFI_H__

#define CH103_LIB_VER		0x11

//#define DISK_BASE_BUF_LEN		512	/* Default disk data buffer size is 512 bytes (2048 or even 4096 may be selected to support USB drives with large sectors); 0 disables the buffer definition in this file and lets the application specify it in pDISK_BASE_BUF */
/* To share the disk data buffer and save RAM, define DISK_BASE_BUF_LEN as 0 to prevent the buffer from being defined in this file, and have the application store the start address of the shared buffer into pDISK_BASE_BUF before calling CH103LibInit */

//#define NO_DEFAULT_ACCESS_SECTOR	1		/* Disable the default disk sector read/write routine; a user-written routine replaces it below */
//#define NO_DEFAULT_DISK_CONNECT		1		/* Disable the default disk connection check routine; a user-written routine replaces it below */
//#define NO_DEFAULT_FILE_ENUMER		1		/* Disable the default file name enumeration callback; a user-written routine replaces it below */

#ifdef __cplusplus
extern "C" {
#endif

/* ********************************************************************************************************************* */
/* FILE: CH32F103UF.H */
#include "stdio.h"
#include "string.h"
#include "usb_host_config.h"
#include "debug.h"

/* Error code */
#ifndef ERR_SUCCESS
#define ERR_SUCCESS				0x00	/* Operation succeeded */
#endif
#ifndef ERR_DISK_DISCON
#define ERR_CH103_ERROR			0x81	/* CH103Hardware error; CH103 may need to be reset */
//#define ERR_DISK_DISCON			0x82	/* The disk is not connected; it may have been disconnected */
#define ERR_STATUS_ERR			0x83	/* Disk status error; the disk may be connecting or disconnecting */
#define ERR_HUB_PORT_FREE		0x84	/* USB-HUBConnected, but no disk is attached to the HUB port; the disk may have been disconnected */
#define ERR_MBR_ERROR			0x91	/* The master boot record of the disk is invalid; the disk may not be partitioned or not formatted */
#define ERR_TYPE_ERROR			0x92	/* The disk partition type is not supported; only FAT12/FAT16/BigDOS/FAT32 are supported, so the disk must be repartitioned with a disk management tool */
#define ERR_BPB_ERROR			0xA1	/* The disk is not formatted, or its parameters are invalid; it must be reformatted by WINDOWS with the default parameters */
#define ERR_TOO_LARGE			0xA2	/* The disk was not formatted in the standard way and has a capacity above 4 GB, or its capacity is above 250 GB; it must be reformatted by WINDOWS with the default parameters */
#define ERR_FAT_ERROR			0xA3	/* The file system of the disk is not supported; only FAT12/FAT16/FAT32 are supported, so the disk must be reformatted by WINDOWS with the default parameters */
#define ERR_DISK_FULL			0xB1	/* The disk is too full; little or no free space remains, so the disk needs defragmenting */
#define ERR_FDT_OVER			0xB2	/* Too many files in the directory, leaving no free directory entry; the root directory of FAT12/FAT16 should hold fewer than 500 files, so the disk needs defragmenting */
#define ERR_MISS_DIR			0xB3	/* A subdirectory of the specified path was not found; the directory name may be wrong */
#define ERR_FILE_CLOSE			0xB4	/* The file has been closed; to use it, open the file again */
#define ERR_OPEN_DIR			0x41	/* The directory at the specified path has been opened */
#define ERR_MISS_FILE			0x42	/* The file at the specified path was not found; the file name may be wrong */
#define ERR_FOUND_NAME			0x43	/* A file name matching the wildcard was found; the file name and its full path are in the command buffer. To use it, open that file */
#endif
/* Codes 2XH-3XH are the communication failure codes for USB host mode, emulating the CH375 return values in the CH103 routines */
/* Codes 1XH are the operation status codes for USB host mode, emulating the CH375 return values in the CH103 routines */
#ifndef ERR_USB_CONNECT
#define	ERR_USB_CONNECT_LS		0x13	/* A low-speed USB device connect event was detected */
#define	ERR_USB_CONNECT			0x15	/* A USB device connect event was detected; the disk is connected */
#define	ERR_USB_DISCON			0x16	/* A USB device disconnect event was detected; the disk is disconnected */
#define	ERR_USB_BUF_OVER		0x17	/* USBThe transferred data is invalid, or there is too much data and the buffer overflows */
#define	ERR_USB_DISK_ERR		0x1F	/* USBStorage operation failed. During initialization the USB storage may be unsupported; during read/write the disk may be damaged or already disconnected */
#define	ERR_USB_TRANSFER		0x20	/* NAK/STALLand other error codes are in the range 0x20~0x2F */
#endif

/* Disk and file status */
#define DISK_UNKNOWN			0x00	/* Not initialized yet; unknown status */
#define DISK_DISCONNECT			0x01	/* The disk is not connected or has been disconnected */
#define DISK_CONNECT			0x02	/* The disk is connected but has not been initialized, or the disk is not recognized */
#define DISK_USB_ADDR			0x04	/* A USB device address has been assigned to the disk, but USB configuration and disk initialization have not been done yet */
#define DISK_MOUNTED			0x05	/* The disk was initialized successfully, but its file system has not been analyzed yet or is not supported */
#define DISK_READY				0x10	/* The file system of the disk has been analyzed and is supported */
#define DISK_OPEN_ROOT			0x12	/* The root directory has been opened in sector mode; its contents can only be read or written in whole sectors. It must be closed after use. Note that the root directory of FAT12/FAT16 has a fixed length */
#define DISK_OPEN_DIR			0x13	/* A subdirectory has been opened in sector mode; its contents can only be read or written in whole sectors */
#define DISK_OPEN_FILE			0x14	/* A file has been opened in sector mode; data can be read and written in whole sectors */
#define DISK_OPEN_FILE_B		0x15	/* A file has been opened in byte mode; data can be read and written in bytes */

/* FATType flag */
#ifndef DISK_FAT16
#define DISK_FS_UNKNOWN			0		/* Unknown file system */
#define DISK_FAT12				1		/* FAT12File system */
#define DISK_FAT16				2		/* FAT16File system */
#define DISK_FAT32				3		/* FAT32File system */
#endif

/* FATFile directory information in the data area */
typedef struct _FAT_DIR_INFO {
	uint8_t	DIR_Name[11];				/* 00H,File name, 11 bytes in total, padded with spaces */
	uint8_t	DIR_Attr;					/* 0BH,File attribute; see the description below */
	uint8_t	DIR_NTRes;					/* 0CH */
	uint8_t	DIR_CrtTimeTenth;			/* 0DH,File creation time, counted in units of 0.1 second */
	uint16_t	DIR_CrtTime;				/* 0EH,File creation time */
	uint16_t	DIR_CrtDate;				/* 10H,File creation date */
	uint16_t	DIR_LstAccDate;				/* 12H,Date of the most recent access */
	uint16_t	DIR_FstClusHI;				/* 14H */
	uint16_t	DIR_WrtTime;				/* 16H,File modification time; see the macro MAKE_FILE_TIME */
	uint16_t	DIR_WrtDate;				/* 18H,File modification date; see the macro MAKE_FILE_DATA */
	uint16_t	DIR_FstClusLO;				/* 1AH */
	uint32_t	DIR_FileSize;				/* 1CH,File length */
} FAT_DIR_INFO;							/* 20H */
						/* 20H */

typedef FAT_DIR_INFO* PX_FAT_DIR_INFO;

/* File attribute */
#define ATTR_READ_ONLY			0x01	/* The file has the read-only attribute */
#define ATTR_HIDDEN				0x02	/* The file has the hidden attribute */
#define ATTR_SYSTEM				0x04	/* The file has the system attribute */
#define ATTR_VOLUME_ID			0x08	/* Volume label */
#define ATTR_DIRECTORY			0x10	/* Subdirectory */
#define ATTR_ARCHIVE			0x20	/* The file has the archive attribute */
#define ATTR_LONG_NAME			( ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID )
/* File attribute (uint8_t) */
/* bit0 bit1 bit2 bit3 bit4 bit5 bit6 bit7 */
/*  Read-only   Hidden   System   Volume   Directory   Archive   Undefined   */
/*  Read   Hidden   System   Volume   Directory   Archive            */
/* File time (uint16_t) */
/* Time = (Hour<<11) + (Minute<<5) + (Second>>1) */
#define MAKE_FILE_TIME( h, m, s )	( (h<<11) + (m<<5) + (s>>1) )	/* Generate file time data for the given hour, minute and second */
/* File date (uint16_t) */
/* Date = ((Year-1980)<<9) + (Month<<5) + Day */
#define MAKE_FILE_DATE( y, m, d )	( ((y-1980)<<9) + (m<<5) + d )	/* Generate file date data for the given year, month and day */

/* File name */
#define PATH_WILDCARD_CHAR		0x2A	/* Wildcard '*' in path names */
#define PATH_SEPAR_CHAR1		0x5C	/* Path separator '\' */
#define PATH_SEPAR_CHAR2		0x2F	/* Path separator '/' */
#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN			64		/* Maximum path length, including all slash separators, the dot separator and the path terminator 00H */
#endif

/* External command parameter */
typedef union _CMD_PARAM
{
    struct
    {
        uint8_t mBuffer[ MAX_PATH_LEN ];
    } Other;
    struct
    {
        uint32_t mTotalSector;          /* Return: total number of sectors of the current logical drive */
        uint32_t mFreeSector;           /* Return: number of free sectors of the current logical drive */
        uint32_t mSaveValue;
    } Query;                            /* CMD_DiskQuery, Query disk information */
    struct
    {
        uint8_t mPathName[ MAX_PATH_LEN ];  /* Input: path: [drive letter, colon, slash, directory name or file name and extension..., terminator 00H]; the drive letter and colon may be omitted, for example "C:\DIR1.EXT\DIR2\FILENAME.EXT",00H */
    } Open;                             /* CMD_FileOpen, Open file */
//  struct
//  {
//      uint8_t mPathName[ MAX_PATH_LEN ];  /* Input: path: [drive letter, colon, slash, directory name or file name and extension (wildcards * allowed)..., terminator 00H]; the drive letter and colon may be omitted, for example "C:\DIR1.EXT\DIR2\FILE*",00H */
//  } Open;                             /* CMD_FileOpen, Enumerate files. If the highest bit of CHRV3vFileSize is 1, xFileNameEnumer is called for each file; if it is 0, the file name with the given index is returned */
    struct
    {
        uint8_t mUpdateLen;             /* Input: whether the length may be updated: 0 = disallow, 1 = allow */
    } Close;                            /* CMD_FileClose, Close the current file */
    struct
    {
        uint8_t mPathName[ MAX_PATH_LEN ];  /* Input: path: [drive letter, colon, slash, directory name or file name and extension..., terminator 00H]; the drive letter and colon may be omitted, for example "C:\DIR1.EXT\DIR2\FILENAME.EXT",00H */
    } Create;                           /* CMD_FileCreate, Create and open a new file; if the file already exists, delete it first */
    struct
    {
        uint8_t mPathName[ MAX_PATH_LEN ];  /* Input: path: [drive letter, colon, slash, directory name or file name and extension..., terminator 00H]; the drive letter and colon may be omitted, for example "C:\DIR1.EXT\DIR2\FILENAME.EXT",00H */
    } Erase;                            /* CMD_FileErase, Delete the file and close it */
    struct
    {
        uint32_t mFileSize;             /* Input: new file length; 0FFFFFFFFH means no change. Return: the original length */
        uint16_t mFileDate;             /* Input: new file date; 0FFFFH means no change. Return: the original date */
        uint16_t mFileTime;             /* Input: new file time; 0FFFFH means no change. Return: the original time */
        uint8_t  mFileAttr;             /* Input: new file attribute; 0FFH means no change. Return: the original attribute */
    } Modify;                           /* CMD_FileQuery, Query the information of the current file; CMD_FileModify queries or modifies the information of the current file */
    struct
    {
        uint32_t mSaveCurrClus;
        uint32_t mSaveLastClus;
    } Alloc;                            /* CMD_FileAlloc, Adjust the disk space allocated to the file according to its length */
    struct
    {
        uint32_t mSectorOffset;      /* Input: sector offset; 0 moves to the start of the file and 0FFFFFFFFH moves to the end of the file. Return: absolute linear sector number corresponding to the current file pointer; 0FFFFFFFFH means the end of the file has been reached */
        uint32_t mLastOffset;
    } Locate;                           /* CMD_FileLocate, Move the current file pointer */
    struct
    {
        uint8_t mSectorCount;           /* Input: number of sectors to read. Return: number of sectors actually read */
        uint8_t mActCnt;
        uint8_t mLbaCount;
        uint8_t mRemainCnt;
        uint8_t *mDataBuffer;           /* Input: start address of the buffer; return: current address of the buffer */
        uint32_t mLbaStart;
    } Read;                             /* CMD_FileRead, Read data from the current file */
    struct
    {
        uint8_t mSectorCount;           /* Input: number of sectors to write. Return: number of sectors actually written */
        uint8_t mActCnt;
        uint8_t mLbaCount;
        uint8_t mAllocCnt;
        uint8_t *mDataBuffer;           /* Input: start address of the buffer; return: current address of the buffer */
        uint32_t mLbaStart;
        uint32_t mSaveValue;
    } Write;                            /* CMD_FileWrite, Write data to the current file */
    struct
    {
        uint32_t mDiskSizeSec;          /* Return: total number of sectors of the whole physical disk; returned only on the first call */
    } DiskReady;                        /* CMD_DiskReady, Query disk readiness */
    struct
    {
        uint32_t mByteOffset;           /* Input: byte offset and the file pointer in bytes. Return: absolute linear sector number corresponding to the current file pointer; 0FFFFFFFFH means the end of the file has been reached */
        uint32_t mLastOffset;
    } ByteLocate;                       /* CMD_ByteLocate, Move the current file pointer in bytes */
    struct
    {
        uint16_t mByteCount;            /* Input: number of bytes to read. Return: number of bytes actually read */
        uint8_t *mByteBuffer;           /* Input: pointer to the buffer that holds the read data block */
        uint16_t mActCnt;
    } ByteRead;                         /* CMD_ByteRead, Read a data block from the current file in bytes */
    struct
    {
        uint16_t mByteCount;            /* Input: number of bytes to write. Return: number of bytes actually written */
        uint8_t *mByteBuffer;           /* Input: pointer to the buffer that holds the read data block */
        uint16_t mActCnt;
    } ByteWrite;                        /* CMD_ByteWrite, Write a data block to the current file in bytes */
    struct
    {
        uint8_t mSaveVariable;          /* Input: 0 restores the variables for a single USB drive, 0x80 restores them for multiple USB drives, and any other value backs up or saves the variables */
        uint8_t mReserved[3];
        uint8_t *mBuffer;               /* Input: pointer to the backup buffer for the library variables; its length is at least 80 bytes */
    } SaveVariable;                     /* CMD_SaveVariable, Back up, save or restore the library variables */
} CMD_PARAM;

typedef CMD_PARAM CMD_PARAM_I;
//typedef CMD_PARAM *P_CMD_PARAM;

/* SCSICommand code */
#ifndef SPC_CMD_INQUIRY
#define SPC_CMD_INQUIRY			0x12
#define SPC_CMD_READ_CAPACITY	0x25
#define SPC_CMD_READ10			0x28
#define SPC_CMD_WRITE10			0x2A
#define SPC_CMD_TEST_READY		0x00
#define SPC_CMD_REQUEST_SENSE	0x03
#define SPC_CMD_MODESENSE6		0x1A
#define SPC_CMD_MODESENSE10		0x5A
#define SPC_CMD_START_STOP		0x1B
#endif

/* FILE: CH32F103UFI.C */

#define EN_DISK_WRITE			1

#ifndef DISK_BASE_BUF_LEN
#define DISK_BASE_BUF_LEN		512		/* Default disk data buffer size is 512 bytes; 2048 or even 4096 is recommended to support USB drives with large sectors. 0 disables the buffer definition in the .H file and lets the application specify it in pDISK_BASE_BUF */
#endif

/* Variables provided by the library */
extern	volatile uint8_t CH103IntStatus;	/* CH103Interrupt status of the operation */
extern	volatile uint8_t CH103DiskStatus;   /* Disk and file status */
extern	uint8_t	 CH103vDiskFat;				/* FAT flag of the logical drive: 1 = FAT12, 2 = FAT16, 3 = FAT32 */
extern	uint8_t	 CH103vSecPerClus;			/* Number of sectors per cluster of the logical drive */
extern	uint8_t	 CH103vSectorSizeB;			/* log2(CH103vSectorSize) */
extern	uint32_t CH103vStartLba;			/* Start absolute sector number (LBA) of the logical drive */
extern	uint32_t CH103vDiskRoot;			/* For a FAT16 disk this is the number of sectors occupied by the root directory; for a FAT32 disk it is the start cluster number of the root directory */
extern	uint32_t CH103vDataStart;			/* Start LBA of the data area of the logical drive */
extern	uint32_t CH103vStartCluster;		/* Start cluster number of the current file or directory */
extern	uint32_t CH103vFileSize;			/* Length of the current file */
extern	uint32_t CH103vCurrentOffset;		/* Current file pointer: the byte offset of the current read/write position */
extern	uint32_t CH103vFdtLba;				/* LBA address of the current FDT */
extern	uint32_t CH103vLbaCurrent;			/* Start LBA address of the disk currently being read or written */
extern	uint16_t CH103vFdtOffset;			/* Offset of the current FDT within the sector */
extern	uint16_t CH103vSectorSize;			/* Sector size of the disk */
extern	uint8_t	 CH103vCurrentLun;			/* Logical unit number currently operated on the disk */
extern	uint8_t	 CH103vSubClassIs6;			/* USBThe subclass of the mass-storage device is 6; 0 means it is not 6 */
extern	uint8_t  *pDISK_BASE_BUF;	        /* Points to the disk data buffer in external RAM; its length is at least CH103vSectorSize and it is initialized by the application */
extern	uint8_t  *pDISK_FAT_BUF;		    /* Points to the disk FAT data buffer in external RAM; its length is at least CH103vSectorSize and it is initialized by the application */
extern	uint16_t CH103vPacketSize;			/* USBMaximum packet size of the mass-storage device: 64 at FS, 512 at HS/SS; initialized by the application */
extern	uint32_t *pTX_DMA_A_REG;			/* Points to the transmit DMA address register; initialized by the application */
extern	uint32_t *pRX_DMA_A_REG;			/* Points to the receive DMA address register; initialized by the application */
extern	uint16_t *pTX_LEN_REG;				/* Points to the transmit length register; initialized by the application */
extern	uint16_t *pRX_LEN_REG;				/* Points to the receive length register; initialized by the application */
extern	CMD_PARAM_I	mCmdParam;				/* Command parameters */

extern uint8_t *pHOST_RX_RAM_Addr;
extern uint8_t *pHOST_TX_RAM_Addr;

#if DEF_USB_PORT_FS_EN
#define	RxBuffer	USBFS_RX_Buf
#define	TxBuffer	USBFS_TX_Buf
#elif DEF_USB_PORT_HS_EN
#define	RxBuffer	USBHS_RX_Buf
#define	TxBuffer	USBHS_TX_Buf
#endif

#ifndef	pSetupReq
#define	pSetupReq	((PUSB_SETUP_REQ)TxBuffer)
#endif
#ifndef	pCBW
#define	pCBW		((PXUDISK_BOC_CBW)TxBuffer)
#define	pCSW		((PXUDISK_BOC_CSW)RxBuffer)
#endif
#ifndef	pBOC_buf
#define	pBOC_buf	(TxBuffer+((USB_BO_CBW_SIZE+4)&0xFE))
#endif

#if		DISK_BASE_BUF_LEN > 0
extern	uint8_t	DISK_BASE_BUF[ DISK_BASE_BUF_LEN ];	/* Disk data buffer in external RAM; its length is one sector */
#endif
extern	uint8_t	CH103ReadSector(uint8_t SectCount, uint8_t *DataBuf);	/* Read several sectors from the disk into the buffer */
#ifdef	EN_DISK_WRITE
extern	uint8_t	CH103WriteSector(uint8_t SectCount, uint8_t *DataBuf);	/* Write the multi-sector data block in the buffer to the disk */
#endif

extern	uint8_t	CH103DiskConnect(void);	/* Check whether the disk is connected and update the disk status */
extern	void	xFileNameEnumer(void);	/* Calls the externally defined routine: the file name enumeration callback */
extern	uint8_t	CH103LibInit(void);		/* Initialize the CH103 library; returns 0 on success */

/* Routines provided by the library */
/* In the routines below, both the file operation routines CH103File* and the disk query routine CH103DiskQuery may use the disk data buffer pDISK_BASE_BUF,
   Disk information may also be kept in pDISK_BASE_BUF, so that buffer must not be used for anything else,
   If RAM is scarce and pDISK_BASE_BUF is used temporarily for something else, CH103DirtyBuffer must be called afterwards to clear the disk buffer */
extern	uint8_t	CH103GetVer(void);		/* Get the version number of the current library */
extern	void	CH103DirtyBuffer(void);	/* Clear the disk buffer */
extern	uint8_t	CH103BulkOnlyCmd(uint8_t *DataBuf);	/* Execute a command based on the Bulk-Only protocol */
extern	uint8_t	CH103DiskReady(void);		/* Query whether the disk is ready */
extern	uint8_t	CH103AnalyzeError(uint8_t iMode);	/* USBOperation failed; analyze CH103IntStatus to obtain the error status */
extern	uint8_t	CH103FileOpen(void);		/* Open a file or enumerate files */
extern	uint8_t	CH103FileClose(void);		/* Close the current file */
#ifdef	EN_DISK_WRITE
extern	uint8_t	CH103FileErase(void);		/* Delete the file and close it */
extern	uint8_t	CH103FileCreate(void);	/* Create and open a new file; if the file already exists, delete it first */
extern	uint8_t	CH103FileAlloc(void);		/* Adjust the disk space allocated to the file according to its length */
#endif
extern	uint8_t	CH103FileModify(void);	/* Query or modify the information of the current file */
extern	uint8_t	CH103FileQuery(void);		/* Query the information of the current file */
extern	uint8_t	CH103FileLocate(void);	/* Move the current file pointer */
extern	uint8_t	CH103FileRead(void);		/* Read data from the current file into the given buffer */
#ifdef	EN_DISK_WRITE
extern	uint8_t	CH103FileWrite(void);		/* Write the data of the given buffer to the current file */
#endif
extern	uint8_t	CH103ByteLocate(void);	/* Move the current file pointer in bytes */
extern	uint8_t	CH103ByteRead(void);		/* Read a data block from the current position in bytes */
#ifdef	EN_DISK_WRITE
extern	uint8_t	CH103ByteWrite(void);		/* Write a data block to the current position in bytes */
#endif
extern	uint8_t	CH103DiskQuery(void);		/* Query disk information */
extern	void	CH103SaveVariable(void);	/* Back up, save or restore the library variables; used to switch the library between several chips or USB drives */

//extern void Delay_Us( uint32_t n );		// Delay in microseconds
//extern void Delay_Ms( uint16_t n );		// Delay in milliseconds
extern uint8_t USBHostTransact(uint8_t endp_pid, uint8_t tog, uint32_t timeout);	// CH103Transfer transaction: input is the destination endpoint address/PID token, the toggle flag and the NAK retry time; returns 0 on success and retries on timeout or error
extern uint8_t HostCtrlTransfer(uint8_t *DataBuf, uint8_t *RetLen);	// Execute a control transfer. The 8-byte request is in pSetupReq, DataBuf is the optional data buffer, and the actual transfer length is returned in the variable pointed to by ReqLen
extern void	CopySetupReqPkg(const uint8_t *pReqPkt);      // Copy the request packet of the control transfer
//extern uint8_t CtrlGetDeviceDescr( uint8_t *DataBuf );  // Get the device descriptor
extern uint8_t CtrlGetConfigDescr(uint8_t *DataBuf);      // Get the configuration descriptor
//extern uint8_t CtrlSetUsbAddress( uint8_t addr );       // Set the USB device address
extern uint8_t CtrlSetUsbConfig(uint8_t cfg);             // Set the USB device configuration
extern uint8_t CtrlClearEndpStall(uint8_t endp);          // Clear endpoint STALL
#ifndef FOR_ROOT_UDISK_ONLY
//extern uint8_t	CtrlGetHubDescr( uint8_t * DataBuf );  // Get the HUB descriptor
//extern uint8_t	HubGetPortStatus( uint8_t HubPortIndex, uint8_t * DataBuf );  // Query the HUB port status
//extern uint8_t	HubSetPortFeature( uint8_t HubPortIndex, uint8_t FeatureSelt );  // Set HUB port feature
//extern uint8_t	HubClearPortFeature( uint8_t HubPortIndex, uint8_t FeatureSelt );  // Clear HUB port feature
#endif

#ifdef __cplusplus
}
#endif

#endif
