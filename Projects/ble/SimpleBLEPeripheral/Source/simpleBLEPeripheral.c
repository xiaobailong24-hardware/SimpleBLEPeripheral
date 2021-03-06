/*********************************************************************
 * INCLUDES
 */

#include "bcomdef.h"
#include "OSAL.h"
#include "OSAL_PwrMgr.h"

#include "OnBoard.h"
#include "hal_adc.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_lcd.h"
#include "hal_uart.h"
#include<ioCC2540.h>
#include "gatt.h"

#include "hci.h"

#include "gapgattserver.h"
#include "gattservapp.h"
#include "devinfoservice.h"
#include "simpleGATTprofile.h"
#include "serial.h"
#include "osal_snv.h"
#include "npi.h"

 
#if defined( CC2540_MINIDK )
  #include "simplekeys.h"
#endif

#include "peripheral.h"

#include "gapbondmgr.h"

#include "simpleBLEPeripheral.h"

#if defined FEATURE_OAD
  #include "oad.h"
  #include "oad_target.h"
#endif

/*********************************************************************
 * CONSTANTS
 */

// How often to perform periodic event
   
#define SBP_PPG_PERI_EVT_PERIOD                   10
#define SBP_PCG_PERI_EVT_PERIOD                   10

// What is the advertising interval when device is discoverable (units of 625us, 160=100ms)
#define DEFAULT_ADVERTISING_INTERVAL          160

// Limited discoverable mode advertises for 30.72s, and then stops
// General discoverable mode advertises indefinitely

#if defined ( CC2540_MINIDK )
#define DEFAULT_DISCOVERABLE_MODE             GAP_ADTYPE_FLAGS_LIMITED
#else
#define DEFAULT_DISCOVERABLE_MODE             GAP_ADTYPE_FLAGS_GENERAL
#endif  // defined ( CC2540_MINIDK )

// Minimum connection interval (units of 1.25ms, 80=100ms) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_MIN_CONN_INTERVAL     80

// Maximum connection interval (units of 1.25ms, 800=1000ms) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_MAX_CONN_INTERVAL     800

// Slave latency to use if automatic parameter update request is enabled
#define DEFAULT_DESIRED_SLAVE_LATENCY         0

// Supervision timeout value (units of 10ms, 1000=10s) if automatic parameter update request is enabled
#define DEFAULT_DESIRED_CONN_TIMEOUT          1000

// Whether to enable automatic parameter update request when a connection is formed
#define DEFAULT_ENABLE_UPDATE_REQUEST         TRUE

// Connection Pause Peripheral time value (in seconds)
#define DEFAULT_CONN_PAUSE_PERIPHERAL         6

// Company Identifier: Texas Instruments Inc. (13)
#define TI_COMPANY_ID                         0x000D

#define INVALID_CONNHANDLE                    0xFFFF

// Length of bd addr as a string
#define B_ADDR_STR_LEN                        15

/*********************************************************************
 * GLOBAL VARIABLES
 */
//PPG
uint8 charValue6[SIMPLEPROFILE_CHAR6_LEN] = {0,1,2,3,4,5,6,7,8,9,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0,1,2,3};
uint8 charValue6Notify[SIMPLEPROFILE_CHAR6_LEN] = {0,1,2,3,4,5,6,7,8,9,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13};
//PCG
uint8 charValue7[SIMPLEPROFILE_CHAR7_LEN] = {0,1,2,3,4,5,6,7,8,9,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0,1,2,3};
uint8 charValue7Notify[SIMPLEPROFILE_CHAR7_LEN] = {0,1,2,3,4,5,6,7,8,9,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13};
/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */


static uint8 simpleBLEPeripheral_TaskID;   // Task ID for internal task/event processing

static gaprole_States_t gapProfileState = GAPROLE_INIT;

// GAP - SCAN RSP data (max size = 31 bytes)
static uint8 scanRspData[] =
{
  // complete name
  0x0A,   // length of this data
  GAP_ADTYPE_LOCAL_NAME_COMPLETE,
  0x43,   // 'S'        C
  0x43,   // 'i'        C
  0x32,   // 'm'        2
  0x35,   // 'p'        5
  0x34,   // 'l'        4
  0x30,   // 'e'        0
  0x42,   // 'B'
  0x4C,   // 'L'
  0x45,   // 'E'


  // connection interval range
  0x05,   // length of this data
  GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE,
  LO_UINT16( DEFAULT_DESIRED_MIN_CONN_INTERVAL ),   // 100ms
  HI_UINT16( DEFAULT_DESIRED_MIN_CONN_INTERVAL ),
  LO_UINT16( DEFAULT_DESIRED_MAX_CONN_INTERVAL ),   // 1s
  HI_UINT16( DEFAULT_DESIRED_MAX_CONN_INTERVAL ),

  // Tx power level
  0x02,   // length of this data
  GAP_ADTYPE_POWER_LEVEL,
  0       // 0dBm
};

// GAP - Advertisement data (max size = 31 bytes, though this is
// best kept short to conserve power while advertisting)
static uint8 advertData[] =
{
  // Flags; this sets the device to use limited discoverable
  // mode (advertises for 30 seconds at a time) instead of general
  // discoverable mode (advertises indefinitely)
  0x02,   // length of this data                                        //0x02
  GAP_ADTYPE_FLAGS,                                                     //0x01
  DEFAULT_DISCOVERABLE_MODE | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED,     //0x05
                                                                            
  // service UUID, to notify central devices what services are included
  // in this peripheral
  0x03,   // length of this data                                        //0x03
  GAP_ADTYPE_16BIT_MORE,      // some of the UUID's, but not all        //0x02
  LO_UINT16( SIMPLEPROFILE_SERV_UUID ),                                 //0xF0
  HI_UINT16( SIMPLEPROFILE_SERV_UUID ),                                 //0xFF

};

// GAP GATT Attributes
static uint8 attDeviceName[GAP_DEVICE_NAME_LEN] = "Simple BLE Peripheral";

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void simpleBLEPeripheral_ProcessOSALMsg( osal_event_hdr_t *pMsg );
static void simpleBLEPeripheral_ProcessGATTMsg( gattMsgEvent_t *pMsg );
static void peripheralStateNotificationCB( gaprole_States_t newState );
static void ppgPeriodicTask( void );
static void ppgNotify( void );
static void pcgPeriodicTask( void );
static void pcgNotify( void );
static void simpleProfileChangeCB( uint8 paramID );

#if defined( CC2540_MINIDK ) || (WEBEE_BOARD)
static void simpleBLEPeripheral_HandleKeys( uint8 shift, uint8 keys );
#endif

#if (defined HAL_LCD) && (HAL_LCD == TRUE)
static char *bdAddr2Str ( uint8 *pAddr );
#endif // (defined HAL_LCD) && (HAL_LCD == TRUE)


//串口接收回掉函数
static void NpiSerialCallback(uint8 port,uint8 events);


/*********************************************************************
 * PROFILE CALLBACKS
 */

// GAP Role Callbacks
static gapRolesCBs_t simpleBLEPeripheral_PeripheralCBs =
{
  peripheralStateNotificationCB,  // Profile State Change Callbacks
  NULL                            // When a valid RSSI is read from controller (not used by application)
};

// GAP Bond Manager Callbacks
static gapBondCBs_t simpleBLEPeripheral_BondMgrCBs =
{
  NULL,                     // Passcode callback (not used by application)
  NULL                      // Pairing / Bonding state Callback (not used by application)
};

// Simple GATT Profile Callbacks
static simpleProfileCBs_t simpleBLEPeripheral_SimpleProfileCBs =
{
  simpleProfileChangeCB    // Charactersitic value change callback
};
/*********************************************************************
 * PUBLIC FUNCTIONS
 */

void SimpleBLEPeripheral_Init( uint8 task_id )
{
  simpleBLEPeripheral_TaskID = task_id;

  //Register for all key events - This app will handle all key events
  RegisterForKeys(simpleBLEPeripheral_TaskID);
  
  // Setup the GAP
  VOID GAP_SetParamValue( TGAP_CONN_PAUSE_PERIPHERAL, DEFAULT_CONN_PAUSE_PERIPHERAL );
  
  // Setup the GAP Peripheral Role Profile
  {
    #if defined( CC2540_MINIDK )
      // For the CC2540DK-MINI keyfob, device doesn't start advertising until button is pressed
      uint8 initial_advertising_enable = FALSE;
    #else
      // For other hardware platforms, device starts advertising upon initialization
      uint8 initial_advertising_enable = TRUE;
    #endif

    // By setting this to zero, the device will go into the waiting state after
    // being discoverable for 30.72 second, and will not being advertising again
    // until the enabler is set back to TRUE
    uint16 gapRole_AdvertOffTime = 0;

    uint8 enable_update_request = DEFAULT_ENABLE_UPDATE_REQUEST;
    uint16 desired_min_interval = DEFAULT_DESIRED_MIN_CONN_INTERVAL;
    uint16 desired_max_interval = DEFAULT_DESIRED_MAX_CONN_INTERVAL;
    uint16 desired_slave_latency = DEFAULT_DESIRED_SLAVE_LATENCY;
    uint16 desired_conn_timeout = DEFAULT_DESIRED_CONN_TIMEOUT;
    
    //
    HalLedSet(HAL_LED_2,HAL_LED_MODE_OFF); //关LED2
    HalAdcSetReference(HAL_ADC_REF_AVDD);     //3.3V  

    // Set the GAP Role Parameters
    GAPRole_SetParameter( GAPROLE_ADVERT_ENABLED, sizeof( uint8 ), &initial_advertising_enable );
    GAPRole_SetParameter( GAPROLE_ADVERT_OFF_TIME, sizeof( uint16 ), &gapRole_AdvertOffTime );

    GAPRole_SetParameter( GAPROLE_SCAN_RSP_DATA, sizeof ( scanRspData ), scanRspData );
    GAPRole_SetParameter( GAPROLE_ADVERT_DATA, sizeof( advertData ), advertData );

    GAPRole_SetParameter( GAPROLE_PARAM_UPDATE_ENABLE, sizeof( uint8 ), &enable_update_request );
    GAPRole_SetParameter( GAPROLE_MIN_CONN_INTERVAL, sizeof( uint16 ), &desired_min_interval );
    GAPRole_SetParameter( GAPROLE_MAX_CONN_INTERVAL, sizeof( uint16 ), &desired_max_interval );
    GAPRole_SetParameter( GAPROLE_SLAVE_LATENCY, sizeof( uint16 ), &desired_slave_latency );
    GAPRole_SetParameter( GAPROLE_TIMEOUT_MULTIPLIER, sizeof( uint16 ), &desired_conn_timeout );
  }

  // Set the GAP Characteristics
  GGS_SetParameter( GGS_DEVICE_NAME_ATT, GAP_DEVICE_NAME_LEN, attDeviceName );

  // Set advertising interval
  {
    uint16 advInt = DEFAULT_ADVERTISING_INTERVAL;

    GAP_SetParamValue( TGAP_LIM_DISC_ADV_INT_MIN, advInt );
    GAP_SetParamValue( TGAP_LIM_DISC_ADV_INT_MAX, advInt );
    GAP_SetParamValue( TGAP_GEN_DISC_ADV_INT_MIN, advInt );
    GAP_SetParamValue( TGAP_GEN_DISC_ADV_INT_MAX, advInt );
  }

  // Setup the GAP Bond Manager
  {
    uint32 passkey = 0; // passkey "000000"
    uint8 pairMode = GAPBOND_PAIRING_MODE_WAIT_FOR_REQ;
    uint8 mitm = TRUE;
    uint8 ioCap = GAPBOND_IO_CAP_DISPLAY_ONLY;
    uint8 bonding = TRUE;
    GAPBondMgr_SetParameter( GAPBOND_DEFAULT_PASSCODE, sizeof ( uint32 ), &passkey );
    GAPBondMgr_SetParameter( GAPBOND_PAIRING_MODE, sizeof ( uint8 ), &pairMode );
    GAPBondMgr_SetParameter( GAPBOND_MITM_PROTECTION, sizeof ( uint8 ), &mitm );
    GAPBondMgr_SetParameter( GAPBOND_IO_CAPABILITIES, sizeof ( uint8 ), &ioCap );
    GAPBondMgr_SetParameter( GAPBOND_BONDING_ENABLED, sizeof ( uint8 ), &bonding );
  }

  // Initialize GATT attributes
  GGS_AddService( GATT_ALL_SERVICES );            // GAP
  GATTServApp_AddService( GATT_ALL_SERVICES );    // GATT attributes
  DevInfo_AddService();                           // Device Information Service
  SimpleProfile_AddService( GATT_ALL_SERVICES );  // Simple GATT Profile
#if defined FEATURE_OAD
  VOID OADTarget_AddService();                    // OAD Profile
#endif

  // Setup the SimpleProfile Characteristic Values
  {
    uint8 charValue4 = 4;
    SimpleProfile_SetParameter( SIMPLEPROFILE_CHAR4, sizeof ( uint8 ), &charValue4 );
  }


#if defined( CC2540_MINIDK )

  SK_AddService( GATT_ALL_SERVICES ); // Simple Keys Profile

  // Register for all key events - This app will handle all key events
  RegisterForKeys( simpleBLEPeripheral_TaskID );

  // makes sure LEDs are off
  HalLedSet( (HAL_LED_1 | HAL_LED_2), HAL_LED_MODE_OFF );

  // For keyfob board set GPIO pins into a power-optimized state
  // Note that there is still some leakage current from the buzzer,
  // accelerometer, LEDs, and buttons on the PCB.

  P0SEL = 0; // Configure Port 0 as GPIO
  P1SEL = 0; // Configure Port 1 as GPIO
  P2SEL = 0; // Configure Port 2 as GPIO

  P0DIR = 0xFC; // Port 0 pins P0.0 and P0.1 as input (buttons),
                // all others (P0.2-P0.7) as output
  P1DIR = 0xFF; // All port 1 pins (P1.0-P1.7) as output
  P2DIR = 0x1F; // All port 1 pins (P2.0-P2.4) as output

  P0 = 0x03; // All pins on port 0 to low except for P0.0 and P0.1 (buttons)
  P1 = 0;   // All pins on port 1 to low
  P2 = 0;   // All pins on port 2 to low

#endif // #if defined( CC2540_MINIDK )

#if (defined HAL_LCD) && (HAL_LCD == TRUE)

#if defined FEATURE_OAD
  #if defined (HAL_IMAGE_A)
    HalLcdWriteStringValue( "BLE Peri-A", OAD_VER_NUM( _imgHdr.ver ), 16, HAL_LCD_LINE_1 );
  #else
    HalLcdWriteStringValue( "BLE Peri-B", OAD_VER_NUM( _imgHdr.ver ), 16, HAL_LCD_LINE_1 );
  #endif // HAL_IMAGE_A
#else
  HalLcdWriteString( "BLE Peripheral", HAL_LCD_LINE_1 );
#endif // FEATURE_OAD

#endif // (defined HAL_LCD) && (HAL_LCD == TRUE)

  // Register callback with SimpleGATTprofile
  VOID SimpleProfile_RegisterAppCBs( &simpleBLEPeripheral_SimpleProfileCBs );

  // Enable clock divide on halt
  // This reduces active current while radio is active and CC254x MCU
  // is halted
  HCI_EXT_ClkDivOnHaltCmd( HCI_EXT_ENABLE_CLK_DIVIDE_ON_HALT );

#if defined ( DC_DC_P0_7 )

  // Enable stack to toggle bypass control on TPS62730 (DC/DC converter)
  HCI_EXT_MapPmIoPortCmd( HCI_EXT_PM_IO_PORT_P0, HCI_EXT_PM_IO_PORT_PIN7 );

#endif // defined ( DC_DC_P0_7 )

  // Setup a delayed profile startup
  osal_set_event( simpleBLEPeripheral_TaskID, SBP_START_DEVICE_EVT );

}


uint16 SimpleBLEPeripheral_ProcessEvent( uint8 task_id, uint16 events )
{

  VOID task_id; // OSAL required parameter that isn't used in this function
  if ( events & SYS_EVENT_MSG )
  {
    uint8 *pMsg;

    if ( (pMsg = osal_msg_receive( simpleBLEPeripheral_TaskID )) != NULL )
    {
      simpleBLEPeripheral_ProcessOSALMsg( (osal_event_hdr_t *)pMsg );

      // Release the OSAL message
      VOID osal_msg_deallocate( pMsg );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }
  
  //开机事件
  if ( events & SBP_START_DEVICE_EVT )
  {
    // Start the Device
    VOID GAPRole_StartDevice( &simpleBLEPeripheral_PeripheralCBs );
    
  
    // Start Bond Manager
    VOID GAPBondMgr_Register( &simpleBLEPeripheral_BondMgrCBs );

//    // Set timer for first periodic event
//    osal_start_timerEx( simpleBLEPeripheral_TaskID, SBP_PERIODIC_EVT, SBP_PERIODIC_EVT_PERIOD );
//
//    //Npi Uart Init
//    NPI_InitTransport(NpiSerialCallback);

    return ( events ^ SBP_START_DEVICE_EVT );
  }
  
  //PPG周期事件
  if ( events & SBP_PPG_PERI_EVT )
  {
    static int speNumber = 0;
    // Restart timer
    if ( SBP_PPG_PERI_EVT_PERIOD )
    {
      osal_start_timerEx( simpleBLEPeripheral_TaskID, SBP_PPG_PERI_EVT, SBP_PPG_PERI_EVT_PERIOD );
    }
    if(speNumber++ <= 2000){    //采集20秒,40x1500
        ppgPeriodicTask();  //40ms采集一次脉搏  
    }
    else {      
      if((speNumber++)%200 == 0){
          ppgNotify();//1000ms发送一次脉搏    
      }
    }
    return (events ^ SBP_PPG_PERI_EVT);
  }
  
   //PCG周期事件
  if ( events & SBP_PCG_PERI_EVT )
  {
    static int pcgNumber = 0;
    // Restart timer
    if ( SBP_PCG_PERI_EVT_PERIOD )
    {
      osal_start_timerEx( simpleBLEPeripheral_TaskID, SBP_PCG_PERI_EVT, SBP_PCG_PERI_EVT_PERIOD );
    }
    if(pcgNumber++ <= 2000){    //采集10秒,10x1000
        pcgPeriodicTask();  //40ms采集一次心音 
    }
    else
    {      
      if((pcgNumber++)%200 == 0){
          pcgNotify();//4000ms发送一次心音
      }
    }
    return (events ^ SBP_PCG_PERI_EVT);
  }
  return 0;
}


static void simpleBLEPeripheral_ProcessOSALMsg( osal_event_hdr_t *pMsg )
{
  switch ( pMsg->event )
  {     
  #if defined( CC2540_MINIDK ) || (WEBEE_BOARD)
    case KEY_CHANGE:
      simpleBLEPeripheral_HandleKeys( ((keyChange_t *)pMsg)->state, 
                                      ((keyChange_t *)pMsg)->keys );
      break;
  #endif // #if defined( CC2540_MINIDK )
 
    case GATT_MSG_EVENT:
      // Process GATT message
      simpleBLEPeripheral_ProcessGATTMsg( (gattMsgEvent_t *)pMsg );
      break;
      
    default:
      // do nothing
      break;
  }
}

#if defined( CC2540_MINIDK ) || (WEBEE_BOARD)

static void simpleBLEPeripheral_HandleKeys( uint8 shift, uint8 keys )
{

  VOID shift;  // Intentionally unreferenced parameter

  if ( keys & HAL_KEY_SW_1 )    //S1按键
  {
//    NPI_WriteTransport("KEY K1\n",7);
//    SimpleProfile_SetParameter( SIMPLEPROFILE_CHAR6, SIMPLEPROFILE_CHAR6_LEN, charValue6Notify );
  }

  if ( keys & HAL_KEY_SW_2 )    //S2按键
  {
//    NPI_WriteTransport("KEY K2\n",7);
//    charValue6Notify[0] ++;
    // Set timer for first periodic event
    osal_start_timerEx( simpleBLEPeripheral_TaskID, SBP_PPG_PERI_EVT, SBP_PPG_PERI_EVT_PERIOD );
//    //Npi Uart Init
    NPI_InitTransport(NpiSerialCallback);
    HalLedSet(HAL_LED_2,HAL_LED_MODE_OFF); //关LED2
  }
}
#endif // #if defined( CC2540_MINIDK )


static void simpleBLEPeripheral_ProcessGATTMsg( gattMsgEvent_t *pMsg )
{  
  GATT_bm_free( &pMsg->msg, pMsg->method );
}

static void peripheralStateNotificationCB( gaprole_States_t newState )
{
#ifdef PLUS_BROADCASTER
  static uint8 first_conn_flag = 0;
#endif // PLUS_BROADCASTER
  
  
  switch ( newState )
  {
    case GAPROLE_STARTED:
      {
        uint8 ownAddress[B_ADDR_LEN];
        uint8 systemId[DEVINFO_SYSTEM_ID_LEN];

        GAPRole_GetParameter(GAPROLE_BD_ADDR, ownAddress);

        // use 6 bytes of device address for 8 bytes of system ID value
        systemId[0] = ownAddress[0];
        systemId[1] = ownAddress[1];
        systemId[2] = ownAddress[2];

        // set middle bytes to zero
        systemId[4] = 0x00;
        systemId[3] = 0x00;

        // shift three bytes up
        systemId[7] = ownAddress[5];
        systemId[6] = ownAddress[4];
        systemId[5] = ownAddress[3];

        DevInfo_SetParameter(DEVINFO_SYSTEM_ID, DEVINFO_SYSTEM_ID_LEN, systemId);

        #if (defined HAL_LCD) && (HAL_LCD == TRUE)
          // Display device address
          HalLcdWriteString( bdAddr2Str( ownAddress ),  HAL_LCD_LINE_2 );
          HalLcdWriteString( "Initialized",  HAL_LCD_LINE_3 );
        #endif // (defined HAL_LCD) && (HAL_LCD == TRUE)
      }
      break;

    case GAPROLE_ADVERTISING:
      {
        #if (defined HAL_LCD) && (HAL_LCD == TRUE)
          HalLcdWriteString( "Advertising",  HAL_LCD_LINE_3 );
        #endif // (defined HAL_LCD) && (HAL_LCD == TRUE)
        #if (defined HAL_LED) && (HAL_LED == TRUE)
          HalLedSet(HAL_LED_1,HAL_LED_MODE_FLASH);
        #endif
      }
      break;

#ifdef PLUS_BROADCASTER   
    /* After a connection is dropped a device in PLUS_BROADCASTER will continue
     * sending non-connectable advertisements and shall sending this change of 
     * state to the application.  These are then disabled here so that sending 
     * connectable advertisements can resume.
     */
    case GAPROLE_ADVERTISING_NONCONN:
      {
        uint8 advertEnabled = FALSE;
      
        // Disable non-connectable advertising.
        GAPRole_SetParameter(GAPROLE_ADV_NONCONN_ENABLED, sizeof(uint8),
                           &advertEnabled);
        
        // Reset flag for next connection.
        first_conn_flag = 0;
      }
      break;
#endif //PLUS_BROADCASTER         
      
    case GAPROLE_CONNECTED:
      {        
        #if (defined HAL_LCD) && (HAL_LCD == TRUE)
          HalLcdWriteString( "Connected",  HAL_LCD_LINE_3 );
        #endif // (defined HAL_LCD) && (HAL_LCD == TRUE)
        #if (defined HAL_LED) && (HAL_LED == TRUE)
          HalLedSet(HAL_LED_1,HAL_LED_MODE_OFF);
          HalLedSet(HAL_LED_3,HAL_LED_MODE_ON);          
        #endif
          
#ifdef PLUS_BROADCASTER
        // Only turn advertising on for this state when we first connect
        // otherwise, when we go from connected_advertising back to this state
        // we will be turning advertising back on.
        if ( first_conn_flag == 0 ) 
        {
            uint8 advertEnabled = FALSE; // Turn on Advertising

            // Disable connectable advertising.
            GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8),
                                 &advertEnabled);
            
            // Set to true for non-connectabel advertising.
            advertEnabled = TRUE;
            
            // Enable non-connectable advertising.
            GAPRole_SetParameter(GAPROLE_ADV_NONCONN_ENABLED, sizeof(uint8),
                                 &advertEnabled);
            
            first_conn_flag = 1;
        }
#endif // PLUS_BROADCASTER
      }
      break;

    case GAPROLE_CONNECTED_ADV:
      {
        #if (defined HAL_LCD) && (HAL_LCD == TRUE)
          HalLcdWriteString( "Connected Advertising",  HAL_LCD_LINE_3 );
        #endif // (defined HAL_LCD) && (HAL_LCD == TRUE)
      }
      break;      
    case GAPROLE_WAITING:
      {
        #if (defined HAL_LCD) && (HAL_LCD == TRUE)
          HalLcdWriteString( "Disconnected",  HAL_LCD_LINE_3 );
        #endif // (defined HAL_LCD) && (HAL_LCD == TRUE)
        #if (defined HAL_LED) && (HAL_LED == TRUE)
          HalLedSet(HAL_LED_3,HAL_LED_MODE_OFF);
        #endif
          
#ifdef PLUS_BROADCASTER                
        uint8 advertEnabled = TRUE;
      
        // Enabled connectable advertising.
        GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED, sizeof(uint8),
                             &advertEnabled);
#endif //PLUS_BROADCASTER
      }
      break;

    case GAPROLE_WAITING_AFTER_TIMEOUT:
      {
        #if (defined HAL_LCD) && (HAL_LCD == TRUE)
          HalLcdWriteString( "Timed Out",  HAL_LCD_LINE_3 );
        #endif // (defined HAL_LCD) && (HAL_LCD == TRUE)
          
#ifdef PLUS_BROADCASTER
        // Reset flag for next connection.
        first_conn_flag = 0;
#endif //#ifdef (PLUS_BROADCASTER)
      }
      break;

    case GAPROLE_ERROR:
      {
        #if (defined HAL_LCD) && (HAL_LCD == TRUE)
          HalLcdWriteString( "Error",  HAL_LCD_LINE_3 );
        #endif // (defined HAL_LCD) && (HAL_LCD == TRUE)
      }
      break;

    default:
      {
        #if (defined HAL_LCD) && (HAL_LCD == TRUE)
          HalLcdWriteString( "",  HAL_LCD_LINE_3 );
        #endif // (defined HAL_LCD) && (HAL_LCD == TRUE)
      }
      break;

  }

  gapProfileState = newState;

#if !defined( CC2540_MINIDK )
  VOID gapProfileState;     // added to prevent compiler warning with
                            // "CC2540 Slave" configurations
#endif


}

//PPG采集
static void ppgPeriodicTask( void ){
    static uint16 ppg = 0;
    static uint16 ppg_nv_write_id = 0;

    if(ppg < SIMPLEPROFILE_CHAR6_LEN){
	charValue6[ppg++] = HalAdcRead(HAL_ADC_CHANNEL_0,HAL_ADC_RESOLUTION_8)*2;     //IN0,P0.0   
    } else { 
	if(ppg_nv_write_id <= 60){  
	    uint8 wrStatus = osal_snv_write( BLE_NVID_PPG_START + ppg_nv_write_id, SIMPLEPROFILE_CHAR6_LEN, charValue6);    
	    if(SUCCESS == wrStatus) {
		SerialPrintf("PPG--Save \"%s\" to Snv ID %d success\r\n", charValue6, BLE_NVID_PPG_START + ppg_nv_write_id);
		ppg_nv_write_id++;
	    } else {
		NPI_WriteTransport("PPG--Save Failed\n",17);
		ppg_nv_write_id++;
	    }      
	    }else {
		HalLedSet(HAL_LED_1,HAL_LED_MODE_ON); //开LED1
	    }    
	ppg = 0;   
    }
}

//PPG发送
static void ppgNotify( void )     
{
    static uint16 ppg_nv_read_id = 0;
    if(ppg_nv_read_id <= 60){  
	uint8 reStatus = osal_snv_read( BLE_NVID_PPG_START + ppg_nv_read_id, SIMPLEPROFILE_CHAR6_LEN, charValue6Notify);    
	if(SUCCESS == reStatus)
	{
	    SimpleProfile_SetParameter( SIMPLEPROFILE_CHAR6, SIMPLEPROFILE_CHAR6_LEN, charValue6Notify );
	    SerialPrintf("Read \"%s\" to Snv ID %d success\r\n", charValue6Notify, BLE_NVID_PPG_START + ppg_nv_read_id);
	    ppg_nv_read_id++;
	}
	else
	{
	    NPI_WriteTransport("Read Failed\n",12);
	    ppg_nv_read_id++;
	}    
    }else{
	//PPG发送完成，开启PCG采集
	HalLedSet(HAL_LED_1,HAL_LED_MODE_OFF); //关LED1
	osal_start_timerEx( simpleBLEPeripheral_TaskID, SBP_PCG_PERI_EVT, SBP_PCG_PERI_EVT_PERIOD );
    }  
}

//PCG采集
static void pcgPeriodicTask( void )
{
    static uint16 pcg = 0;
    static uint16 pcg_nv_write_id = 0;

    if(pcg < SIMPLEPROFILE_CHAR7_LEN)
    {
	charValue7[pcg++] = HalAdcRead(HAL_ADC_CHANNEL_1,HAL_ADC_RESOLUTION_8)*3;     //IN1,P0.1
    }
    else 
    { 
	if(pcg_nv_write_id <= 60){  
	    uint8 wrStatus = osal_snv_write( BLE_NVID_PCG_START + pcg_nv_write_id, SIMPLEPROFILE_CHAR7_LEN, charValue7);    
	    if(SUCCESS == wrStatus) {
		SerialPrintf("PCG--Save \"%s\" to Snv ID %d success\r\n", charValue7, BLE_NVID_PCG_START + pcg_nv_write_id);
		pcg_nv_write_id++;
	    } else {
		NPI_WriteTransport("PCG--Save Failed\n",17);
		pcg_nv_write_id++;
	    }
	}else{
	    HalLedSet(HAL_LED_2,HAL_LED_MODE_ON); //开LED2
	}
    	pcg = 0;
    }
} 

//PCG发送
static void pcgNotify( void )     
{
    static uint16 pcg_nv_read_id = 0;
    if(pcg_nv_read_id <= 60){  
	uint8 reStatus = osal_snv_read( BLE_NVID_PCG_START + pcg_nv_read_id, SIMPLEPROFILE_CHAR7_LEN, charValue7Notify);    
	if(SUCCESS == reStatus)
	{
	    SimpleProfile_SetParameter( SIMPLEPROFILE_CHAR7, SIMPLEPROFILE_CHAR7_LEN, charValue7Notify );
	    SerialPrintf("Read \"%s\" to Snv ID %d success\r\n", charValue7Notify, BLE_NVID_PCG_START + pcg_nv_read_id);
	    pcg_nv_read_id++;
	}
	else
	{
	    NPI_WriteTransport("Read Failed\n",12);
	    pcg_nv_read_id++;
	}    
    }else{
	//PPG发送完成，开启PCG采集
	//  HalLedSet(HAL_LED_1,HAL_LED_MODE_OFF); //关LED1
	HalLedSet(HAL_LED_2,HAL_LED_MODE_OFF); //关LED2
	//Npi Uart Init
	//  NPI_InitTransport(NpiSerialCallback);
    } 
}

static void simpleProfileChangeCB( uint8 paramID )
{
//  uint8 newValue,newChar6Value[20];
  
  switch( paramID )
  {
//    case SIMPLEPROFILE_CHAR1:
//      SimpleProfile_GetParameter( SIMPLEPROFILE_CHAR1, &newValue );
//
//      #if (defined HAL_LCD) && (HAL_LCD == TRUE)
//        HalLcdWriteStringValue( "Char 1:", (uint16)(newValue), 10,  HAL_LCD_LINE_3 );
//      #endif // (defined HAL_LCD) && (HAL_LCD == TRUE)
//      //根据CHAR1的值改变LED状态
//      if(newValue == 0x31)
//      {
//          HalLedSet(HAL_LED_2,HAL_LED_MODE_ON); //开LED2
//      }
//      else
//      {
//          HalLedSet(HAL_LED_2,HAL_LED_MODE_OFF); //关LED2
//      }
//
//      break;
//
//    case SIMPLEPROFILE_CHAR3:
//      SimpleProfile_GetParameter( SIMPLEPROFILE_CHAR3, &newValue );
//
//      #if (defined HAL_LCD) && (HAL_LCD == TRUE)
//        HalLcdWriteStringValue( "Char 3:", (uint16)(newValue), 10,  HAL_LCD_LINE_3 );
//      #endif // (defined HAL_LCD) && (HAL_LCD == TRUE)
//
//      break;
    
    default:
      // should not reach here!
      break;
  }
}

#if (defined HAL_LCD) && (HAL_LCD == TRUE)

char *bdAddr2Str( uint8 *pAddr )
{
  uint8       i;
  char        hex[] = "0123456789ABCDEF";
  static char str[B_ADDR_STR_LEN];
  char        *pStr = str;

  *pStr++ = '0';
  *pStr++ = 'x';

  // Start from end of addr
  pAddr += B_ADDR_LEN;

  for ( i = B_ADDR_LEN; i > 0; i-- )
  {
    *pStr++ = hex[*--pAddr >> 4];
    *pStr++ = hex[*pAddr & 0x0F];
  }

  *pStr = 0;

  return str;
}
#endif // (defined HAL_LCD) && (HAL_LCD == TRUE)


//串口接收回掉函数(BMD101)
static void NpiSerialCallback(uint8 port,uint8 events)
{
    (void)port;
    uint8 numBytes = 0;
    uint8 buf[128]={0};    
    if(events & HAL_UART_RX_TIMEOUT)    //串口有数据
    {
      numBytes = NPI_RxBufLen();        //读出串口缓冲区有多少字节
      if(numBytes >= 16)
      {
        //从串口缓冲区读出numBytes字节数据
        NPI_ReadTransport(buf,numBytes);
        NPI_WriteTransport(buf,numBytes);          
        SimpleProfile_SetParameter( SIMPLEPROFILE_CHAR8, SIMPLEPROFILE_CHAR8_LEN, buf );     
      }

    }
}

/*********************************************************************
*********************************************************************/
