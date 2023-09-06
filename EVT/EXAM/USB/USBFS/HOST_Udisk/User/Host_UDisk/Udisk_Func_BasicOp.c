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
#include "Udisk_Operation.h"\

/*******************************************************************************/
/* Variable Definition */
__attribute__((aligned(4)))  uint8_t  MY_DATA_BUF[ DISK_BASE_BUF_LEN ];   /* MY_DATA_BUFָ���ⲿRAM�Ĵ������ݻ�����,����������Ϊ����һ�������ĳ���,�����û����ݻ��� */
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
        /* ���ļ� */
        strcpy((char *)mCmdParam.Open.mPathName, "/C51/NEWFILE.C");         //���ý�Ҫ�������ļ�·�����ļ���/NEWFILE.C
        s = CH103FileOpen();                                               //���ļ�
        if(s == ERR_MISS_DIR || s == ERR_MISS_FILE)                        //û���ҵ��ļ�
        {
            //�����ļ���ʾ
            printf("Find No File And Create\r\n");
            strcpy((char *)mCmdParam.Create.mPathName, "/NEWFILE.C");       //���ļ���,�ڸ�Ŀ¼��,�����ļ���
            s = CH103FileCreate();                                         //�½��ļ�����,����ļ��Ѿ���������ɾ�������½�
            mStopIfError(s);
            printf("ByteWrite\r\n");
            //ʵ��Ӧ���ж�д���ݳ��ȺͶ��建���������Ƿ������������ڻ�������������Ҫ���д��
            i = sprintf((char *)Com_Buffer, "Note: \xd\xa������������ֽ�Ϊ��λ����U���ļ���д,����ʾ���ܡ�\xd\xa"); //��ʾ
            for(c = 0; c < 10; c++)
            {
                mCmdParam.ByteWrite.mByteCount = i;                          //ָ������д����ֽ���
                mCmdParam.ByteWrite.mByteBuffer = Com_Buffer;                       //ָ�򻺳���
                s = CH103ByteWrite();                                        //���ֽ�Ϊ��λ���ļ�д������
                mStopIfError(s);
                printf("WriteIn Success %02X times\r\n", (uint16_t)c);
            }
            //��ʾ�޸��ļ�����
            printf("Modify\r\n");
            mCmdParam.Modify.mFileAttr = 0xff;                               //�������: �µ��ļ�����,Ϊ0FFH���޸�
            mCmdParam.Modify.mFileTime = 0xffff;                             //�������: �µ��ļ�ʱ��,Ϊ0FFFFH���޸�,ʹ���½��ļ�������Ĭ��ʱ��
            mCmdParam.Modify.mFileDate = MAKE_FILE_DATE(2015, 5, 18);        //�������: �µ��ļ�����: 2015.05.18
            mCmdParam.Modify.mFileSize = 0xffffffff;                         // �������: �µ��ļ�����,���ֽ�Ϊ��λд�ļ�Ӧ���ɳ����ر��ļ�ʱ�Զ����³���,���Դ˴����޸�
            i = CH103FileModify();                                           //�޸ĵ�ǰ�ļ�����Ϣ,�޸�����
            mStopIfError(i);
            printf("Close\r\n");
            mCmdParam.Close.mUpdateLen = 1;                                  //�Զ������ļ�����,���ֽ�Ϊ��λд�ļ�,�����ó����ر��ļ��Ա��Զ������ļ�����
            i = CH103FileClose();
            mStopIfError(i);

            /* ɾ��ĳ���ļ����ļ���OLD.C�� ��Ҫ��ǰ������U�̸�Ŀ¼�� */
            printf("Erase\r\n");
            strcpy((char *)mCmdParam.Create.mPathName, "/OLD.C");            //��Ҫɾ�����ļ������ڸ�Ŀ¼��
            i = CH103FileErase();                                            //ɾ���ļ����ر�
            if(i != ERR_SUCCESS)
            {
                printf("Error: %02X\r\n", (uint16_t)i);                      //��ʾ������Ϣ��������û���ҵ��ļ�
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
            //���ϣ������������ӵ�ԭ�ļ���β��,�����ƶ��ļ�ָ��
            //д���ļ�
            printf("ByteWrite\r\n");
            mCmdParam.ByteLocate.mByteOffset = 0xffffffff;                   //�Ƶ��ļ���β��
            CH103ByteLocate();
            //ʵ��Ӧ���ж�д���ݳ��ȺͶ��建���������Ƿ������������ڻ�������������Ҫ���д��
            i = sprintf((char *)Com_Buffer, "Note: \xd\xa������������ֽ�Ϊ��λ����U���ļ���д,����ʾ���ܡ�\xd\xa");
            for(c = 0; c < 10; c++)
            {
                mCmdParam.ByteWrite.mByteCount = i;                          //ָ������д����ֽ���
                mCmdParam.ByteWrite.mByteBuffer = Com_Buffer;                       //ָ�򻺳���
                s = CH103ByteWrite();                                        //���ֽ�Ϊ��λ���ļ�д������
                mStopIfError(s);
                printf("WriteIn Success %02X times\r\n", (uint16_t)c);
            }

            //��ȡ�ļ�ǰN�ֽ�
            TotalCount = 60;                                                 //����׼����ȡ�ܳ���100�ֽ�
            printf("First %d characters read out:\r\n", TotalCount);
            while(TotalCount)                                                //����ļ��Ƚϴ�,һ�ζ�����,�����ٵ���CH103ByteRead������ȡ,�ļ�ָ���Զ�����ƶ�
            {
                if(TotalCount > (MAX_PATH_LEN - 1))
                {
                    c = MAX_PATH_LEN - 1;                                    //ʣ�����ݽ϶�,���Ƶ��ζ�д�ĳ��Ȳ��ܳ��� sizeof( mCmdParam.Other.mBuffer ) */
                }
                else
                {
                    c = TotalCount;                                          //���ʣ����ֽ���
                }
                mCmdParam.ByteRead.mByteCount = c;                           //���������ʮ�ֽ�����
                mCmdParam.ByteRead.mByteBuffer = &Com_Buffer[0];
                s = CH103ByteRead();                                         //���ֽ�Ϊ��λ��ȡ���ݿ�,���ζ�д�ĳ��Ȳ��ܳ���MAX_BYTE_IO,�ڶ��ε���ʱ���Ÿղŵ�����
                TotalCount -= mCmdParam.ByteRead.mByteCount;                 //����,��ȥ��ǰʵ���Ѿ��������ַ���
                for(i = 0; i != mCmdParam.ByteRead.mByteCount; i++)
                {
                    printf("%c", mCmdParam.ByteRead.mByteBuffer[i]);         //��ʾ�������ַ�
                }
                if(mCmdParam.ByteRead.mByteCount < c)                        //ʵ�ʶ������ַ�������Ҫ��������ַ���,˵���Ѿ����ļ��Ľ�β
                {
                    printf("\r\n");
                    printf("File end\r\n");
                    break;
                }
            }
            printf("Close\r\n");
            i = CH103FileClose();                                            //�ر��ļ�
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
            /* ���������ݻ������Ƿ��㹻��,CHRV3vSectorSize��U�̵�ʵ��������С */
            printf("Too large sector size\n");
            return;
        }
#endif
        /* ��ȡԭ�ļ� */
        printf("Open\r\n");
        strcpy((char *)mCmdParam.Open.mPathName, "/C51/CHRV3HFT.C");    //�ļ���,���ļ���C51��Ŀ¼��
        s = CH103FileOpen();                                           //���ļ�
        if(s == ERR_MISS_DIR || s == ERR_MISS_FILE)                    //û���ҵ��ļ�
        {
            printf("û���ҵ��ļ�\r\n");
        }
        else                                                           //�ҵ��ļ����߳���
        {
            printf("Query\r\n");
            i = CH103FileQuery();                                      //��ѯ��ǰ�ļ�����Ϣ
            mStopIfError(i);
            printf("Read\r\n");
            CH103vFileSize = CH103vFileSize + (sizeof(MY_DATA_BUF) - 1); //ԭ�ļ��ĳ���
            SecCount = CH103vFileSize / sizeof(MY_DATA_BUF);          //�����ļ���������,��Ϊ��д��������Ϊ��λ��,�ȼ�CHRV3vSectorSize-1��Ϊ�˶����ļ�β������1�������Ĳ���
            printf("Size=%ld, Sec=%d\r\n", (long)CH103vFileSize, (uint16_t)SecCount);
            while(SecCount--)
            {
                mCmdParam.Read.mSectorCount = sizeof(MY_DATA_BUF) / 512; //��ȡȫ������,�������2��������ֻ��ȡ2������
                mCmdParam.Read.mDataBuffer = &MY_DATA_BUF[0];            //ָ���ļ����ݻ���������ʼ��ַ
                i = CH103FileRead();                                     //���ļ���ȡ����
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
            CH103vFileSize = CH103vFileSize - (sizeof(MY_DATA_BUF) - 1);  //�ָ�ԭ�ļ��ĳ���
//            for(i=0; i<tmp; i++)
//            {
//                printf("%02X ",(uint16_t)MY_DATA_BUF[i]);
//            }
//            printf("\n");

//            //����ļ��Ƚϴ�,һ�ζ�����,�����ٵ���CHRV3FileRead������ȡ,�ļ�ָ���Զ�����ƶ�
//            while ( 1 )
//            {
//                c = 4;   //ÿ�ζ�ȡ4������,�����������Խ��һ�ζ�ȡ��������Խ��
//                mCmdParam.Read.mSectorCount = c;   //ָ����ȡ��������
//                mCmdParam.Read.mDataBuffer = &MY_DATA_BUF[0];  //ָ���ļ����ݻ���������ʼ��ַ
//                CH103FileRead( );   //������ļ�ָ���Զ�����
//                //��������
//                if ( mCmdParam.Read.mSectorCount < c ) break;   //ʵ�ʶ�������������С��˵���ļ��Ѿ�����
//            }
//            //���ϣ����ָ��λ�ÿ�ʼ��д,�����ƶ��ļ�ָ��
//            mCmdParam.Locate.mSectorOffset = 3;  //�����ļ���ǰ3��������ʼ��д
//            i = CH103FileLocate( );
//            mCmdParam.Read.mSectorCount = 10;
//            mCmdParam.Read.mDataBuffer = &MY_DATA_BUF[0];  //ָ���ļ����ݻ���������ʼ��ַ
//            CH103FileRead( );  // ֱ�Ӷ�ȡ���ļ��ĵ�(CHRV3vSectorSizeH*256*3)���ֽڿ�ʼ������,ǰ3������������
//            //���ϣ������������ӵ�ԭ�ļ���β��,�����ƶ��ļ�ָ��
//            i = CH103FileOpen( );
//            mCmdParam.Locate.mSectorOffset = 0xffffffff;  //�Ƶ��ļ���β��,������Ϊ��λ,���ԭ�ļ���3�ֽ�,���CHRV3vSectorSizeH���ֽڴ���ʼ���
//            i = CH103FileLocate( );
//            mCmdParam.Write.mSectorCount = 10;
//            mCmdParam.Write.mDataBuffer = &MY_DATA_BUF[0];
//            CH103FileWrite( );   //��ԭ�ļ��ĺ����������
//            //ʹ��CHRV3FileRead�������ж������ݻ���������ʼ��ַ
//            mCmdParam.Read.mSectorCount = 2;
//            mCmdParam.Read.mDataBuffer = 0x50;  //�����������ݷŵ�50H��ʼ�Ļ������У���Ҫָ������������ʼ��ַ
//            CH103FileRead( );   //���ļ��ж�ȡ2��������ָ��������
//            //ʹ��CHRV3FileWrite�������ж������ݻ���������ʼ��ַ
//            mCmdParam.Write.mSectorCount = 2;
//            mCmdParam.Write.mDataBuffer = 0x50;  //��50H��ʼ�Ļ������е�����д��
//            CH103FileWrite( );   //��ָ���������е�����д��2���������ļ���

            printf("Close\r\n");
            i = CH103FileClose();                                        //�ر��ļ�
            mStopIfError(i);
        }
        printf("Create\r\n");
        strcpy((char *)mCmdParam.Create.mPathName, "/NEWFILE.TXT");       //���ļ���,�ڸ�Ŀ¼��,�����ļ���
        s = CH103FileCreate();                                           //�½��ļ�����,����ļ��Ѿ���������ɾ�������½� */
        mStopIfError(s);
        printf("Write\r\n");
        strcpy((char *)tmpbuf, "000ABCDEFGHIJKLMNOPQRSTUVWXYZ\xd\xa");    //׼��д�ļ�����
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
            mCmdParam.Write.mSectorCount = 1;                 //д����������������
            mCmdParam.Write.mDataBuffer = &MY_DATA_BUF[0];    //ָ���ļ����ݻ���������ʼ��ַ
            i = CH103FileWrite();                             //���ļ�д������
            mStopIfError(i);
            printf("�ɹ�д�� %02X��\r\n", (uint16_t)s);
        }
//        printf( "Modify\n" );
//        mCmdParam.Modify.mFileAttr = 0xff;                           //�������: �µ��ļ�����,Ϊ0FFH���޸�
//        mCmdParam.Modify.mFileTime = 0xffff;                         //�������: �µ��ļ�ʱ��,Ϊ0FFFFH���޸�,ʹ���½��ļ�������Ĭ��ʱ��
//        mCmdParam.Modify.mFileDate = MAKE_FILE_DATE( 2015, 5, 18 );  //�������: �µ��ļ�����: 2015.05.18
//        mCmdParam.Modify.mFileSize = 0xffffffff;                     //�������: �µ��ļ�����,���ֽ�Ϊ��λд�ļ�Ӧ���ɳ����ر��ļ�ʱ�Զ����³���,���Դ˴����޸�
//        i = CH103FileModify( );                                      //�޸ĵ�ǰ�ļ�����Ϣ,�޸�����
//        mStopIfError( i );
        printf("Close\r\n");
        mCmdParam.Close.mUpdateLen = 1;                                    //�Զ������ļ�����,���ֽ�Ϊ��λд�ļ�,�����ó����ر��ļ��Ա��Զ������ļ�����
        i = CH103FileClose();
//        mStopIfError(i);
//        /* Delete a file��An OLD.C file needs to be prepared in advance in the root directory of the USB Disk. */
//        /* ɾ��ĳ���ļ����ļ���OLD.C�� ��Ҫ��ǰ������U�̸�Ŀ¼�� */
//        printf( "Erase\r\n" );
//        strcpy( (char *)mCmdParam.Create.mPathName, "/OLD.C" );       //file to be delated, under root directory.//��Ҫɾ�����ļ������ڸ�Ŀ¼��
//        i = CH103FileErase( );                                       //delate a file and close��//ɾ���ļ����ر�
//        if ( i != ERR_SUCCESS )
//        {
//            printf( "Error: %02X\r\n", (uint16_t)i );                  //Display ERR Info , can be not found a file.//��ʾ������Ϣ��������û���ҵ��ļ�
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
        /* ��ȡԭ�ļ� */
        printf( "Open\r\n" );
        strcpy( (char *)mCmdParam.Open.mPathName, "/C51/CHRV3HFT.C" );//�ļ���,���ļ���C51��Ŀ¼��
        s = CH103FileOpen( );                        //���ļ�
        /* �г��ļ� */
        if ( s == ERR_MISS_DIR )
        {
            printf("�����ڸ��ļ����г������ļ�\r\n");  //C51��Ŀ¼���������г���Ŀ¼�µ������ļ�
            pCodeStr = (uint8_t *)"/*";
        }
        else
        {
            pCodeStr = (uint8_t *)"/C51/*";                     //CHRV3HFT.C�ļ����������г�\C51��Ŀ¼�µ���CHRV3��ͷ���ļ�
        }
        printf( "List file %s\r\n", pCodeStr );
        for ( j = 0; j < 10000; j ++ )               //�������ǰ10000���ļ�,ʵ����û������
        {
            strcpy( (char *)mCmdParam.Open.mPathName, (char *)pCodeStr );//�����ļ���,*Ϊͨ���,�����������ļ�������Ŀ¼
            i = strlen( (char *)mCmdParam.Open.mPathName );
            mCmdParam.Open.mPathName[ i ] = 0xFF;    //�����ַ������Ƚ��������滻Ϊ���������,��0��254,�����0xFF��255��˵�����������CHRV3vFileSize������
            CH103vFileSize = j;                      //ָ������/ö�ٵ����
            i = CH103FileOpen( );                    //���ļ�,����ļ����к���ͨ���*,��Ϊ�����ļ�������
            /* CHRV3FileEnum �� CHRV3FileOpen ��Ψһ�����ǵ����߷���ERR_FOUND_NAMEʱ��ô��Ӧ��ǰ�߷���ERR_SUCCESS */
            if ( i == ERR_MISS_FILE )
            {
                break;                                //��Ҳ��������ƥ����ļ�,�Ѿ�û��ƥ����ļ���
            }
            if ( i == ERR_FOUND_NAME )
            {
                /* ��������ͨ�����ƥ����ļ���,�ļ�����������·������������� */
                printf( "  match file %04d#: %s\r\n", (unsigned int)j, mCmdParam.Open.mPathName );//��ʾ��ź���������ƥ���ļ���������Ŀ¼��
                continue;                             //����������һ��ƥ����ļ���,�´�����ʱ��Ż��1
            }
            else
            {
                /* ���� */
                mStopIfError( i );
                break;
            }
        }
        printf( "Close\r\n" );
        CH103FileClose( );                            //�ر��ļ�
        printf( "U����ʾ���\r\n" );
    }
}
