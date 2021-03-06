/**************************************************************************************************
  Filename:       simpleGATTprofile.h
  Revised:        $Date: 2010-08-06 08:56:11 -0700 (Fri, 06 Aug 2010) $
  Revision:       $Revision: 23333 $

  Description:    This file contains the Simple GATT profile definitions and
                  prototypes.
**************************************************************************************************/

#ifndef SIMPLEGATTPROFILE_H
#define SIMPLEGATTPROFILE_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * CONSTANTS
 */

// Profile Parameters
//#define SIMPLEPROFILE_CHAR1                   0  // RW uint8 - Profile Characteristic 1 value 
//#define SIMPLEPROFILE_CHAR2                   1  // RW uint8 - Profile Characteristic 2 value
//#define SIMPLEPROFILE_CHAR3                   2  // RW uint8 - Profile Characteristic 3 value
#define SIMPLEPROFILE_CHAR4                   3  // RW uint8 - Profile Characteristic 4 value
//#define SIMPLEPROFILE_CHAR5                   4  // RW uint8 - Profile Characteristic 5 value
#define SIMPLEPROFILE_CHAR6                   5  // RW uint8 - Profile Characteristic 6 value
#define SIMPLEPROFILE_CHAR7                   6  // RW uint8 - Profile Characteristic 7 value
#define SIMPLEPROFILE_CHAR8                   7  // RW uint8 - Profile Characteristic 8 value
  
  
// Simple Profile Service UUID
#define SIMPLEPROFILE_SERV_UUID               0xFFF0
    
// Key Pressed UUID
//#define SIMPLEPROFILE_CHAR1_UUID            0xFFF1
//#define SIMPLEPROFILE_CHAR2_UUID            0xFFF2
//#define SIMPLEPROFILE_CHAR3_UUID            0xFFF3
#define SIMPLEPROFILE_CHAR4_UUID            0xFFF4
//#define SIMPLEPROFILE_CHAR5_UUID            0xFFF5      
#define SIMPLEPROFILE_CHAR6_UUID            0xFFF6      //PulseSensor
#define SIMPLEPROFILE_CHAR7_UUID            0xFFF7      //ECG
#define SIMPLEPROFILE_CHAR8_UUID            0xFFF8      //PCG
  
// Simple Keys Profile Services bit fields
#define SIMPLEPROFILE_SERVICE               0x00000001

// Length of Characteristic 5 in bytes
//#define SIMPLEPROFILE_CHAR5_LEN           5 
#define SIMPLEPROFILE_CHAR6_LEN           20
#define SIMPLEPROFILE_CHAR7_LEN           20    
#define SIMPLEPROFILE_CHAR8_LEN           16 
   
/*********************************************************************
 * TYPEDEFS
 */
// GATT Maximum number of connections (including loopback)
#define GATT_MAX_NUM_CONN ( MAX_NUM_LL_CONN + 1 )

#if !defined ( MAX_NUM_LL_CONN )
       #if ( CTRL_CONFIG & INIT_CFG )
             #define MAX_NUM_LL_CONN 3
       #elif ( !( CTRL_CONFIG & INIT_CFG ) && ( CTRL_CONFIG & ADV_CONN_CFG ) )
             #define MAX_NUM_LL_CONN 1
       #else // no connection needed
             #define MAX_NUM_LL_CONN 0
       #endif // CTRL_CONFIG=INIT_CFG
#endif // !MAX_NUM_LL_CONN
  
/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * Profile Callbacks
 */

// Callback when a characteristic value has changed
typedef void (*simpleProfileChange_t)( uint8 paramID );

typedef struct
{
  simpleProfileChange_t        pfnSimpleProfileChange;  // Called when characteristic value changes
} simpleProfileCBs_t;

    

/*********************************************************************
 * API FUNCTIONS 
 */


/*
 * SimpleProfile_AddService- Initializes the Simple GATT Profile service by registering
 *          GATT attributes with the GATT server.
 *
 * @param   services - services to add. This is a bit map and can
 *                     contain more than one service.
 */

extern bStatus_t SimpleProfile_AddService( uint32 services );

/*
 * SimpleProfile_RegisterAppCBs - Registers the application callback function.
 *                    Only call this function once.
 *
 *    appCallbacks - pointer to application callbacks.
 */
extern bStatus_t SimpleProfile_RegisterAppCBs( simpleProfileCBs_t *appCallbacks );

/*
 * SimpleProfile_SetParameter - Set a Simple GATT Profile parameter.
 *
 *    param - Profile parameter ID
 *    len - length of data to right
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 */
extern bStatus_t SimpleProfile_SetParameter( uint8 param, uint8 len, void *value );
  
/*
 * SimpleProfile_GetParameter - Get a Simple GATT Profile parameter.
 *
 *    param - Profile parameter ID
 *    value - pointer to data to write.  This is dependent on
 *          the parameter ID and WILL be cast to the appropriate 
 *          data type (example: data type of uint16 will be cast to 
 *          uint16 pointer).
 */
extern bStatus_t SimpleProfile_GetParameter( uint8 param, void *value );


/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* SIMPLEGATTPROFILE_H */
