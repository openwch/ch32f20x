/********************************** (C) COPYRIGHT *******************************
* File Name          : Udisk_Func_BasicOp.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2022/11/22
* Description        : USB full-speed port host operation functions.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/

/*******************************************************************************/
/* Header File */
#include "UDisk_Operation.h"\

/*******************************************************************************/
/* Variable Definition */
__attribute__((aligned(4)))  uint8_t  MY_DATA_BUF[ DISK_BASE_BUF_LEN ];   /* MY_DATA_BUFPoints to the disk data buffer in external RAM; its length is at least one sector, used to cache user data */
uint8_t  *pCodeStr;

/*********************************************************************
 * @fn      UDisk_USBH_ByteOperation
 *
 * @brief   Demo Function For UDisk File Byte-operation
 *          including Create\Modify\Read\Erase (EXAM1)
 *
 * @return  none
 */
void UDisk_USBH_ByteOperation( void )
{
    uint8_t  c, s, i;
    uint16_t TotalCount = 0;

   UDisk_USBH_DiskReady( );
    if( ( CH103DiskStatus >= DISK_MOUNTED )&&( UDisk_Opeation_Flag == 1 ) )
    {
        UDisk_Opeation_Flag = 0;
        printf("CH103DiskStatus:%02x\r\n", CH103DiskStatus);
        /* Read file */
        strcpy((char *)mCmdParam.Open.mPathName, "/NEWFILE.C");         //Set the path and file name to operate on: /NEWFILE.C
        s = CH103FileOpen();                                               //Open file
        if(s == ERR_MISS_DIR || s == ERR_MISS_FILE)                        //File not found
        {
            //File creation demonstration
            printf("Find No File And Create\r\n");
            strcpy((char *)mCmdParam.Create.mPathName, "/NEWFILE.C");       //New file name, in the root directory; a Chinese file name
            s = CH103FileCreate();                                         //Create and open a new file; if the file already exists, delete it first
            mStopIfError(s);
            printf("ByteWrite\r\n");
            //In practice the write length should be checked against the defined buffer length; if it is larger, the data must be written in several passes
            i = sprintf((char *)Com_Buffer, "Note: \xd\xaThis program reads and writes the USB drive file byte by byte, as a simple demonstration.\xd\xa"); //Demonstrate
            for(c = 0; c < 10; c++)
            {
                mCmdParam.ByteWrite.mByteCount = i;                          //Number of bytes to write this time
                mCmdParam.ByteWrite.mByteBuffer = Com_Buffer;                       //Pointer to the buffer
                s = CH103ByteWrite();                                        //Write data to the file in bytes
                mStopIfError(s);
                printf("WriteIn Success %02X times\r\n", (uint16_t)c);
            }
            //Demonstrate modifying a file attribute
            printf("Modify\r\n");
            mCmdParam.Modify.mFileAttr = 0xff;                               //Input: new file attribute; 0FFH means no change
            mCmdParam.Modify.mFileTime = 0xffff;                             //Input: new file time; 0FFFFH means no change, and the default time of the newly created file is used
            mCmdParam.Modify.mFileDate = MAKE_FILE_DATE(2015, 5, 18);        //Input: new file date: 2015.05.18
            mCmdParam.Modify.mFileSize = 0xffffffff;                         // Input: new file length in bytes; the length is updated automatically when the library closes the file, so it is not modified here
            i = CH103FileModify();                                           //Modify the information of the current file; modify the date
            mStopIfError(i);
            printf("Close\r\n");
            mCmdParam.Close.mUpdateLen = 1;                                  //File length is calculated automatically. Data is written in bytes. It is recommended to let the library close the file so that the file length is updated automatically
            i = CH103FileClose();
            mStopIfError(i);

            /* Delete a file named OLD.C, which must be placed in the root directory of the USB drive beforehand */
            printf("Erase\r\n");
            strcpy((char *)mCmdParam.Create.mPathName, "/OLD.C");            //Name of the file to delete, in the root directory
            i = CH103FileErase();                                            //Delete the file and close it
            if(i != ERR_SUCCESS)
            {
                printf("Error: %02X\r\n", (uint16_t)i);                      //Display an error message; the file may not have been found
                if(i == ERR_MISS_FILE)
                {
                    printf("Didn't file the file\r\n");
                }
            }
            else
            {
                printf("Erase Over\r\n");
            }
        }
        else
        {
            //To append new data to the end of the original file, move the file pointer
            //Write to file
            printf("ByteWrite\r\n");
            mCmdParam.ByteLocate.mByteOffset = 0xffffffff;                   //Move to the end of the file
            CH103ByteLocate();
            //In practice the write length should be checked against the defined buffer length; if it is larger, the data must be written in several passes
            i = sprintf((char *)Com_Buffer, "Note: \xd\xaThis program reads and writes the USB drive file byte by byte, as a simple demonstration.\xd\xa");
            for(c = 0; c < 10; c++)
            {
                mCmdParam.ByteWrite.mByteCount = i;                          //Number of bytes to write this time
                mCmdParam.ByteWrite.mByteBuffer = Com_Buffer;                       //Pointer to the buffer
                s = CH103ByteWrite();                                        //Write data to the file in bytes
                mStopIfError(s);
                printf("WriteIn Success %02X times\r\n", (uint16_t)c);
            }

						//Demonstrate modifying a file attribute
            printf("Modify\r\n");
            mCmdParam.Modify.mFileAttr = 0xff;                               //Input: new file attribute; 0FFH means no change
            mCmdParam.Modify.mFileTime = 0xffff;                             //Input: new file time; 0FFFFH means no change, and the default time of the newly created file is used
            mCmdParam.Modify.mFileDate = MAKE_FILE_DATE(2015, 5, 18);        //Input: new file date: 2015.05.18
            mCmdParam.Modify.mFileSize = 0xffffffff;                         // Input: new file length in bytes; the length is updated automatically when the library closes the file, so it is not modified here
            i = CH103FileModify();                                           //Modify the information of the current file; modify the date
            mStopIfError(i);
																		
            //Read the first N bytes of a file
            TotalCount = 60;                                                 //Set the total length to read to 60 bytes
            printf("First %d characters read out:\r\n", TotalCount);
            while(TotalCount)                                                //If the file is too large to read in one call, call CH103ByteRead again to continue; the file pointer advances automatically
            {
                if(TotalCount > (MAX_PATH_LEN - 1))
                {
                    c = MAX_PATH_LEN - 1;                                    //A large amount of data remains; the length of a single read/write must not exceed sizeof( mCmdParam.Other.mBuffer ) */
                }
                else
                {
                    c = TotalCount;                                          //Number of bytes remaining at the end
                }
                mCmdParam.ByteRead.mByteCount = c;                           //Request to read a few dozen bytes of data
                mCmdParam.ByteRead.mByteBuffer = &Com_Buffer[0];
                s = CH103ByteRead();                                         //Read a data block in bytes. A single read must not exceed MAX_BYTE_IO. A second call continues reading from where the previous one stopped
                TotalCount -= mCmdParam.ByteRead.mByteCount;                 //Counter; subtract the number of characters actually read so far
                for(i = 0; i != mCmdParam.ByteRead.mByteCount; i++)
                {
                    printf("%c", mCmdParam.ByteRead.mByteBuffer[i]);         //Display the characters read
                }
//                for(i = 0; i != mCmdParam.ByteRead.mByteCount; i++)
//                {
//                    printf("%c", mCmdParam.ByteRead.mByteBuffer[i]);         //Display the characters read
//                }
                if(mCmdParam.ByteRead.mByteCount < c)                        //The number of characters actually read is smaller than requested, which means the end of the file has been reached
                {
                    printf("\r\n");
                    printf("File end\r\n");
                    break;
                }
            }
            printf("Close\r\n");
            i = CH103FileClose();                                            //Close file
            mStopIfError(i);
        }
    }
}

/*********************************************************************
 * @fn      UDisk_USBH_SectorOperation
 *
 * @brief   Demo Function For UDisk File Sector-operation
 *          including Create\Modify\Read\Erase (EXAM6)
 *
 * @return  none
 */
void UDisk_USBH_SectorOperation( void )
{
    uint8_t  SecCount, s, i;
    uint16_t tmp;
    uint8_t  tmpbuf[64];

    UDisk_USBH_DiskReady( );
    if( ( CH103DiskStatus >= DISK_MOUNTED )&&( UDisk_Opeation_Flag == 1 ) )
    {
        UDisk_Opeation_Flag = 0;

#if DISK_BASE_BUF_LEN
        if(DISK_BASE_BUF_LEN < CH103vSectorSize)
        {
            /* Check whether the disk data buffer is large enough; CHRV3vSectorSize is the actual sector size of the USB drive */
            printf("Too large sector size\n");
            return;
        }
#endif
        /* Read the original file */
        printf("Open\r\n");
        strcpy((char *)mCmdParam.Open.mPathName, "/C51/CHRV3HFT.C");    //File name; the file is located in the C51 subdirectory
        s = CH103FileOpen();                                           //Open file
        if(s == ERR_MISS_DIR || s == ERR_MISS_FILE)                    //File not found
        {
            printf("File not found\r\n");
        }
        else                                                           //The file was found, or an error occurred
        {
            printf("Query\r\n");
            i = CH103FileQuery();                                      //Query the information of the current file
            mStopIfError(i);
            printf("Read\r\n");
            CH103vFileSize = CH103vFileSize + (sizeof(MY_DATA_BUF) - 1); //Length of the original file
            SecCount = CH103vFileSize / sizeof(MY_DATA_BUF);          //Calculate the number of sectors of the file. Since reads and writes are in sectors, CHRV3vSectorSize-1 is added first so that the partial sector at the end of the file is included
            printf("Size=%ld, Sec=%d\r\n", (long)CH103vFileSize, (uint16_t)SecCount);
            while(SecCount--)
            {
                mCmdParam.Read.mSectorCount = sizeof(MY_DATA_BUF) / 512; //Read all the data; if it spans more than 2 sectors, only 2 sectors are read
                mCmdParam.Read.mDataBuffer = &MY_DATA_BUF[0];            //Start address of the file data buffer
                i = CH103FileRead();                                     //Read data from the file
                mStopIfError(i);
                if(SecCount == 0) break;
//                for(tmp=0; tmp<sizeof( MY_DATA_BUF ); tmp++)
//                {
//                    printf("%02X ",(uint16_t)MY_DATA_BUF[tmp]);
//                }
//                printf("\n");
            }
            tmp = (CH103vFileSize - (sizeof(MY_DATA_BUF) - 1)) % sizeof(MY_DATA_BUF);
            if((tmp == 0) && (CH103vFileSize != 0)) tmp = sizeof(MY_DATA_BUF);
            CH103vFileSize = CH103vFileSize - (sizeof(MY_DATA_BUF) - 1);  //Restore the length of the original file
//            for(i=0; i<tmp; i++)
//            {
//                printf("%02X ",(uint16_t)MY_DATA_BUF[i]);
//            }
//            printf("\n");

//            //If the file is too large to read in one call, call CHRV3FileRead again to continue; the file pointer advances automatically
//            while ( 1 )
//            {
//                c = 4;   //Four sectors are read at a time; the larger the buffer defined, the more sectors can be read in one pass
//                mCmdParam.Read.mSectorCount = c;   //Number of sectors to read
//                mCmdParam.Read.mDataBuffer = &MY_DATA_BUF[0];  //Start address of the file data buffer
//                CH103FileRead( );   //The file pointer advances automatically after reading
//                //Process data
//                if ( mCmdParam.Read.mSectorCount < c ) break;   //A smaller actual sector count means the end of the file has been reached
//            }
//            //To start reading or writing at a specified position, move the file pointer
//            mCmdParam.Locate.mSectorOffset = 3;  //Skip the first 3 sectors of the file and start reading or writing
//            i = CH103FileLocate( );
//            mCmdParam.Read.mSectorCount = 10;
//            mCmdParam.Read.mDataBuffer = &MY_DATA_BUF[0];  //Start address of the file data buffer
//            CH103FileRead( );  // Read directly from byte (CHRV3vSectorSizeH*256*3) of the file; the first 3 sectors are skipped
//            //To append new data to the end of the original file, move the file pointer
//            i = CH103FileOpen( );
//            mCmdParam.Locate.mSectorOffset = 0xffffffff;  //Move to the end of the file, in sectors. If the original file is 3 bytes long, appending starts at byte CHRV3vSectorSizeH
//            i = CH103FileLocate( );
//            mCmdParam.Write.mSectorCount = 10;
//            mCmdParam.Write.mDataBuffer = &MY_DATA_BUF[0];
//            CH103FileWrite( );   //Append data after the original file content
//            //CHRV3FileRead lets you define the start address of the data buffer yourself
//            mCmdParam.Read.mSectorCount = 2;
//            mCmdParam.Read.mDataBuffer = 0x50;  //The data read is placed in the buffer starting at 50H; the start address of the buffer must be specified
//            CH103FileRead( );   //Read 2 sectors from the file into the given buffer
//            //CHRV3FileWrite lets you define the start address of the data buffer yourself
//            mCmdParam.Write.mSectorCount = 2;
//            mCmdParam.Write.mDataBuffer = 0x50;  //Write the data in the buffer starting at 50H
//            CH103FileWrite( );   //Write the data in the given buffer into the file as 2 sectors

            printf("Close\r\n");
            i = CH103FileClose();                                        //Close file
            mStopIfError(i);
        }
        printf("Create\r\n");
        strcpy((char *)mCmdParam.Create.mPathName, "/NEWFILE.TXT");       //New file name, in the root directory; a Chinese file name
        s = CH103FileCreate();                                           //Create and open a new file; if the file already exists, delete it first */
        mStopIfError(s);
        printf("Write\r\n");
        strcpy((char *)tmpbuf, "000ABCDEFGHIJKLMNOPQRSTUVWXYZ\xd\xa");    //Prepare to write file data
        for(i = 0; i < (DISK_BASE_BUF_LEN / sizeof(tmpbuf)); i++)
        {
            tmp = i * sizeof(tmpbuf);
            strcpy((char *)&MY_DATA_BUF[tmp], (char *)tmpbuf);
        }
        for(tmp = 0; tmp < sizeof(MY_DATA_BUF); tmp++)
        {
            printf("%02X", (uint16_t)MY_DATA_BUF[tmp]);
        }
        printf("\r\n");
        for(s = 0; s < 10; s++)
        {
            mCmdParam.Write.mSectorCount = 1;                 //Write the data of all sectors
            mCmdParam.Write.mDataBuffer = &MY_DATA_BUF[0];    //Start address of the file data buffer
            i = CH103FileWrite();                             //Write data to the file
            mStopIfError(i);
            printf("Successfully wrote %02X times\r\n", (uint16_t)s);
        }
//        printf( "Modify\n" );
//        mCmdParam.Modify.mFileAttr = 0xff;                           //Input: new file attribute; 0FFH means no change
//        mCmdParam.Modify.mFileTime = 0xffff;                         //Input: new file time; 0FFFFH means no change, and the default time of the newly created file is used
//        mCmdParam.Modify.mFileDate = MAKE_FILE_DATE( 2015, 5, 18 );  //Input: new file date: 2015.05.18
//        mCmdParam.Modify.mFileSize = 0xffffffff;                     //Input: new file length in bytes; the length is updated automatically when the library closes the file, so it is not modified here
//        i = CH103FileModify( );                                      //Modify the information of the current file; modify the date
//        mStopIfError( i );
        printf("Close\r\n");
        mCmdParam.Close.mUpdateLen = 1;                                    //File length is calculated automatically. Data is written in bytes. It is recommended to let the library close the file so that the file length is updated automatically
        i = CH103FileClose();
//        mStopIfError(i);
//        /* Delete a file,An OLD.C file needs to be prepared in advance in the root directory of the USB Disk. */
//        /* Delete a file named OLD.C, which must be placed in the root directory of the USB drive beforehand */
//        printf( "Erase\r\n" );
//        strcpy( (char *)mCmdParam.Create.mPathName, "/OLD.C" );       //file to be delated, under root directory.//Name of the file to delete, in the root directory
//        i = CH103FileErase( );                                       //delate a file and close.//Delete the file and close it
//        if ( i != ERR_SUCCESS )
//        {
//            printf( "Error: %02X\r\n", (uint16_t)i );                  //Display ERR Info , can be not found a file.//Display an error message; the file may not have been found
//            if( i == ERR_MISS_FILE )
//            {
//                printf( "Didn't file the file\r\n" );
//            }
//        }
//        else
//        {
//            printf( "Erase Over\r\n" );
//        }
    }
}


/*********************************************************************
 * @fn      UDisk_USBH_EnumFiles
 *
 * @brief   Demo Function For Enumerating files in UDisk(EXAM11)
 *
 * @return  none
 */
void UDisk_USBH_EnumFiles( void )
{
    uint8_t  i, s;
    uint16_t j;

    UDisk_USBH_DiskReady( );
    if( ( CH103DiskStatus >= DISK_MOUNTED )&&( UDisk_Opeation_Flag == 1 ) )
    {
        UDisk_Opeation_Flag = 0;
        /* Read the original file */
        printf( "Open\r\n" );
        strcpy( (char *)mCmdParam.Open.mPathName, "/C51/CHRV3HFT.C" );//File name; the file is located in the C51 subdirectory
        s = CH103FileOpen( );                        //Open file
        /* List files */
        if ( s == ERR_MISS_DIR )
        {
            printf("The file does not exist, so all files are listed\r\n");  //C51if the subdirectory does not exist, list all files in the root directory
            pCodeStr = (uint8_t *)"/*";
        }
        else
        {
            pCodeStr = (uint8_t *)"/C51/*";                     //CHRV3HFT.CIf the file does not exist, list the files starting with CHRV3 in the \C51 subdirectory
        }
        printf( "List file %s\r\n", pCodeStr );
        for ( j = 0; j < 10000; j ++ )               //At most the first 10000 files are searched; in practice there is no limit
        {
            strcpy( (char *)mCmdParam.Open.mPathName, (char *)pCodeStr );//Search for a file name; * is a wildcard that matches all files or subdirectories
            i = strlen( (char *)mCmdParam.Open.mPathName );
            mCmdParam.Open.mPathName[ i ] = 0xFF;    //Replace the terminator with the search index according to the string length, from 0 to 254; if it is 0xFF (255), the search index is held in CHRV3vFileSize
            CH103vFileSize = j;                      //Index of the search or enumeration
            i = CH103FileOpen( );                    //Open the file; if the file name contains the wildcard *, the call searches for files instead of opening one
            /* CHRV3FileEnum The only difference from CHRV3FileOpen is that where the latter returns ERR_FOUND_NAME, the former returns ERR_SUCCESS */
            if ( i == ERR_MISS_FILE )
            {
                break;                                //No further matching files can be found; no file name matches
            }
            if ( i == ERR_FOUND_NAME )
            {
                /* A file name matching the wildcard was found; the file name and its full path are in the command buffer */
                printf( "  match file %04d#: %s\r\n", (unsigned int)j, mCmdParam.Open.mPathName );//Display the index and the matching file name or subdirectory name that was found
                continue;                             //Continue searching for the next matching file name; the index is incremented on the next search
            }
            else
            {
                /* Error */
                mStopIfError( i );
                break;
            }
        }
        printf( "Close\r\n" );
        CH103FileClose( );                            //Close file
        printf( "USB drive demonstration completed\r\n" );
    }
}
