/*******************************************************************************
**                                                                            **
**  Copyright (C) AUTOSarZs olc (2020)		                                  **
**                                                                            **
**  All rights reserved.                                                      **
**                                                                            **
**  This document contains proprietary information belonging to AUTOSarZs     **
**  olc . Passing on and copying of this document, and communication          **
**  of its contents is not permitted without prior written authorization.     **
**                                                                            **
********************************************************************************
**                                                                            **
**  FILENAME     : CanIf.c         			                                      **
**                                                                            **
**  VERSION      : 1.0.0                                                      **
**                                                                            **
**  DATE         : 2020-01-26                                                 **
**                                                                            **
**  VARIANT      : Variant PB                                                 **
**                                                                            **
**  PLATFORM     : TIVA C		                                              **
**                                                                            **
**  AUTHOR       : AUTOSarZs-DevTeam	                                      **
**                                                                            **
**  VENDOR       : AUTOSarZs OLC	                                          **
**                                                                            **
**                                                                            **
**  DESCRIPTION  : CAN Interface source file                                  **
**                                                                            **
**  SPECIFICATION(S) : Specification of CAN Interface, AUTOSAR Release 4.3.1  **
**                                                                            **
**  MAY BE CHANGED BY USER : no                                               **
**                                                                            **
*******************************************************************************/


/*****************************************************************************************/
/*                                   Include Common headres                              */
/*****************************************************************************************/

/*****************************************************************************************/
/*                                   Include Other  headres                              */
/*****************************************************************************************/



/*****************************************************************************************/
/*                                   Include Component headres                           */
/*****************************************************************************************/
#include "CanIf.h"

/*****************************************************************************************/
/*                                   Local Macro Definition                              */
/*****************************************************************************************/


/*****************************************************************************************/
/*                                   Local Definition                                    */
/*****************************************************************************************/
/*    Description        :
 */

/*****************************************************************************************/
/*                                   Local types Definition                              */
/*****************************************************************************************/
/*
 *  Type Description : Struct to map CanIds to a specific L-PDU of type dynamic  .
 */
typedef struct
{
   Can_IdType     CanId;
   PduIdType      PduId;
}str_MapCanIdToPdu ;


/*****************************************************************************************/
/*                                Exported Variables Definition                          */
/*****************************************************************************************/
/*    Type Description        :                                                          */
/*    Type range              :                                                          */

/*****************************************************************************************/
/*                                Local Variables Definition                             */
/*****************************************************************************************/

/*Array of struct to map CanIds to a specific L-PDU of type dynamic*/
static str_MapCanIdToPdu  MapCanIdToPdu[TX_CAN_L_PDU_NUM] = {0};


/*Pointer to save configuration parameters set */
static CanIf_ConfigType*    CanIf_ConfigPtr = NULL_PTR;

/*Array to save each logical controller PDUs mode */
static CanIf_PduModeType CanIf_PduMode[CANIF_CONTROLLERS_NUM] ;

/* Holding the CanIf module current state. Initially, CANIF_UNINT. */
static CanIf_ModuleStateType CanIf_ModuleState = CANIF_UNINT;

/*****************************************************************************************/
/*                                   Local Function Declaration                          */
/*****************************************************************************************/

/*****************************************************************************************/
/*                                   Local Function Definition                           */
/*****************************************************************************************/




 void CanIf_Init( const CanIf_ConfigType* ConfigPtr )
 {
     PduIdType counter ,DynamicPduCounter = 0;
     uint8 TxBufferSize = 0;
     uint8 PduCounter = 0;

    /*Report Null pointer parameter error*/
    #if (CANIF_DEV_ERROR_DETECT == STD_ON)

     Det_ReportError(CANIF_MODULE_ID, CANIF_INSTANCE_ID,CANIF_INIT_API_ID,CANIF_E_PARAM_POINTER);
    #endif

     /*Save in the global config*/
     CanIf_ConfigPtr = (CanIf_ConfigType*)ConfigPtr ;

     /*
      * [SWS_CANIF_00857]  CanIf_Init() (see [SWS_CANIF_00085]) initializes the CanIds of
      *  the dynamic Transmit L-PDUs with CanIfTxPduType to the value configured
      *  via CanIfTxPduCanId.
      */
     for(counter = 0 ; counter<TX_CAN_L_PDU_NUM;counter++)
     {
         /*Check L-PDUs of dynamic type */
         if(CanIf_ConfigPtr->CanIfInitCfgObj-> CanIfTxPduCfgObj[counter].CanIfTxPduType == DYNAMIC_TX_PDU)
         {
             /*Save CanId init value as CanIfTxPduCanId */
             MapCanIdToPdu[DynamicPduCounter].CanId = CanIf_ConfigPtr->CanIfInitCfgObj->CanIfTxPduCfgObj[counter] \
                                                      .CanIfTxPduCanId ;
             /*Save PduId assigned to this CanId */
             MapCanIdToPdu[DynamicPduCounter].PduId = CanIf_ConfigPtr->CanIfInitCfgObj->CanIfTxPduCfgObj[counter] \
                                                      .CanIfTxPduId ;
             DynamicPduCounter++;
         }
     }

     /*
      * Init Tx buffers only if transmit buffering is enabled
      */
      #if(CANIF_PUBLIC_TX_BUFFERING==STD_ON)

        /* [SWS_CANIF_00387]  When function CanIf_Init() is called, CanIf shall initialize
         * every Transmit L-PDU Buffer assigned to CanIf.
         * The requirement [SWS_CANIF_00387] is necessary to prevent transmission of old data
         *  after restart of the CAN Controller
         */
         for(counter = 0 ; counter<BUFFERS_NUM;counter++)
         {
             /*Get number of PDUs saved in this buffer*/
              TxBufferSize = CanIf_ConfigPtr->CanIfPduTxBuffers[counter].CanIfBufferRef->CanIfBufferSize;

             /*Loop to initialize all PDUs saved in this buffer */
             for(PduCounter =0;PduCounter<TxBufferSize;PduCounter++)
             {
                /*
                 * Initialize all PDUs information
                 */
                 CanIf_ConfigPtr->CanIfPduTxBuffers[counter].CanIfPduInfoRef[PduCounter].CanId   = 0 ;
                 CanIf_ConfigPtr->CanIfPduTxBuffers[counter].CanIfPduInfoRef[PduCounter].TxPduId = 0 ;
                 CanIf_ConfigPtr->CanIfPduTxBuffers[counter].CanIfPduInfoRef[PduCounter].TxPduInfo->MetaDataPtr = NULL_PTR ;
                 CanIf_ConfigPtr->CanIfPduTxBuffers[counter].CanIfPduInfoRef[PduCounter].TxPduInfo->SduDataPtr  = NULL_PTR ;
                 CanIf_ConfigPtr->CanIfPduTxBuffers[counter].CanIfPduInfoRef[PduCounter].TxPduInfo->SduLength   = 0 ;
             }
         }
     #endif
    /*
     * [SWS_CANIF_00864]  During initialization CanIf shall switch every channel to CANIF_OFFLINE
     */
     for( counter=0; counter<CANIF_CONTROLLERS_NUM; counter++)
     {
         CanIf_PduMode[counter] = CANIF_OFFLINE ;
     }

     /*Set module to Ready state*/
     CanIf_ModuleState = CANIF_READY ;
 }
