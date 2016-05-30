/**************************************************************************************************
  Filename:       simpleGATTprofile.c
  Revised:        $Date: 2015-03-24 09:19:15 -0700 (Tue, 24 Mar 2015) $
  Revision:       $Revision: 43274 $

  Description:    This file contains the Simple GATT profile sample GATT service 
                  profile for use with the BLE sample application.
**************************************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include <string.h>

#include "bcomdef.h"
#include "OSAL.h"
#include "linkdb.h"
#include "att.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"
#include "gapbondmgr.h"

#include "simpleGATTprofile.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

#define SERVAPP_NUM_ATTR_SUPPORTED        17

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
// Simple GATT Profile Service UUID: 0xFFF0
CONST uint8 simpleProfileServUUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(SIMPLEPROFILE_SERV_UUID), HI_UINT16(SIMPLEPROFILE_SERV_UUID)
};

// Characteristic 4 UUID: 0xFFF4
CONST uint8 simpleProfilechar4UUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(SIMPLEPROFILE_CHAR4_UUID), HI_UINT16(SIMPLEPROFILE_CHAR4_UUID)
};

// Characteristic 6 UUID: 0xFFF6
CONST uint8 simpleProfilechar6UUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(SIMPLEPROFILE_CHAR6_UUID), HI_UINT16(SIMPLEPROFILE_CHAR6_UUID)
};

// Characteristic 7 UUID: 0xFFF7
CONST uint8 simpleProfilechar7UUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(SIMPLEPROFILE_CHAR7_UUID), HI_UINT16(SIMPLEPROFILE_CHAR7_UUID)
};

// Characteristic 8 UUID: 0xFFF8
CONST uint8 simpleProfilechar8UUID[ATT_BT_UUID_SIZE] =
{ 
  LO_UINT16(SIMPLEPROFILE_CHAR8_UUID), HI_UINT16(SIMPLEPROFILE_CHAR8_UUID)
};

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

static simpleProfileCBs_t *simpleProfile_AppCBs = NULL;

/*********************************************************************
 * Profile Attributes - variables
 */

// Simple Profile Service attribute
static CONST gattAttrType_t simpleProfileService = { ATT_BT_UUID_SIZE, simpleProfileServUUID };

// Simple Profile Characteristic 4 Properties
static uint8 simpleProfileChar4Props = GATT_PROP_NOTIFY;

// Characteristic 4 Value
static uint8 simpleProfileChar4 = 0;

// Simple Profile Characteristic 4 Configuration Each client has its own
// instantiation of the Client Characteristic Configuration. Reads of the
// Client Characteristic Configuration only shows the configuration for
// that client and writes only affect the configuration of that client.
static gattCharCfg_t *simpleProfileChar4Config;
                                        
// Simple Profile Characteristic 4 User Description
static uint8 simpleProfileChar4UserDesp[17] = "Characteristic 4";

// Simple Profile Characteristic 6 Properties  PPG
static uint8 simpleProfileChar6Props = GATT_PROP_NOTIFY;
// Characteristic 6 Value
static uint8 simpleProfileChar6[SIMPLEPROFILE_CHAR6_LEN] = { 0, 1, 2 };
static gattCharCfg_t *simpleProfileChar6Config;
// Simple Profile Characteristic 6 User Description
static uint8 simpleProfileChar6UserDesp[] = "PPG";


// Simple Profile Characteristic 7 Properties  PCG
static uint8 simpleProfileChar7Props = GATT_PROP_NOTIFY;
// Characteristic 7 Value
static uint8 simpleProfileChar7[SIMPLEPROFILE_CHAR7_LEN] = { 0, 1, 2 };
static gattCharCfg_t *simpleProfileChar7Config;
// Simple Profile Characteristic 6 User Description
static uint8 simpleProfileChar7UserDesp[] = "PCG";


// Simple Profile Characteristic 8 Properties  ECG
static uint8 simpleProfileChar8Props = GATT_PROP_NOTIFY;
// Characteristic 8 Value
static uint8 simpleProfileChar8[SIMPLEPROFILE_CHAR8_LEN] = { 1, 0 };
static gattCharCfg_t *simpleProfileChar8Config;
// Simple Profile Characteristic 8 User Description
static uint8 simpleProfileChar8UserDesp[] = "PCG";

/*********************************************************************
 * Profile Attributes - Table
 */

static gattAttribute_t simpleProfileAttrTbl[SERVAPP_NUM_ATTR_SUPPORTED] = 
{
  // Simple Profile Service
  { 
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&simpleProfileService            /* pValue */
  },

  // Characteristic 4 Declaration
  { 
    { ATT_BT_UUID_SIZE, characterUUID },
    GATT_PERMIT_READ, 
    0,
    &simpleProfileChar4Props 
  },

  // Characteristic Value 4
  { 
    { ATT_BT_UUID_SIZE, simpleProfilechar4UUID },
    0, 
    0, 
    &simpleProfileChar4 
  },

  // Characteristic 4 configuration
  { 
    { ATT_BT_UUID_SIZE, clientCharCfgUUID },
    GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
    0, 
    (uint8 *)&simpleProfileChar4Config 
  },
  
  // Characteristic 4 User Description
  { 
    { ATT_BT_UUID_SIZE, charUserDescUUID },
    GATT_PERMIT_READ, 
    0, 
    simpleProfileChar4UserDesp 
  },
  
  // Characteristic 6 Declaration  PPG
  { 
    { ATT_BT_UUID_SIZE, characterUUID },
    GATT_PERMIT_READ, 
    0,
    &simpleProfileChar6Props 
  },
  // Characteristic Value 6
  { 
    { ATT_BT_UUID_SIZE, simpleProfilechar6UUID },
    0, 
    0, 
    simpleProfileChar6 
  },
  // Characteristic 6 configuration
  { 
    { ATT_BT_UUID_SIZE, clientCharCfgUUID },
    GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
    0, 
    (uint8 *)&simpleProfileChar6Config 
  }, 
  // Characteristic 6 User Description
  { 
    { ATT_BT_UUID_SIZE, charUserDescUUID },
    GATT_PERMIT_READ, 
    0, 
    simpleProfileChar6UserDesp 
  },
  
  
  // Characteristic 7 Declaration  PPG
  { 
    { ATT_BT_UUID_SIZE, characterUUID },
    GATT_PERMIT_READ, 
    0,
    &simpleProfileChar7Props 
  },
  // Characteristic Value 7
  { 
    { ATT_BT_UUID_SIZE, simpleProfilechar7UUID },
    0, 
    0, 
    simpleProfileChar7 
  },
  // Characteristic 7 configuration
  { 
    { ATT_BT_UUID_SIZE, clientCharCfgUUID },
    GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
    0, 
    (uint8 *)&simpleProfileChar7Config 
  }, 
  // Characteristic 7 User Description
  { 
    { ATT_BT_UUID_SIZE, charUserDescUUID },
    GATT_PERMIT_READ, 
    0, 
    simpleProfileChar7UserDesp 
  },    
  
  
  // Characteristic 8 Declaration  ECG
  { 
    { ATT_BT_UUID_SIZE, characterUUID },
    GATT_PERMIT_READ, 
    0,
    &simpleProfileChar8Props 
  },
  // Characteristic Value 8
  { 
    { ATT_BT_UUID_SIZE, simpleProfilechar8UUID },
    0, 
    0, 
    simpleProfileChar8 
  },
  // Characteristic 8 configuration
  { 
    { ATT_BT_UUID_SIZE, clientCharCfgUUID },
    GATT_PERMIT_READ | GATT_PERMIT_WRITE, 
    0, 
    (uint8 *)&simpleProfileChar8Config 
  }, 
  // Characteristic 8 User Description
  { 
    { ATT_BT_UUID_SIZE, charUserDescUUID },
    GATT_PERMIT_READ, 
    0, 
    simpleProfileChar8UserDesp 
  },

};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static bStatus_t simpleProfile_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                                           uint8 *pValue, uint8 *pLen, uint16 offset,
                                           uint8 maxLen, uint8 method );
static bStatus_t simpleProfile_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                            uint8 *pValue, uint8 len, uint16 offset,
                                            uint8 method );

/*********************************************************************
 * PROFILE CALLBACKS
 */
// Simple Profile Service Callbacks
CONST gattServiceCBs_t simpleProfileCBs =
{
  simpleProfile_ReadAttrCB,  // Read callback function pointer
  simpleProfile_WriteAttrCB, // Write callback function pointer
  NULL                       // Authorization callback function pointer
};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      SimpleProfile_AddService
 *
 * @brief   Initializes the Simple Profile service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 *
 * @return  Success or Failure
 */
bStatus_t SimpleProfile_AddService( uint32 services )
{
  uint8 status;
  
  // Allocate Client Characteristic Configuration table
  simpleProfileChar4Config = (gattCharCfg_t *)osal_mem_alloc( sizeof(gattCharCfg_t) *
                                                              linkDBNumConns );
  simpleProfileChar6Config = (gattCharCfg_t *)osal_mem_alloc( sizeof(gattCharCfg_t) *
                                                              linkDBNumConns );
  simpleProfileChar7Config = (gattCharCfg_t *)osal_mem_alloc( sizeof(gattCharCfg_t) *
                                                              linkDBNumConns );
  simpleProfileChar8Config = (gattCharCfg_t *)osal_mem_alloc( sizeof(gattCharCfg_t) *
                                                              linkDBNumConns );
  if ( simpleProfileChar4Config == NULL)
  {     
    return ( bleMemAllocError );
  }
  if ( simpleProfileChar6Config == NULL)
  {     
    return ( bleMemAllocError );
  }
  if ( simpleProfileChar7Config == NULL)
  {     
    return ( bleMemAllocError );
  }
  if ( simpleProfileChar8Config == NULL)
  {     
    return ( bleMemAllocError );
  }
  
  // Initialize Client Characteristic Configuration attributes
  GATTServApp_InitCharCfg( INVALID_CONNHANDLE, simpleProfileChar4Config );
  GATTServApp_InitCharCfg( INVALID_CONNHANDLE, simpleProfileChar6Config );
  GATTServApp_InitCharCfg( INVALID_CONNHANDLE, simpleProfileChar7Config );
  GATTServApp_InitCharCfg( INVALID_CONNHANDLE, simpleProfileChar8Config );
  
  if ( services & SIMPLEPROFILE_SERVICE )
  {
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService( simpleProfileAttrTbl, 
                                          GATT_NUM_ATTRS( simpleProfileAttrTbl ),
                                          GATT_MAX_ENCRYPT_KEY_SIZE,
                                          &simpleProfileCBs );
  }
  else
  {
    status = SUCCESS;
  }
  
  return ( status );
}

/*********************************************************************
 * @fn      SimpleProfile_RegisterAppCBs
 *
 * @brief   Registers the application callback function. Only call 
 *          this function once.
 *
 * @param   callbacks - pointer to application callbacks.
 *
 * @return  SUCCESS or bleAlreadyInRequestedMode
 */
bStatus_t SimpleProfile_RegisterAppCBs( simpleProfileCBs_t *appCallbacks )
{
  if ( appCallbacks )
  {
    simpleProfile_AppCBs = appCallbacks;
    
    return ( SUCCESS );
  }
  else
  {
    return ( bleAlreadyInRequestedMode );
  }
}

/*********************************************************************
 * @fn      SimpleProfile_SetParameter
 *
 * @brief   Set a Simple Profile parameter.
 *
 * @param   param - Profile parameter ID
 * @param   len - length of data to write
 * @param   value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t SimpleProfile_SetParameter( uint8 param, uint8 len, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
    case SIMPLEPROFILE_CHAR4:
      if ( len == sizeof ( uint8 ) ) 
      {
        simpleProfileChar4 = *((uint8*)value);
        
        // See if Notification has been enabled
        GATTServApp_ProcessCharCfg( simpleProfileChar4Config, &simpleProfileChar4, FALSE,
                                    simpleProfileAttrTbl, GATT_NUM_ATTRS( simpleProfileAttrTbl ),
                                    INVALID_TASK_ID, simpleProfile_ReadAttrCB );
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;
      
    case SIMPLEPROFILE_CHAR6:
      if ( len == SIMPLEPROFILE_CHAR6_LEN ) 
      {
        VOID osal_memcpy( simpleProfileChar6, value, SIMPLEPROFILE_CHAR6_LEN );
        // See if Notification has been enabled
        GATTServApp_ProcessCharCfg( simpleProfileChar6Config, simpleProfileChar6, FALSE,
                                    simpleProfileAttrTbl, GATT_NUM_ATTRS( simpleProfileAttrTbl ),
                                    INVALID_TASK_ID, simpleProfile_ReadAttrCB );
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;
      
    case SIMPLEPROFILE_CHAR7:
      if ( len == SIMPLEPROFILE_CHAR7_LEN ) 
      {
        VOID osal_memcpy( simpleProfileChar7, value, SIMPLEPROFILE_CHAR7_LEN );
        // See if Notification has been enabled
        GATTServApp_ProcessCharCfg( simpleProfileChar7Config, simpleProfileChar7, FALSE,
                                    simpleProfileAttrTbl, GATT_NUM_ATTRS( simpleProfileAttrTbl ),
                                    INVALID_TASK_ID, simpleProfile_ReadAttrCB );
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;
    case SIMPLEPROFILE_CHAR8:
      if ( len == SIMPLEPROFILE_CHAR8_LEN ) 
      {
        VOID osal_memcpy( simpleProfileChar8, value, SIMPLEPROFILE_CHAR8_LEN );
        // See if Notification has been enabled
        GATTServApp_ProcessCharCfg( simpleProfileChar8Config, simpleProfileChar8, FALSE,
                                    simpleProfileAttrTbl, GATT_NUM_ATTRS( simpleProfileAttrTbl ),
                                    INVALID_TASK_ID, simpleProfile_ReadAttrCB );
      }
      else
      {
        ret = bleInvalidRange;
      }
      break;
      
    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

/*********************************************************************
 * @fn      SimpleProfile_GetParameter
 *
 * @brief   Get a Simple Profile parameter.
 *
 * @param   param - Profile parameter ID
 * @param   value - pointer to data to put.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 *
 * @return  bStatus_t
 */
bStatus_t SimpleProfile_GetParameter( uint8 param, void *value )
{
  bStatus_t ret = SUCCESS;
  switch ( param )
  {
    case SIMPLEPROFILE_CHAR4:
      *((uint8*)value) = simpleProfileChar4;
      break;   
      
//    case SIMPLEPROFILE_CHAR6:
//      VOID memcpy( value, simpleProfileChar6, SIMPLEPROFILE_CHAR6_LEN );
//      break; 
    
    case SIMPLEPROFILE_CHAR7:
      VOID memcpy( value, simpleProfileChar7, SIMPLEPROFILE_CHAR7_LEN );
      break; 
      
    case SIMPLEPROFILE_CHAR8:
      VOID memcpy( value, simpleProfileChar8, SIMPLEPROFILE_CHAR8_LEN );
      break; 
    
    default:
      ret = INVALIDPARAMETER;
      break;
  }
  
  return ( ret );
}

/*********************************************************************
 * @fn          simpleProfile_ReadAttrCB
 *
 * @brief       Read an attribute.
 *
 * @param       connHandle - connection message was received on
 * @param       pAttr - pointer to attribute
 * @param       pValue - pointer to data to be read
 * @param       pLen - length of data to be read
 * @param       offset - offset of the first octet to be read
 * @param       maxLen - maximum length of data to be read
 * @param       method - type of read message
 *
 * @return      SUCCESS, blePending or Failure
 */
static bStatus_t simpleProfile_ReadAttrCB( uint16 connHandle, gattAttribute_t *pAttr, 
                                           uint8 *pValue, uint8 *pLen, uint16 offset,
                                           uint8 maxLen, uint8 method )
{
  bStatus_t status = SUCCESS;

  // If attribute permissions require authorization to read, return error
  if ( gattPermitAuthorRead( pAttr->permissions ) )
  {
    // Insufficient authorization
    return ( ATT_ERR_INSUFFICIENT_AUTHOR );
  }
  
  // Make sure it's not a blob operation (no attributes in the profile are long)
  if ( offset > 0 )
  {
    return ( ATT_ERR_ATTR_NOT_LONG );
  }
 
  if ( pAttr->type.len == ATT_BT_UUID_SIZE )
  {
    // 16-bit UUID
    uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
    switch ( uuid )
    {
      // No need for "GATT_SERVICE_UUID" or "GATT_CLIENT_CHAR_CFG_UUID" cases;
      // gattserverapp handles those reads

      // characteristics 1 and 2 have read permissions
      // characteritisc 3 does not have read permissions; therefore it is not
      //   included here
      // characteristic 4 does not have read permissions, but because it
      //   can be sent as a notification, it is included here
//      case SIMPLEPROFILE_CHAR1_UUID:
//      case SIMPLEPROFILE_CHAR2_UUID:
      case SIMPLEPROFILE_CHAR4_UUID:
        *pLen = 1;
        pValue[0] = *pAttr->pValue;
        break;

//      case SIMPLEPROFILE_CHAR5_UUID:
//        *pLen = SIMPLEPROFILE_CHAR5_LEN;
//        VOID memcpy( pValue, pAttr->pValue, SIMPLEPROFILE_CHAR5_LEN );
//        break;
        
      case SIMPLEPROFILE_CHAR6_UUID:
        *pLen = SIMPLEPROFILE_CHAR6_LEN;
        VOID memcpy( pValue, pAttr->pValue, SIMPLEPROFILE_CHAR6_LEN );
        break;
        
      case SIMPLEPROFILE_CHAR7_UUID:
        *pLen = SIMPLEPROFILE_CHAR7_LEN;
        VOID memcpy( pValue, pAttr->pValue, SIMPLEPROFILE_CHAR7_LEN );
        break;
      
      case SIMPLEPROFILE_CHAR8_UUID:
        *pLen = SIMPLEPROFILE_CHAR8_LEN;
        VOID memcpy( pValue, pAttr->pValue, SIMPLEPROFILE_CHAR8_LEN );
        break;
        
      default:
        // Should never get here! (characteristics 3 and 4 do not have read permissions)
        *pLen = 0;
        status = ATT_ERR_ATTR_NOT_FOUND;
        break;
    }
  }
  else
  {
    // 128-bit UUID
    *pLen = 0;
    status = ATT_ERR_INVALID_HANDLE;
  }

  return ( status );
}

/*********************************************************************
 * @fn      simpleProfile_WriteAttrCB
 *
 * @brief   Validate attribute data prior to a write operation
 *
 * @param   connHandle - connection message was received on
 * @param   pAttr - pointer to attribute
 * @param   pValue - pointer to data to be written
 * @param   len - length of data
 * @param   offset - offset of the first octet to be written
 * @param   method - type of write message
 *
 * @return  SUCCESS, blePending or Failure
 */
static bStatus_t simpleProfile_WriteAttrCB( uint16 connHandle, gattAttribute_t *pAttr,
                                            uint8 *pValue, uint8 len, uint16 offset,
                                            uint8 method )
{
  bStatus_t status = SUCCESS;
  uint8 notifyApp = 0xFF;
  
  // If attribute permissions require authorization to write, return error
  if ( gattPermitAuthorWrite( pAttr->permissions ) )
  {
    // Insufficient authorization
    return ( ATT_ERR_INSUFFICIENT_AUTHOR );
  }
  
  if ( pAttr->type.len == ATT_BT_UUID_SIZE )
  {
    // 16-bit UUID
    uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
    switch ( uuid )
    {
        
      case SIMPLEPROFILE_CHAR6_UUID:

        //Validate the value
        // Make sure it's not a blob oper
        if ( offset == 0 )
        {
          if ( len != SIMPLEPROFILE_CHAR6_LEN )
          {
            status = ATT_ERR_INVALID_VALUE_SIZE;
          }
        }
        else
        {
          status = ATT_ERR_ATTR_NOT_LONG;
        }
        
        //Write the value
        if ( status == SUCCESS )
        {
            VOID memcpy( pValue, pAttr->pValue, SIMPLEPROFILE_CHAR6_LEN );
            notifyApp = SIMPLEPROFILE_CHAR6;        
        }
             
        break;
        
      /////////////////////////////////////////////
      case SIMPLEPROFILE_CHAR7_UUID:
        if ( offset == 0 )
        {
          if ( len != SIMPLEPROFILE_CHAR7_LEN )
          {
            status = ATT_ERR_INVALID_VALUE_SIZE;
          }
        }
        else
        {
          status = ATT_ERR_ATTR_NOT_LONG;
        }
        if ( status == SUCCESS )
        {
            VOID memcpy( pValue, pAttr->pValue, SIMPLEPROFILE_CHAR7_LEN );
            notifyApp = SIMPLEPROFILE_CHAR7;        
        }            
        break;
      /////////////////////////////////////////////
      case SIMPLEPROFILE_CHAR8_UUID:

        //Validate the value
        // Make sure it's not a blob oper
        if ( offset == 0 )
        {
          if ( len != SIMPLEPROFILE_CHAR8_LEN )
          {
            status = ATT_ERR_INVALID_VALUE_SIZE;
          }
        }
        else
        {
          status = ATT_ERR_ATTR_NOT_LONG;
        }
        
        //Write the value
        if ( status == SUCCESS )
        {
            VOID memcpy( pValue, pAttr->pValue, SIMPLEPROFILE_CHAR8_LEN );
            notifyApp = SIMPLEPROFILE_CHAR8;        
        }
             
        break;

      case GATT_CLIENT_CHAR_CFG_UUID:
        status = GATTServApp_ProcessCCCWriteReq( connHandle, pAttr, pValue, len,
                                                 offset, GATT_CLIENT_CFG_NOTIFY );
        break;
        
      default:
        // Should never get here! (characteristics 2 and 4 do not have write permissions)
        status = ATT_ERR_ATTR_NOT_FOUND;
        break;
    }
  }
  else
  {
    // 128-bit UUID
    status = ATT_ERR_INVALID_HANDLE;
  }

  // If a charactersitic value changed then callback function to notify application of change
  if ( (notifyApp != 0xFF ) && simpleProfile_AppCBs && simpleProfile_AppCBs->pfnSimpleProfileChange )
  {
    simpleProfile_AppCBs->pfnSimpleProfileChange( notifyApp );  
  }
  
  return ( status );
}

/*********************************************************************
*********************************************************************/
