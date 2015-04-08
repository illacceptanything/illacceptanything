//=====================================================
// CopyRight (C) 2007 Qualcomm Inc. All Rights Reserved.
//
//
// This file is part of Express Card USB Driver
//
// $Id:
//====================================================
// 20090926; aelias; removed compiler warnings; ubuntu 9.04; 2.6.28-15-generic

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/usb.h>
#include <linux/vmalloc.h>
#include "ft1000_usb.h"


#define  DWNLD_HANDSHAKE_LOC     0x02
#define  DWNLD_TYPE_LOC          0x04
#define  DWNLD_SIZE_MSW_LOC      0x06
#define  DWNLD_SIZE_LSW_LOC      0x08
#define  DWNLD_PS_HDR_LOC        0x0A

#define  MAX_DSP_WAIT_LOOPS      40
#define  DSP_WAIT_SLEEP_TIME     1000       /* 1 millisecond */
#define  DSP_WAIT_DISPATCH_LVL   50         /* 50 usec */

#define  HANDSHAKE_TIMEOUT_VALUE 0xF1F1
#define  HANDSHAKE_RESET_VALUE   0xFEFE   /* When DSP requests startover */
#define  HANDSHAKE_RESET_VALUE_USB   0xFE7E   /* When DSP requests startover */
#define  HANDSHAKE_DSP_BL_READY  0xFEFE   /* At start DSP writes this when bootloader ready */
#define  HANDSHAKE_DSP_BL_READY_USB  0xFE7E   /* At start DSP writes this when bootloader ready */
#define  HANDSHAKE_DRIVER_READY  0xFFFF   /* Driver writes after receiving 0xFEFE */
#define  HANDSHAKE_SEND_DATA     0x0000   /* DSP writes this when ready for more data */

#define  HANDSHAKE_REQUEST       0x0001   /* Request from DSP */
#define  HANDSHAKE_RESPONSE      0x0000   /* Satisfied DSP request */

#define  REQUEST_CODE_LENGTH     0x0000
#define  REQUEST_RUN_ADDRESS     0x0001
#define  REQUEST_CODE_SEGMENT    0x0002   /* In WORD count */
#define  REQUEST_DONE_BL         0x0003
#define  REQUEST_DONE_CL         0x0004
#define  REQUEST_VERSION_INFO    0x0005
#define  REQUEST_CODE_BY_VERSION 0x0006
#define  REQUEST_MAILBOX_DATA    0x0007
#define  REQUEST_FILE_CHECKSUM   0x0008

#define  STATE_START_DWNLD       0x01
#define  STATE_BOOT_DWNLD        0x02
#define  STATE_CODE_DWNLD        0x03
#define  STATE_DONE_DWNLD        0x04
#define  STATE_SECTION_PROV      0x05
#define  STATE_DONE_PROV         0x06
#define  STATE_DONE_FILE         0x07

#define  MAX_LENGTH              0x7f0

// Temporary download mechanism for Magnemite
#define  DWNLD_MAG_TYPE_LOC          0x00
#define  DWNLD_MAG_LEN_LOC           0x01
#define  DWNLD_MAG_ADDR_LOC          0x02
#define  DWNLD_MAG_CHKSUM_LOC        0x03
#define  DWNLD_MAG_VAL_LOC           0x04

#define  HANDSHAKE_MAG_DSP_BL_READY  0xFEFE0000   /* At start DSP writes this when bootloader ready */
#define  HANDSHAKE_MAG_DSP_ENTRY     0x01000000   /* Dsp writes this to request for entry address */
#define  HANDSHAKE_MAG_DSP_DATA      0x02000000   /* Dsp writes this to request for data block */
#define  HANDSHAKE_MAG_DSP_DONE      0x03000000   /* Dsp writes this to indicate download done */

#define  HANDSHAKE_MAG_DRV_READY     0xFFFF0000   /* Driver writes this to indicate ready to download */
#define  HANDSHAKE_MAG_DRV_DATA      0x02FECDAB   /* Driver writes this to indicate data available to DSP */
#define  HANDSHAKE_MAG_DRV_ENTRY     0x01FECDAB   /* Driver writes this to indicate entry point to DSP */

#define  HANDSHAKE_MAG_TIMEOUT_VALUE 0xF1F1


// New Magnemite downloader
#define  DWNLD_MAG1_HANDSHAKE_LOC     0x00
#define  DWNLD_MAG1_TYPE_LOC          0x01
#define  DWNLD_MAG1_SIZE_LOC          0x02
#define  DWNLD_MAG1_PS_HDR_LOC        0x03

struct dsp_file_hdr {
   long              version_id;          // Version ID of this image format.
   long              package_id;          // Package ID of code release.
   long              build_date;          // Date/time stamp when file was built.
   long              commands_offset;     // Offset to attached commands in Pseudo Hdr format.
   long              loader_offset;       // Offset to bootloader code.
   long              loader_code_address; // Start address of bootloader.
   long              loader_code_end;     // Where bootloader code ends.
   long              loader_code_size;
   long              version_data_offset; // Offset were scrambled version data begins.
   long              version_data_size;   // Size, in words, of scrambled version data.
   long              nDspImages;          // Number of DSP images in file.
};

#pragma pack(1)
struct dsp_image_info {
   long              coff_date;           // Date/time when DSP Coff image was built.
   long              begin_offset;        // Offset in file where image begins.
   long              end_offset;          // Offset in file where image begins.
   long              run_address;         // On chip Start address of DSP code.
   long              image_size;          // Size of image.
   long              version;             // Embedded version # of DSP code.
   unsigned short    checksum;            // DSP File checksum
   unsigned short    pad1;
};


//---------------------------------------------------------------------------
// Function:    check_usb_db
//
// Parameters:  struct ft1000_device  - device structure
//
// Returns:     0 - success
//
// Description: This function checks if the doorbell register is cleared
//
// Notes:
//
//---------------------------------------------------------------------------
static ULONG check_usb_db (struct ft1000_device *ft1000dev)
{
   int               loopcnt;
   USHORT            temp;
   ULONG             status;

   loopcnt = 0;
   while (loopcnt < 10)
   {

      status = ft1000_read_register (ft1000dev, &temp, FT1000_REG_DOORBELL);
      DEBUG("check_usb_db: read FT1000_REG_DOORBELL value is %x\n", temp);
      if (temp & 0x0080)
      {
           DEBUG("FT1000:Got checkusb doorbell\n");
           status = ft1000_write_register (ft1000dev, 0x0080, FT1000_REG_DOORBELL);
           status = ft1000_write_register (ft1000dev, 0x0100, FT1000_REG_DOORBELL);
           status = ft1000_write_register (ft1000dev,  0x8000, FT1000_REG_DOORBELL);
           break;
      }
      else
      {
           loopcnt++;
           msleep (10);
      }

   } //end of while


   loopcnt = 0;
   while (loopcnt < 20)
   {

      status = ft1000_read_register (ft1000dev, &temp, FT1000_REG_DOORBELL);
      DEBUG("FT1000:check_usb_db:Doorbell = 0x%x\n", temp);
      if (temp & 0x8000)
      {
         loopcnt++;
         msleep (10);
      }
      else
      {
         DEBUG("check_usb_db: door bell is cleared, return 0\n");
         return 0;
      }
#if 0
      // Check if Card is present
      status = ft1000_read_register (ft1000dev, &temp, FT1000_REG_SUP_IMASK);
      if (temp == 0x0000) {
          break;
      }

      status = ft1000_read_register (ft1000dev, &temp, FT1000_REG_ASIC_ID);
      if (temp == 0xffff) {
         break;
      }
#endif
   }

   return HANDSHAKE_MAG_TIMEOUT_VALUE;

}

//---------------------------------------------------------------------------
// Function:    get_handshake
//
// Parameters:  struct ft1000_device  - device structure
//              USHORT expected_value - the handshake value expected
//
// Returns:     handshakevalue - success
//              HANDSHAKE_TIMEOUT_VALUE - failure
//
// Description: This function gets the handshake and compare with the expected value
//
// Notes:
//
//---------------------------------------------------------------------------
static USHORT get_handshake(struct ft1000_device *ft1000dev, USHORT expected_value)
{
   USHORT            handshake;
   int               loopcnt;
   ULONG             status=0;
	struct ft1000_info *pft1000info = netdev_priv(ft1000dev->net);

   loopcnt = 0;
   while (loopcnt < 100)
   {

           // Need to clear downloader doorbell if Hartley ASIC
           status = ft1000_write_register (ft1000dev,  FT1000_DB_DNLD_RX, FT1000_REG_DOORBELL);
           //DEBUG("FT1000:get_handshake:doorbell = 0x%x\n", temp);
               if (pft1000info->fcodeldr)
               {
                   DEBUG(" get_handshake: fcodeldr is %d\n", pft1000info->fcodeldr);
                   pft1000info->fcodeldr = 0;
                   status = check_usb_db(ft1000dev);
                   if (status != STATUS_SUCCESS)
                   {
                       DEBUG("get_handshake: check_usb_db failed\n");
                       status = STATUS_FAILURE;
                       break;
                   }
                   status = ft1000_write_register (ft1000dev,  FT1000_DB_DNLD_RX, FT1000_REG_DOORBELL);
               }

                status = ft1000_read_dpram16 (ft1000dev, DWNLD_MAG1_HANDSHAKE_LOC, (PUCHAR)&handshake, 1);
                //DEBUG("get_handshake: handshake is %x\n", tempx);
                handshake = ntohs(handshake);
                //DEBUG("get_handshake: after swap, handshake is %x\n", handshake);

        if (status)
           return HANDSHAKE_TIMEOUT_VALUE;

        //DEBUG("get_handshake: handshake= %x\n", handshake);
        if ((handshake == expected_value) || (handshake == HANDSHAKE_RESET_VALUE_USB))
        {
            //DEBUG("get_handshake: return handshake %x\n", handshake);
            return handshake;
        }
        else
        {
            loopcnt++;
            msleep (10);
        }
        //DEBUG("HANDSHKE LOOP: %d\n", loopcnt);

   }

   //DEBUG("get_handshake: return handshake time out\n");
   return HANDSHAKE_TIMEOUT_VALUE;
}

//---------------------------------------------------------------------------
// Function:    put_handshake
//
// Parameters:  struct ft1000_device  - device structure
//              USHORT handshake_value - handshake to be written
//
// Returns:     none
//
// Description: This function write the handshake value to the handshake location
//              in DPRAM
//
// Notes:
//
//---------------------------------------------------------------------------
static void put_handshake(struct ft1000_device *ft1000dev,USHORT handshake_value)
{
    ULONG tempx;
    USHORT tempword;
    ULONG status;



        tempx = (ULONG)handshake_value;
        tempx = ntohl(tempx);

        tempword = (USHORT)(tempx & 0xffff);
        status = ft1000_write_dpram16 (ft1000dev, DWNLD_MAG1_HANDSHAKE_LOC, tempword, 0);
        tempword = (USHORT)(tempx >> 16);
        status = ft1000_write_dpram16 (ft1000dev, DWNLD_MAG1_HANDSHAKE_LOC, tempword, 1);
        status = ft1000_write_register(ft1000dev, FT1000_DB_DNLD_TX, FT1000_REG_DOORBELL);
}

static USHORT get_handshake_usb(struct ft1000_device *ft1000dev, USHORT expected_value)
{
   USHORT            handshake;
   int               loopcnt;
   USHORT            temp;
   ULONG             status=0;

	struct ft1000_info *pft1000info = netdev_priv(ft1000dev->net);
   loopcnt = 0;
   handshake = 0;
   while (loopcnt < 100)
   {
       if (pft1000info->usbboot == 2) {
           status = ft1000_read_dpram32 (ft1000dev, 0, (PUCHAR)&(pft1000info->tempbuf[0]), 64);
           for (temp=0; temp<16; temp++)
               DEBUG("tempbuf %d = 0x%x\n", temp, pft1000info->tempbuf[temp]);
           status = ft1000_read_dpram16 (ft1000dev, DWNLD_MAG1_HANDSHAKE_LOC, (PUCHAR)&handshake, 1);
           DEBUG("handshake from read_dpram16 = 0x%x\n", handshake);
           if (pft1000info->dspalive == pft1000info->tempbuf[6])
               handshake = 0;
           else {
               handshake = pft1000info->tempbuf[1];
               pft1000info->dspalive = pft1000info->tempbuf[6];
           }
       }
       else {
           status = ft1000_read_dpram16 (ft1000dev, DWNLD_MAG1_HANDSHAKE_LOC, (PUCHAR)&handshake, 1);
       }
       loopcnt++;
       msleep(10);
       handshake = ntohs(handshake);
       if ((handshake == expected_value) || (handshake == HANDSHAKE_RESET_VALUE_USB))
       {
           return handshake;
       }
   }

   return HANDSHAKE_TIMEOUT_VALUE;
}

static void put_handshake_usb(struct ft1000_device *ft1000dev,USHORT handshake_value)
{
   int i;

        for (i=0; i<1000; i++);
}

//---------------------------------------------------------------------------
// Function:    get_request_type
//
// Parameters:  struct ft1000_device  - device structure
//
// Returns:     request type - success
//
// Description: This function returns the request type
//
// Notes:
//
//---------------------------------------------------------------------------
static USHORT get_request_type(struct ft1000_device *ft1000dev)
{
   USHORT   request_type;
   ULONG    status;
   USHORT   tempword;
   ULONG    tempx;
	struct ft1000_info *pft1000info = netdev_priv(ft1000dev->net);

   if ( pft1000info->bootmode == 1)
   {
       status = fix_ft1000_read_dpram32 (ft1000dev, DWNLD_MAG1_TYPE_LOC, (PUCHAR)&tempx);
       tempx = ntohl(tempx);
   }
   else
   {
       tempx = 0;

       status = ft1000_read_dpram16 (ft1000dev, DWNLD_MAG1_TYPE_LOC, (PUCHAR)&tempword, 1);
       tempx |= (tempword << 16);
       tempx = ntohl(tempx);
   }
   request_type = (USHORT)tempx;

   //DEBUG("get_request_type: request_type is %x\n", request_type);
   return request_type;

}

static USHORT get_request_type_usb(struct ft1000_device *ft1000dev)
{
   USHORT   request_type;
   ULONG    status;
   USHORT   tempword;
   ULONG    tempx;
	struct ft1000_info *pft1000info = netdev_priv(ft1000dev->net);
   if ( pft1000info->bootmode == 1)
   {
       status = fix_ft1000_read_dpram32 (ft1000dev, DWNLD_MAG1_TYPE_LOC, (PUCHAR)&tempx);
       tempx = ntohl(tempx);
   }
   else
   {
       if (pft1000info->usbboot == 2) {
          tempx = pft1000info->tempbuf[2];
          tempword = pft1000info->tempbuf[3];
       }
       else {
          tempx = 0;
          status = ft1000_read_dpram16 (ft1000dev, DWNLD_MAG1_TYPE_LOC, (PUCHAR)&tempword, 1);
       }
       tempx |= (tempword << 16);
       tempx = ntohl(tempx);
   }
   request_type = (USHORT)tempx;

   //DEBUG("get_request_type: request_type is %x\n", request_type);
   return request_type;

}

//---------------------------------------------------------------------------
// Function:    get_request_value
//
// Parameters:  struct ft1000_device  - device structure
//
// Returns:     request value - success
//
// Description: This function returns the request value
//
// Notes:
//
//---------------------------------------------------------------------------
static long get_request_value(struct ft1000_device *ft1000dev)
{
   ULONG     value;
   USHORT   tempword;
   ULONG    status;
	struct ft1000_info *pft1000info = netdev_priv(ft1000dev->net);


       if ( pft1000info->bootmode == 1)
       {
	   status = fix_ft1000_read_dpram32(ft1000dev, DWNLD_MAG1_SIZE_LOC, (PUCHAR)&value);
	   value = ntohl(value);
       }
       else
       {
	   status = ft1000_read_dpram16(ft1000dev, DWNLD_MAG1_SIZE_LOC, (PUCHAR)&tempword, 0);
	   value = tempword;
           status = ft1000_read_dpram16(ft1000dev, DWNLD_MAG1_SIZE_LOC, (PUCHAR)&tempword, 1);
	   value |= (tempword << 16);
	   value = ntohl(value);
       }


   //DEBUG("get_request_value: value is %x\n", value);
   return value;

}

#if 0
static long get_request_value_usb(struct ft1000_device *ft1000dev)
{
   ULONG     value;
   USHORT   tempword;
   ULONG    status;
   struct ft1000_info * pft1000info = netdev_priv(ft1000dev->net);

       if (pft1000info->usbboot == 2) {
          value = pft1000info->tempbuf[4];
          tempword = pft1000info->tempbuf[5];
       }
       else {
          value = 0;
          status = ft1000_read_dpram16(ft1000dev, DWNLD_MAG1_SIZE_LOC, (PUCHAR)&tempword, 1);
       }

       value |= (tempword << 16);
       value = ntohl(value);

   if (pft1000info->usbboot == 1)
       pft1000info->usbboot = 2;

   //DEBUG("get_request_value_usb: value is %x\n", value);
   return value;

}
#endif

//---------------------------------------------------------------------------
// Function:    put_request_value
//
// Parameters:  struct ft1000_device  - device structure
//              long lvalue - value to be put into DPRAM location DWNLD_MAG1_SIZE_LOC
//
// Returns:     none
//
// Description: This function writes a value to DWNLD_MAG1_SIZE_LOC
//
// Notes:
//
//---------------------------------------------------------------------------
static void put_request_value(struct ft1000_device *ft1000dev, long lvalue)
{
   ULONG    tempx;
   ULONG    status;

       tempx = ntohl(lvalue);
       status = fix_ft1000_write_dpram32(ft1000dev, DWNLD_MAG1_SIZE_LOC, (PUCHAR)&tempx);



   //DEBUG("put_request_value: value is %x\n", lvalue);

}



//---------------------------------------------------------------------------
// Function:    hdr_checksum
//
// Parameters:  struct pseudo_hdr *pHdr - Pseudo header pointer
//
// Returns:     checksum - success
//
// Description: This function returns the checksum of the pseudo header
//
// Notes:
//
//---------------------------------------------------------------------------
static USHORT hdr_checksum(struct pseudo_hdr *pHdr)
{
   USHORT   *usPtr = (USHORT *)pHdr;
   USHORT   chksum;


  chksum = ((((((usPtr[0] ^ usPtr[1]) ^ usPtr[2]) ^ usPtr[3]) ^
                    usPtr[4]) ^ usPtr[5]) ^ usPtr[6]);

  return chksum;
}


//---------------------------------------------------------------------------
// Function:    write_blk
//
// Parameters:  struct ft1000_device  - device structure
//              USHORT **pUsFile - DSP image file pointer in USHORT
//              UCHAR  **pUcFile - DSP image file pointer in UCHAR
//              long   word_length - lenght of the buffer to be written
//                                   to DPRAM
//
// Returns:     STATUS_SUCCESS - success
//              STATUS_FAILURE - failure
//
// Description: This function writes a block of DSP image to DPRAM
//
// Notes:
//
//---------------------------------------------------------------------------
static ULONG write_blk (struct ft1000_device *ft1000dev, USHORT **pUsFile, UCHAR **pUcFile, long word_length)
{
   ULONG Status = STATUS_SUCCESS;
   USHORT dpram;
   long temp_word_length;
   int loopcnt, i, j;
   USHORT *pTempFile;
   USHORT tempword;
   USHORT tempbuffer[64];
   USHORT resultbuffer[64];
	struct ft1000_info *pft1000info = netdev_priv(ft1000dev->net);

   //DEBUG("FT1000:download:start word_length = %d\n",(int)word_length);
   dpram = (USHORT)DWNLD_MAG1_PS_HDR_LOC;
   tempword = *(*pUsFile);
   (*pUsFile)++;
   Status = ft1000_write_dpram16(ft1000dev, dpram, tempword, 0);
   tempword = *(*pUsFile);
   (*pUsFile)++;
   Status = ft1000_write_dpram16(ft1000dev, dpram++, tempword, 1);

   *pUcFile = *pUcFile + 4;
   word_length--;
   tempword = (USHORT)word_length;
   word_length = (word_length / 16) + 1;
   pTempFile = *pUsFile;
   temp_word_length = word_length;
   for (; word_length > 0; word_length--) /* In words */
   {
	   loopcnt = 0;

	      for (i=0; i<32; i++)
	      {
		       if (tempword != 0)
		       {
    		           tempbuffer[i++] = *(*pUsFile);
			   (*pUsFile)++;
   	    	           tempbuffer[i] = *(*pUsFile);
			   (*pUsFile)++;
			   *pUcFile = *pUcFile + 4;
			   loopcnt++;
			   tempword--;
		       }
		       else
		       {
			   tempbuffer[i++] = 0;
			   tempbuffer[i] = 0;
                       }
	      }

              //DEBUG("write_blk: loopcnt is %d\n", loopcnt);
              //DEBUG("write_blk: bootmode = %d\n", bootmode);
              //DEBUG("write_blk: dpram = %x\n", dpram);
	      if (pft1000info->bootmode == 0)
	      {
		 if (dpram >= 0x3F4)
                     Status = ft1000_write_dpram32 (ft1000dev, dpram, (PUCHAR)&tempbuffer[0], 8);
	         else
                    Status = ft1000_write_dpram32 (ft1000dev, dpram, (PUCHAR)&tempbuffer[0], 64);
	      }
	      else
	      {
                 for (j=0; j<10; j++)
                 {
                   Status = ft1000_write_dpram32 (ft1000dev, dpram, (PUCHAR)&tempbuffer[0], 64);
		   if (Status == STATUS_SUCCESS)
		   {
		       // Work around for ASIC bit stuffing problem.
		       if ( (tempbuffer[31] & 0xfe00) == 0xfe00)
		       {
      		           Status = ft1000_write_dpram32(ft1000dev, dpram+12, (PUCHAR)&tempbuffer[24], 64);
		       }
    		       // Let's check the data written
	    	       Status = ft1000_read_dpram32 (ft1000dev, dpram, (PUCHAR)&resultbuffer[0], 64);
		       if ( (tempbuffer[31] & 0xfe00) == 0xfe00)
		       {
		           for (i=0; i<28; i++)
		           {
			       if (resultbuffer[i] != tempbuffer[i])
			       {
			           //NdisMSleep (100);
                                   DEBUG("FT1000:download:DPRAM write failed 1 during bootloading\n");
				   msleep(10);
     	  			   Status = STATUS_FAILURE;
				   break;
				}
			   }
   			   Status = ft1000_read_dpram32 (ft1000dev, dpram+12, (PUCHAR)&resultbuffer[0], 64);
		           for (i=0; i<16; i++)
		           {
    			       if (resultbuffer[i] != tempbuffer[i+24])
    			       {
                                   //NdisMSleep (100);
                                   DEBUG("FT1000:download:DPRAM write failed 2 during bootloading\n");
				   msleep(10);
				   Status = STATUS_FAILURE;
				   break;
				}
			   }
			}
			else
			{
			    for (i=0; i<32; i++)
			    {
    			        if (resultbuffer[i] != tempbuffer[i])
    			        {
                                    //NdisMSleep (100);
                                    DEBUG("FT1000:download:DPRAM write failed 3 during bootloading\n");
				    msleep(10);
				    Status = STATUS_FAILURE;
				    break;
				}
			    }
			}

			if (Status == STATUS_SUCCESS)
			    break;

		    }
		}

		if (Status != STATUS_SUCCESS)
		{
                    DEBUG("FT1000:download:Write failed tempbuffer[31] = 0x%x\n", tempbuffer[31]);
		    break;
		}

	     }
   	     dpram = dpram + loopcnt;
   }

   return Status;
}

static void usb_dnld_complete (struct urb *urb)
{
    //DEBUG("****** usb_dnld_complete\n");
}

//---------------------------------------------------------------------------
// Function:    write_blk_fifo
//
// Parameters:  struct ft1000_device  - device structure
//              USHORT **pUsFile - DSP image file pointer in USHORT
//              UCHAR  **pUcFile - DSP image file pointer in UCHAR
//              long   word_length - lenght of the buffer to be written
//                                   to DPRAM
//
// Returns:     STATUS_SUCCESS - success
//              STATUS_FAILURE - failure
//
// Description: This function writes a block of DSP image to DPRAM
//
// Notes:
//
//---------------------------------------------------------------------------
static ULONG write_blk_fifo (struct ft1000_device *ft1000dev, USHORT **pUsFile, UCHAR **pUcFile, long word_length)
{
   ULONG Status = STATUS_SUCCESS;
   int byte_length;
   long aligncnt;

   byte_length = word_length * 4;

   if (byte_length % 4)
      aligncnt = 4 - (byte_length % 4);
   else
      aligncnt = 0;
   byte_length += aligncnt;

   if (byte_length && ((byte_length % 64) == 0)) {
      byte_length += 4;
   }

   if (byte_length < 64)
       byte_length = 68;

#if 0
   pblk = kzalloc(byte_length, GFP_KERNEL);
   memcpy (pblk, *pUcFile, byte_length);

   pipe = usb_sndbulkpipe (ft1000dev->dev, ft1000dev->bulk_out_endpointAddr);

   Status = usb_bulk_msg (ft1000dev->dev,
                          pipe,
                          pblk,
                          byte_length,
                          &cnt,
                          10);
   DEBUG("write_blk_fifo Status = 0x%8x Bytes Transfer = %d Data = 0x%x\n", Status, cnt, *pblk);

   kfree(pblk);
#else
    usb_init_urb(ft1000dev->tx_urb);
    memcpy (ft1000dev->tx_buf, *pUcFile, byte_length);
    usb_fill_bulk_urb(ft1000dev->tx_urb,
                      ft1000dev->dev,
                      usb_sndbulkpipe(ft1000dev->dev, ft1000dev->bulk_out_endpointAddr),
                      ft1000dev->tx_buf,
                      byte_length,
                      usb_dnld_complete,
                      (void*)ft1000dev);

    usb_submit_urb(ft1000dev->tx_urb, GFP_ATOMIC);
#endif

   *pUsFile = *pUsFile + (word_length << 1);
   *pUcFile = *pUcFile + (word_length << 2);

   return Status;
}

//---------------------------------------------------------------------------
//
//  Function:   scram_dnldr
//
//  Synopsis:   Scramble downloader for Harley based ASIC via USB interface
//
//  Arguments:  pFileStart              - pointer to start of file
//              FileLength              - file length
//
//  Returns:    status                  - return code
//---------------------------------------------------------------------------

u16 scram_dnldr(struct ft1000_device *ft1000dev, void *pFileStart, ULONG  FileLength)
{
   u16                     Status = STATUS_SUCCESS;
   UINT                    uiState;
   USHORT                  handshake;
	struct pseudo_hdr *pHdr;
   USHORT                  usHdrLength;
   long                    word_length;
   USHORT                  request;
   USHORT                  temp;
   USHORT                  tempword;

	struct dsp_file_hdr *pFileHdr5;
	struct dsp_image_info *pDspImageInfoV6 = NULL;
   long                    requested_version;
   BOOLEAN                 bGoodVersion;
	struct drv_msg *pMailBoxData;
   USHORT                  *pUsData = NULL;
   USHORT                  *pUsFile = NULL;
   UCHAR                   *pUcFile = NULL;
   UCHAR                   *pBootEnd = NULL, *pCodeEnd= NULL;
   int                     imageN;
   long                    loader_code_address, loader_code_size = 0;
   long                    run_address = 0, run_size = 0;

   ULONG                   templong;
   ULONG                   image_chksum = 0;

   USHORT                  dpram = 0;
   PUCHAR                  pbuffer;
	struct prov_record *pprov_record;
	struct ft1000_info *pft1000info = netdev_priv(ft1000dev->net);

   DEBUG("Entered   scram_dnldr...\n");

   pft1000info->fcodeldr = 0;
   pft1000info->usbboot = 0;
   pft1000info->dspalive = 0xffff;


   //
   // Get version id of file, at first 4 bytes of file, for newer files.
   //

   uiState = STATE_START_DWNLD;

   pFileHdr5 = (struct dsp_file_hdr *)pFileStart;

   ft1000_write_register (ft1000dev, 0x800, FT1000_REG_MAG_WATERMARK);

      pUsFile = (USHORT *)(pFileStart + pFileHdr5->loader_offset);
      pUcFile = (UCHAR *)(pFileStart + pFileHdr5->loader_offset);

      pBootEnd = (UCHAR *)(pFileStart + pFileHdr5->loader_code_end);

      loader_code_address = pFileHdr5->loader_code_address;
      loader_code_size = pFileHdr5->loader_code_size;
      bGoodVersion = FALSE;

   while ((Status == STATUS_SUCCESS) && (uiState != STATE_DONE_FILE))
   {
      switch (uiState)
      {
      case  STATE_START_DWNLD:
         DEBUG("FT1000:STATE_START_DWNLD\n");
         if (pft1000info->usbboot)
             handshake = get_handshake_usb(ft1000dev, HANDSHAKE_DSP_BL_READY);
         else
             handshake = get_handshake(ft1000dev, HANDSHAKE_DSP_BL_READY);

         if (handshake == HANDSHAKE_DSP_BL_READY)
         {
            DEBUG("scram_dnldr: handshake is HANDSHAKE_DSP_BL_READY, call put_handshake(HANDSHAKE_DRIVER_READY)\n");
            put_handshake(ft1000dev, HANDSHAKE_DRIVER_READY);
         }
         else
         {
            DEBUG("FT1000:download:Download error: Handshake failed\n");
            Status = STATUS_FAILURE;
         }

         uiState = STATE_BOOT_DWNLD;

         break;

      case STATE_BOOT_DWNLD:
         DEBUG("FT1000:STATE_BOOT_DWNLD\n");
         pft1000info->bootmode = 1;
         handshake = get_handshake(ft1000dev, HANDSHAKE_REQUEST);
         if (handshake == HANDSHAKE_REQUEST)
         {
            /*
             * Get type associated with the request.
             */
            request = get_request_type(ft1000dev);
            switch (request)
            {
            case  REQUEST_RUN_ADDRESS:
               DEBUG("FT1000:REQUEST_RUN_ADDRESS\n");
               put_request_value(ft1000dev, loader_code_address);
               break;
            case  REQUEST_CODE_LENGTH:
               DEBUG("FT1000:REQUEST_CODE_LENGTH\n");
               put_request_value(ft1000dev, loader_code_size);
               break;
            case  REQUEST_DONE_BL:
               DEBUG("FT1000:REQUEST_DONE_BL\n");
               /* Reposition ptrs to beginning of code section */
               pUsFile = (USHORT *)(pBootEnd);
               pUcFile = (UCHAR *)(pBootEnd);
               //DEBUG("FT1000:download:pUsFile = 0x%8x\n", (int)pUsFile);
               //DEBUG("FT1000:download:pUcFile = 0x%8x\n", (int)pUcFile);
               uiState = STATE_CODE_DWNLD;
               pft1000info->fcodeldr = 1;
               break;
            case  REQUEST_CODE_SEGMENT:
               //DEBUG("FT1000:REQUEST_CODE_SEGMENT\n");
               word_length = get_request_value(ft1000dev);
               //DEBUG("FT1000:word_length = 0x%x\n", (int)word_length);
               //NdisMSleep (100);
               if (word_length > MAX_LENGTH)
               {
                  DEBUG("FT1000:download:Download error: Max length exceeded\n");
                  Status = STATUS_FAILURE;
                  break;
               }
               if ( (word_length*2 + pUcFile) > pBootEnd)
               {
                  /*
                   * Error, beyond boot code range.
                   */
                  DEBUG("FT1000:download:Download error: Requested len=%d exceeds BOOT code boundry.\n",
                                                            (int)word_length);
                  Status = STATUS_FAILURE;
                  break;
               }
               /*
                * Position ASIC DPRAM auto-increment pointer.
                */
				    dpram = (USHORT)DWNLD_MAG1_PS_HDR_LOC;
					if (word_length & 0x1)
						word_length++;
					word_length = word_length / 2;

			Status =   write_blk(ft1000dev, &pUsFile, &pUcFile, word_length);
			//DEBUG("write_blk returned %d\n", Status);
               break;
            default:
               DEBUG("FT1000:download:Download error: Bad request type=%d in BOOT download state.\n",request);
               Status = STATUS_FAILURE;
               break;
            }
            if (pft1000info->usbboot)
                put_handshake_usb(ft1000dev, HANDSHAKE_RESPONSE);
            else
                put_handshake(ft1000dev, HANDSHAKE_RESPONSE);
         }
         else
         {
            DEBUG("FT1000:download:Download error: Handshake failed\n");
            Status = STATUS_FAILURE;
         }

         break;

      case STATE_CODE_DWNLD:
         //DEBUG("FT1000:STATE_CODE_DWNLD\n");
         pft1000info->bootmode = 0;
         if (pft1000info->usbboot)
            handshake = get_handshake_usb(ft1000dev, HANDSHAKE_REQUEST);
         else
            handshake = get_handshake(ft1000dev, HANDSHAKE_REQUEST);
         if (handshake == HANDSHAKE_REQUEST)
         {
            /*
             * Get type associated with the request.
             */
            if (pft1000info->usbboot)
                request = get_request_type_usb(ft1000dev);
            else
                request = get_request_type(ft1000dev);
            switch (request)
            {
            case REQUEST_FILE_CHECKSUM:
                DEBUG("FT1000:download:image_chksum = 0x%8x\n", image_chksum);
                put_request_value(ft1000dev, image_chksum);
                break;
            case  REQUEST_RUN_ADDRESS:
               DEBUG("FT1000:download:  REQUEST_RUN_ADDRESS\n");
               if (bGoodVersion)
               {
                  DEBUG("FT1000:download:run_address = 0x%8x\n", (int)run_address);
                  put_request_value(ft1000dev, run_address);
               }
               else
               {
                  DEBUG("FT1000:download:Download error: Got Run address request before image offset request.\n");
                  Status = STATUS_FAILURE;
                  break;
               }
               break;
            case  REQUEST_CODE_LENGTH:
               DEBUG("FT1000:download:REQUEST_CODE_LENGTH\n");
               if (bGoodVersion)
               {
                  DEBUG("FT1000:download:run_size = 0x%8x\n", (int)run_size);
                  put_request_value(ft1000dev, run_size);
               }
               else
               {
                  DEBUG("FT1000:download:Download error: Got Size request before image offset request.\n");
                  Status = STATUS_FAILURE;
                  break;
               }
               break;
            case  REQUEST_DONE_CL:
               pft1000info->usbboot = 3;
               /* Reposition ptrs to beginning of provisioning section */
                  pUsFile = (USHORT *)(pFileStart + pFileHdr5->commands_offset);
                  pUcFile = (UCHAR *)(pFileStart + pFileHdr5->commands_offset);
               uiState = STATE_DONE_DWNLD;
               break;
            case  REQUEST_CODE_SEGMENT:
               //DEBUG("FT1000:download: REQUEST_CODE_SEGMENT - CODELOADER\n");
               if (!bGoodVersion)
               {
                  DEBUG("FT1000:download:Download error: Got Code Segment request before image offset request.\n");
                  Status = STATUS_FAILURE;
                  break;
               }
#if 0
               word_length = get_request_value_usb(ft1000dev);
               //DEBUG("FT1000:download:word_length = %d\n", (int)word_length);
               if (word_length > MAX_LENGTH/2)
#else
               word_length = get_request_value(ft1000dev);
               //DEBUG("FT1000:download:word_length = %d\n", (int)word_length);
               if (word_length > MAX_LENGTH)
#endif
               {
                  DEBUG("FT1000:download:Download error: Max length exceeded\n");
                  Status = STATUS_FAILURE;
                  break;
               }
               if ( (word_length*2 + pUcFile) > pCodeEnd)
               {
                  /*
                   * Error, beyond boot code range.
                   */
                  DEBUG("FT1000:download:Download error: Requested len=%d exceeds DSP code boundry.\n",
                               (int)word_length);
                  Status = STATUS_FAILURE;
                  break;
               }
               /*
                * Position ASIC DPRAM auto-increment pointer.
                */
		   dpram = (USHORT)DWNLD_MAG1_PS_HDR_LOC;
		   if (word_length & 0x1)
			word_length++;
		   word_length = word_length / 2;

   	       write_blk_fifo (ft1000dev, &pUsFile, &pUcFile, word_length);
               if (pft1000info->usbboot == 0)
                   pft1000info->usbboot++;
               if (pft1000info->usbboot == 1) {
                   tempword = 0;
                   ft1000_write_dpram16 (ft1000dev, DWNLD_MAG1_PS_HDR_LOC, tempword, 0);
               }

               break;

            case  REQUEST_MAILBOX_DATA:
               DEBUG("FT1000:download: REQUEST_MAILBOX_DATA\n");
               // Convert length from byte count to word count. Make sure we round up.
               word_length = (long)(pft1000info->DSPInfoBlklen + 1)/2;
               put_request_value(ft1000dev, word_length);
		pMailBoxData = (struct drv_msg *)&(pft1000info->DSPInfoBlk[0]);
               /*
                * Position ASIC DPRAM auto-increment pointer.
                */


                   pUsData = (USHORT *)&pMailBoxData->data[0];
                   dpram = (USHORT)DWNLD_MAG1_PS_HDR_LOC;
                   if (word_length & 0x1)
                       word_length++;

                   word_length = (word_length / 2);


               for (; word_length > 0; word_length--) /* In words */
               {

                      templong = *pUsData++;
					  templong |= (*pUsData++ << 16);
                      Status = fix_ft1000_write_dpram32 (ft1000dev, dpram++, (PUCHAR)&templong);

               }
               break;

            case  REQUEST_VERSION_INFO:
               DEBUG("FT1000:download:REQUEST_VERSION_INFO\n");
               word_length = pFileHdr5->version_data_size;
               put_request_value(ft1000dev, word_length);
               /*
                * Position ASIC DPRAM auto-increment pointer.
                */

               pUsFile = (USHORT *)(pFileStart + pFileHdr5->version_data_offset);


                   dpram = (USHORT)DWNLD_MAG1_PS_HDR_LOC;
                   if (word_length & 0x1)
                       word_length++;

                   word_length = (word_length / 2);


               for (; word_length > 0; word_length--) /* In words */
               {

                      templong = ntohs(*pUsFile++);
					  temp = ntohs(*pUsFile++);
					  templong |= (temp << 16);
                      Status = fix_ft1000_write_dpram32 (ft1000dev, dpram++, (PUCHAR)&templong);

               }
               break;

            case  REQUEST_CODE_BY_VERSION:
               DEBUG("FT1000:download:REQUEST_CODE_BY_VERSION\n");
               bGoodVersion = FALSE;
               requested_version = get_request_value(ft1000dev);

                   pDspImageInfoV6 = (struct dsp_image_info *)(pFileStart + sizeof(struct dsp_file_hdr ));

               for (imageN = 0; imageN < pFileHdr5->nDspImages; imageN++)
               {

                       temp = (USHORT)(pDspImageInfoV6->version);
                       templong = temp;
                       temp = (USHORT)(pDspImageInfoV6->version >> 16);
                       templong |= (temp << 16);
                   if (templong == (ULONG)requested_version)
                       {
                           bGoodVersion = TRUE;
                           DEBUG("FT1000:download: bGoodVersion is TRUE\n");
                           pUsFile = (USHORT *)(pFileStart + pDspImageInfoV6->begin_offset);
                           pUcFile = (UCHAR *)(pFileStart + pDspImageInfoV6->begin_offset);
                           pCodeEnd = (UCHAR *)(pFileStart + pDspImageInfoV6->end_offset);
                           run_address = pDspImageInfoV6->run_address;
                           run_size = pDspImageInfoV6->image_size;
                           image_chksum = (ULONG)pDspImageInfoV6->checksum;
                           break;
                        }
                        pDspImageInfoV6++;


               } //end of for

               if (!bGoodVersion)
               {
                  /*
                   * Error, beyond boot code range.
                   */
                  DEBUG("FT1000:download:Download error: Bad Version Request = 0x%x.\n",(int)requested_version);
                  Status = STATUS_FAILURE;
                  break;
               }
               break;

            default:
               DEBUG("FT1000:download:Download error: Bad request type=%d in CODE download state.\n",request);
               Status = STATUS_FAILURE;
               break;
            }
            if (pft1000info->usbboot)
                put_handshake_usb(ft1000dev, HANDSHAKE_RESPONSE);
            else
                put_handshake(ft1000dev, HANDSHAKE_RESPONSE);
         }
         else
         {
            DEBUG("FT1000:download:Download error: Handshake failed\n");
            Status = STATUS_FAILURE;
         }

         break;

      case STATE_DONE_DWNLD:
         DEBUG("FT1000:download:Code loader is done...\n");
         uiState = STATE_SECTION_PROV;
         break;

      case  STATE_SECTION_PROV:
         DEBUG("FT1000:download:STATE_SECTION_PROV\n");
		pHdr = (struct pseudo_hdr *)pUcFile;

         if (pHdr->checksum == hdr_checksum(pHdr))
         {
            if (pHdr->portdest != 0x80 /* Dsp OAM */)
            {
               uiState = STATE_DONE_PROV;
               break;
            }
            usHdrLength = ntohs(pHdr->length);    /* Byte length for PROV records */

            // Get buffer for provisioning data
		pbuffer = kmalloc((usHdrLength + sizeof(struct pseudo_hdr)), GFP_ATOMIC);
            if (pbuffer) {
		memcpy(pbuffer, (void *)pUcFile, (UINT)(usHdrLength + sizeof(struct pseudo_hdr)));
                // link provisioning data
		pprov_record = kmalloc(sizeof(struct prov_record), GFP_ATOMIC);
                if (pprov_record) {
                    pprov_record->pprov_data = pbuffer;
                    list_add_tail (&pprov_record->list, &pft1000info->prov_list);
                    // Move to next entry if available
			pUcFile = (UCHAR *)((unsigned long)pUcFile + (UINT)((usHdrLength + 1) & 0xFFFFFFFE) + sizeof(struct pseudo_hdr));
                    if ( (unsigned long)(pUcFile) - (unsigned long)(pFileStart) >= (unsigned long)FileLength) {
                       uiState = STATE_DONE_FILE;
                    }
                }
                else {
                    kfree(pbuffer);
                    Status = STATUS_FAILURE;
                }
            }
            else {
                Status = STATUS_FAILURE;
            }
         }
         else
         {
            /* Checksum did not compute */
            Status = STATUS_FAILURE;
         }
         DEBUG("ft1000:download: after STATE_SECTION_PROV, uiState = %d, Status= %d\n", uiState, Status);
         break;

      case  STATE_DONE_PROV:
         DEBUG("FT1000:download:STATE_DONE_PROV\n");
         uiState = STATE_DONE_FILE;
         break;


      default:
         Status = STATUS_FAILURE;
         break;
      } /* End Switch */

      if (Status != STATUS_SUCCESS) {
          break;
      }

/****
      // Check if Card is present
      Status = Harley_Read_Register(&temp, FT1000_REG_SUP_IMASK);
      if ( (Status != NDIS_STATUS_SUCCESS) || (temp == 0x0000) ) {
          break;
      }

      Status = Harley_Read_Register(&temp, FT1000_REG_ASIC_ID);
      if ( (Status != NDIS_STATUS_SUCCESS) || (temp == 0xffff) ) {
          break;
      }
****/

   } /* End while */

   DEBUG("Download exiting with status = 0x%8x\n", Status);
   ft1000_write_register(ft1000dev, FT1000_DB_DNLD_TX, FT1000_REG_DOORBELL);

   return Status;
}

