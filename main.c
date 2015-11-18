/* Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/** @file
 *
 * @defgroup ble_sdk_app_template_main main.c
 * @{
 * @ingroup ble_sdk_app_template
 * @brief Template project main file.
 *
 * This file contains a template for creating a new application. It has the code necessary to wakeup
 * from button, advertise, get a connection restart advertising on disconnect and if no new
 * connection created go back to system-off mode.
 * It can easily be used as a starting point for creating a new application, the comments identified
 * with 'YOUR_JOB' indicates where and how you can customize.
 */

#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "nrf_gpio.h"
#include "nrf51_bitfields.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_conn_params.h"
#include "boards.h"
#include "app_scheduler.h"
#include "softdevice_handler.h"
#include "app_timer.h"
#include "ble_error_log.h"
#include "app_gpiote.h"
#include "app_button.h"
#include "ble_debug_assert_handler.h"
#include "pstorage.h"
#include "ble_cis.h"
#include "ble_gap.h"

#define IS_SRVC_CHANGED_CHARACT_PRESENT 0                                           /**< Include or not the service_changed characteristic. if not enabled, the server's database cannot be changed for the lifetime of the device*/

#define WAKEUP_BUTTON_PIN               BUTTON_0                                    /**< Button used to wake up the application. */



#define ADVERTISING_LED_PIN_NO          LED_0                                       /**< Is on when device is advertising. */
#define CONNECTED_LED_PIN_NO            LED_1                                       /**< Is on when device has connected. */
#define ASSERT_LED_PIN_NO               LED_2                                       /**< Is on when application has asserted. */
#define DEBUG_LED_PIN_NO                LED_3                                       /**< Is on when application has asserted. */



#define LEDBUTTON_BUTTON_PIN_NO         BUTTON_1
#define DEVICE_NAME                     "Coffee"                           /**< Name of device. Will be included in the advertising data. */

#define APP_ADV_INTERVAL                64                                          /**< The advertising interval (in units of 0.625 ms. This value corresponds to 40 ms). */
#define APP_ADV_TIMEOUT_IN_SECONDS      180                                         /**< The advertising timeout (in units of seconds). */

#define APP_TIMER_PRESCALER             0                                           /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_MAX_TIMERS            8                                           /**< Maximum number of simultaneously created timers. */
#define APP_TIMER_OP_QUEUE_SIZE         8                                           /**< Size of timer operation queues. */

#define MIN_CONN_INTERVAL               MSEC_TO_UNITS(500, UNIT_1_25_MS)            /**< Minimum acceptable connection interval (0.5 seconds). */
#define MAX_CONN_INTERVAL               MSEC_TO_UNITS(1000, UNIT_1_25_MS)           /**< Maximum acceptable connection interval (1 second). */
#define SLAVE_LATENCY                   0                                           /**< Slave latency. */
#define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(4000, UNIT_10_MS)             /**< Connection supervisory timeout (4 seconds). */
#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER)  /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER) /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT    3                                           /**< Number of attempts before giving up the connection parameter negotiation. */

#define APP_GPIOTE_MAX_USERS            1                                           /**< Maximum number of users of the GPIOTE handler. */

#define BUTTON_DETECTION_DELAY          APP_TIMER_TICKS(50, APP_TIMER_PRESCALER)    /**< Delay from a GPIOTE event until a button is reported as pushed (in number of timer ticks). */

#define SEC_PARAM_TIMEOUT               30                                          /**< Timeout for Pairing Request or Security Request (in seconds). */
#define SEC_PARAM_BOND                  1                                           /**< Perform bonding. */
#define SEC_PARAM_MITM                  0                                           /**< Man In The Middle protection not required. */
#define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE                        /**< No I/O capabilities. */
#define SEC_PARAM_OOB                   0                                           /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE          7                                           /**< Minimum encryption key size. */
#define SEC_PARAM_MAX_KEY_SIZE          16                                          /**< Maximum encryption key size. */

#define DEAD_BEEF                       0xDEADBEEF                                  /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

#define COLLECT_DELAY                   APP_TIMER_TICKS(500, APP_TIMER_PRESCALER)

static ble_gap_sec_params_t             m_sec_params;                               /**< Security requirements for this application. */
static uint16_t                         m_conn_handle = BLE_CONN_HANDLE_INVALID;    /**< Handle of the current connection. */
static ble_cis_t                        m_cis;
static app_timer_id_t                   m_collect_1st_id;
static app_timer_id_t                   m_collect_2nd_id;
static app_timer_id_t                   m_collect_3rd_id;
static app_timer_id_t                   m_collect_4th_id;
static app_timer_id_t                   m_collect_5th_id;
static app_timer_id_t                   m_collect_6th_id;
static app_timer_id_t                   m_collect_7th_id;

#define SCHED_MAX_EVENT_DATA_SIZE       sizeof(app_timer_event_t)                   /**< Maximum size of scheduler events. Note that scheduler BLE stack events do not contain any data, as the events are being pulled from the stack in the event handler. */
#define SCHED_QUEUE_SIZE                10                                          /**< Maximum number of events in the scheduler queue. */
#define OFF     0
#define ON      1
#define INPUT   2
#define OUTPUT  3

enum{
    COLLECT_MENU = 0,
    COLLECT_LARGE_COFFEE,
    COLLECT_COFFEE,
    COLLECT_ESPRESSO,
    COLLECT_HOT_WATER,
    COLLECT_POWDER_COFFEE,
    COLLECT_CAPPUCCINO,
};

enum{
        MENU            =   3,
        LARGE_COFFEE,
        COFFEE,  
        ESPRESSO,
        HOT_WATER,
        POWDER_COFFEE   =   9,
        CAPPUCCINO      =   12,
};
// Persistent storage system event handler
void pstorage_sys_event_handler (uint32_t p_evt);

/**@brief Function for error handling, which is called when an error has occurred.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of error.
 *
 * @param[in] error_code  Error code supplied to the handler.
 * @param[in] line_num    Line number where the handler is called.
 * @param[in] p_file_name Pointer to the file name.
 */
void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name)
{
    // This call can be used for debug purposes during application development.
    // @note CAUTION: Activating this code will write the stack to flash on an error.
    //                This function should NOT be used in a final product.
    //                It is intended STRICTLY for development/debugging purposes.
    //                The flash write will happen EVEN if the radio is active, thus interrupting
    //                any communication.
    //                Use with care. Un-comment the line below to use.
    ble_debug_assert_handler(error_code, line_num, p_file_name);
    debug(0,"Error code = 0x%d, line_num is %d, file name is %s \n\r",error_code, line_num, p_file_name);
    // On assert, the system can only recover with a reset.
    //NVIC_SystemReset();
}


/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in]   line_num   Line number of the failing ASSERT call.
 * @param[in]   file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}
/**@brief Function for the GPIO control.
 *
 * @details Set all base state of GPIO.
 */
void control_gpio(uint32_t pin_no,uint8_t state,nrf_gpio_pin_pull_t pin_pull){
    switch(state)
    {
        case ON:
            nrf_gpio_pin_clear(pin_no);
            break;
        case OFF:
            nrf_gpio_pin_set(pin_no);
            break;
        case INPUT:
            nrf_gpio_cfg_input(pin_no,pin_pull);
            break;
        case OUTPUT:
            nrf_gpio_cfg_output(pin_no);
            break;
        default:
            debug(0,"Unknown state: %d",state); 
            break;
    }
}

/**@brief Function for the LEDs initialization.
 *
 * @details Initializes all LEDs used by the application.
 */
static void leds_init(void)
{
    debug(0,"Initializing led... \n\r");
    control_gpio(MENU,OUTPUT,0);
    control_gpio(LARGE_COFFEE,OUTPUT,0);
    control_gpio(COFFEE,OUTPUT,0);
    control_gpio(ESPRESSO,OUTPUT,0);
    control_gpio(HOT_WATER,OUTPUT,0);
    control_gpio(POWDER_COFFEE,OUTPUT,0);
    control_gpio(CAPPUCCINO,OUTPUT,0);
    
    // Set initializing state
    
    control_gpio(MENU,OFF,0);
    control_gpio(LARGE_COFFEE,OFF,0);
    control_gpio(COFFEE,OFF,0);
    control_gpio(ESPRESSO,OFF,0);
    control_gpio(HOT_WATER,OFF,0);
    control_gpio(POWDER_COFFEE,OFF,0);
    control_gpio(CAPPUCCINO,OFF,0);
}

/**@brief Function for handler operation when timer call.
 *
 * @details Off state of LED 1.
 */
static void collect_1st_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    control_gpio(MENU,OFF,0);
    debug(0,"1st done!\n\r");
}
/**@brief Function for handler operation when timer call.
 *
 * @details Off state of LED 2.
 */
static void collect_2nd_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    control_gpio(LARGE_COFFEE,OFF,0);
    debug(0,"2nd done!\n\r");
}
/**@brief Function for handler operation when timer call.
 *
 * @details Off state of LED 3.
 */
static void collect_3rd_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    control_gpio(COFFEE,OFF,0);
    debug(0,"3rd done!\n\r");
}
/**@brief Function for handler operation when timer call.
 *
 * @details Off state of LED 3.
 */
static void collect_4th_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    control_gpio(ESPRESSO,OFF,0);
    debug(0,"4th done!\n\r");
}
/**@brief Function for handler operation when timer call.
 *
 * @details Off state of LED 3.
 */
static void collect_5th_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    control_gpio(HOT_WATER,OFF,0);
    debug(0,"5th done!\n\r");
}
/**@brief Function for handler operation when timer call.
 *
 * @details Off state of LED 3.
 */
static void collect_6th_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    control_gpio(POWDER_COFFEE,OFF,0);
    debug(0,"6th done!\n\r");
}
/**@brief Function for handler operation when timer call.
 *
 * @details Off state of LED 3.
 */
static void collect_7th_handler(void * p_context)
{
    UNUSED_PARAMETER(p_context);
    control_gpio(CAPPUCCINO,OFF,0);
    debug(0,"7th done!\n\r");
}
/**@brief Function for starting timers.
*/
static void timers_start(void)
{
    debug(0,"Timer start \n\r");
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module.
 */
static void timers_init(void)
{
    // Initialize timer module, making it use the scheduler
    uint32_t                err_code;
    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_MAX_TIMERS, APP_TIMER_OP_QUEUE_SIZE, false);
    debug(0,"Initializing timer... \n\r");
        // Initialize timer module.
    // Create timer.
    err_code = app_timer_create(&m_collect_1st_id,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                collect_1st_handler);
    APP_ERROR_CHECK(err_code);
    
    err_code = app_timer_create(&m_collect_2nd_id,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                collect_2nd_handler);
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_collect_3rd_id,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                collect_3rd_handler);
    APP_ERROR_CHECK(err_code);
    err_code = app_timer_create(&m_collect_4th_id,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                collect_4th_handler);
    APP_ERROR_CHECK(err_code);
    err_code = app_timer_create(&m_collect_5th_id,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                collect_5th_handler);
    APP_ERROR_CHECK(err_code);
    err_code = app_timer_create(&m_collect_6th_id,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                collect_6th_handler);
    APP_ERROR_CHECK(err_code);
    err_code = app_timer_create(&m_collect_7th_id,
                                APP_TIMER_MODE_SINGLE_SHOT,
                                collect_7th_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
    debug(0,"Initializing GAP parameter... \n\r");

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Advertising functionality.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 */
static void advertising_init(void)
{
    uint32_t      err_code;
    ble_advdata_t advdata;
    ble_advdata_t scanrsp;
    uint8_t       flags = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;

    // YOUR_JOB: Use UUIDs for service(s) used in your application.
    ble_uuid_t adv_uuids[] = {{CIS_UUID_SERVICE, m_cis.uuid_type}};
    debug(0,"Initializing advertisement... \n\r");
    // Build and set advertising data
    memset(&advdata, 0, sizeof(advdata));

    advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance      = true;
    advdata.flags.size              = sizeof(flags);
    advdata.flags.p_data            = &flags;
    
    memset(&scanrsp, 0, sizeof(scanrsp));
    scanrsp.uuids_complete.uuid_cnt = sizeof(adv_uuids) / sizeof(adv_uuids[0]);
    scanrsp.uuids_complete.p_uuids  = adv_uuids;
    
    err_code = ble_advdata_set(&advdata, &scanrsp);
    APP_ERROR_CHECK(err_code);
}
/**@brief Function for handler operation when received data from devices.
 *
 * @details Control LED and start time out.
 */
static void led_write_handler(ble_cis_t * p_cis, uint8_t led_state)
{
    debug(0,"Led write handler, data received is %2d \n\r",led_state);
    switch(led_state)
    {
        case COLLECT_MENU:
            control_gpio(MENU,ON,0);
            app_timer_start(m_collect_1st_id,COLLECT_DELAY,NULL);
            debug(0,"Select 1st \n\r");
            break;
        case COLLECT_LARGE_COFFEE:
            control_gpio(LARGE_COFFEE,ON,0);
            app_timer_start(m_collect_2nd_id,COLLECT_DELAY,NULL);
            debug(0,"Select 2nd \n\r");
            break;
        case COLLECT_COFFEE:
            control_gpio(COFFEE,ON,0);
            app_timer_start(m_collect_3rd_id,COLLECT_DELAY,NULL);
            debug(0,"Select 3rd \n\r");
            break; 
        case COLLECT_ESPRESSO:
            control_gpio(ESPRESSO,ON,0);
            app_timer_start(m_collect_4th_id,COLLECT_DELAY,NULL);
            debug(0,"Select 4th \n\r");
            break;
        case COLLECT_HOT_WATER:
            control_gpio(HOT_WATER,ON,0);
            app_timer_start(m_collect_5th_id,COLLECT_DELAY,NULL);
            debug(0,"Select 5th \n\r");
            break;
        case COLLECT_POWDER_COFFEE:
            control_gpio(POWDER_COFFEE,ON,0);
            app_timer_start(m_collect_6th_id,COLLECT_DELAY,NULL);
            debug(0,"Select 6th \n\r");
            break;
         case COLLECT_CAPPUCCINO:
            control_gpio(CAPPUCCINO,ON,0);
            app_timer_start(m_collect_7th_id,COLLECT_DELAY,NULL);
            debug(0,"Select 7th \n\r");
            break;
        default:
            debug(0,"Wrong state : %d",led_state);
            break;
    }
}

/**@brief Function for initializing services that will be used by the application.
 */
static void services_init(void)
{
    uint32_t err_code;
    ble_cis_init_t init;
	
    init.led_write_handler = led_write_handler;
    debug(0,"Initializing service... \n\r");
    err_code = ble_cis_init(&m_cis, &init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing security parameters.
 */
static void sec_params_init(void)
{
    m_sec_params.bond         = SEC_PARAM_BOND;
    m_sec_params.mitm         = SEC_PARAM_MITM;
    m_sec_params.io_caps      = SEC_PARAM_IO_CAPABILITIES;
    m_sec_params.oob          = SEC_PARAM_OOB;
    m_sec_params.min_key_size = SEC_PARAM_MIN_KEY_SIZE;
    m_sec_params.max_key_size = SEC_PARAM_MAX_KEY_SIZE;
}


/**@brief Function for handling the Connection Parameters Module.
 *
 * @details This function will be called for all events in the Connection Parameters Module which
 *          are passed to the application.
 *          @note All this function does is to disconnect. This could have been done by simply
 *                setting the disconnect_on_fail config parameter, but instead we use the event
 *                handler mechanism to demonstrate its use.
 *
 * @param[in]   p_evt   Event received from the Connection Parameters Module.
 */
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    uint32_t err_code;
    debug(0,"Connect parameter event \n\r");
    if(p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
        APP_ERROR_CHECK(err_code);
    }
    
}


/**@brief Function for handling a Connection Parameters error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;
    debug(0,"Initializing connect parameter \n\r");
    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for starting advertising.
 */
static void advertising_start(void)
{
    uint32_t             err_code;
    ble_gap_adv_params_t adv_params;

    // Start advertising
    memset(&adv_params, 0, sizeof(adv_params));

    adv_params.type        = BLE_GAP_ADV_TYPE_ADV_IND;
    adv_params.p_peer_addr = NULL;
    adv_params.fp          = BLE_GAP_ADV_FP_ANY;
    adv_params.interval    = APP_ADV_INTERVAL;
    adv_params.timeout     = APP_ADV_TIMEOUT_IN_SECONDS;
    debug(0,"Start advertising \n\r");
    err_code = sd_ble_gap_adv_start(&adv_params);
    APP_ERROR_CHECK(err_code);
    debug(0,"Off led 0 \n\r");
    control_gpio(LED_0,ON,0);
}


/**@brief Function for handling the Application's BLE Stack events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void on_ble_evt(ble_evt_t * p_ble_evt)
{
    uint32_t                         err_code;
    static ble_gap_evt_auth_status_t m_auth_status;
    ble_gap_enc_info_t *             p_enc_info;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            debug(0,"Connect device \n\r");
            control_gpio(LED_0,OFF,0);
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;

            // vng err_code = app_button_enable();
            // vng APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            debug(0,"Disconnect device \n\r");
            m_conn_handle = BLE_CONN_HANDLE_INVALID;

            // vng err_code = app_button_disable();
            // vng APP_ERROR_CHECK(err_code);
            debug(0,"Start advertising \n\r");
            advertising_start();
            break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            debug(0,"Parameter request \n\r");
            err_code = sd_ble_gap_sec_params_reply(m_conn_handle,
                                                   BLE_GAP_SEC_STATUS_SUCCESS,
                                                   &m_sec_params);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
            err_code = sd_ble_gatts_sys_attr_set(m_conn_handle, NULL, 0);
            APP_ERROR_CHECK(err_code);
            break;

        case BLE_GAP_EVT_AUTH_STATUS:
            m_auth_status = p_ble_evt->evt.gap_evt.params.auth_status;
            break;

        case BLE_GAP_EVT_SEC_INFO_REQUEST:
            p_enc_info = &m_auth_status.periph_keys.enc_info;
            if (p_enc_info->div == p_ble_evt->evt.gap_evt.params.sec_info_request.div)
            {
                err_code = sd_ble_gap_sec_info_reply(m_conn_handle, p_enc_info, NULL);
                APP_ERROR_CHECK(err_code);
            }
            else
            {
                // No keys found for this device
                err_code = sd_ble_gap_sec_info_reply(m_conn_handle, NULL, NULL);
                APP_ERROR_CHECK(err_code);
            }
            break;

        case BLE_GAP_EVT_TIMEOUT:
            debug(0,"Time out in GAP \n\r");
            if (p_ble_evt->evt.gap_evt.params.timeout.src == BLE_GAP_TIMEOUT_SRC_ADVERTISEMENT)
            {
                // Configure buttons with sense level low as wakeup source.
                // vng nrf_gpio_cfg_sense_input(WAKEUP_BUTTON_PIN,
                // vng                          BUTTON_PULL,
                // vng                          NRF_GPIO_PIN_SENSE_LOW);
                
                // Go to system-off mode (this function will not return; wakeup will cause a reset)                
                err_code = sd_power_system_off();
                APP_ERROR_CHECK(err_code);
            }
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called from the scheduler in the main loop after a BLE stack
 *          event has been received.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
    debug(0,"BLE event dispatch \n\r");
    on_ble_evt(p_ble_evt);
    ble_conn_params_on_ble_evt(p_ble_evt);
    ble_cis_on_ble_evt(&m_cis, p_ble_evt);
}

/**@brief Function for dispatching a system event to interested modules.
 *
 * @details This function is called from the System event interrupt handler after a system
 *          event has been received.
 *
 * @param[in]   sys_evt   System stack event.
 */
static void sys_evt_dispatch(uint32_t sys_evt)
{
    pstorage_sys_event_handler(sys_evt);
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    uint32_t err_code;

    // Initialize the SoftDevice handler module.
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, false);
    
    // Enable BLE stack 
    ble_enable_params_t ble_enable_params;
    memset(&ble_enable_params, 0, sizeof(ble_enable_params));
    ble_enable_params.gatts_enable_params.service_changed = IS_SRVC_CHANGED_CHARACT_PRESENT;
    err_code = sd_ble_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);

    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);
    
    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
    debug(0,"Initializing stack... \n\r");
}


/**@brief Function for the Event Scheduler initialization.
 */
static void scheduler_init(void)
{
    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}

/*
static void button_event_handler(uint8_t pin_no, uint8_t button_action)
{
    uint32_t err_code;
    debug(0,"Button event handler, pin_no is %d \n\r",pin_no);

    switch (pin_no)
    {
        case LEDBUTTON_BUTTON_PIN_NO:
            err_code = ble_cis_on_button_change(&m_cis, button_action);
            if (err_code != NRF_SUCCESS &&
                err_code != BLE_ERROR_INVALID_CONN_HANDLE &&
                err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
            break;

        default:
            APP_ERROR_HANDLER(pin_no);
            break;
    }
}
*/
/**@brief Function for initializing the GPIOTE handler module.
 */
static void gpiote_init(void)
{
    APP_GPIOTE_INIT(APP_GPIOTE_MAX_USERS);
}


/**@brief Function for initializing the button handler module.
 */
/*
static void buttons_init(void)
{
    // Note: Array must be static because a pointer to it will be saved in the Button handler
    //       module.
    static app_button_cfg_t buttons[] =
    {
        {WAKEUP_BUTTON_PIN, false, BUTTON_PULL, NULL},
        {LEDBUTTON_BUTTON_PIN_NO, false, BUTTON_PULL, button_event_handler}
    };

    APP_BUTTON_INIT(buttons, sizeof(buttons) / sizeof(buttons[0]), BUTTON_DETECTION_DELAY, true);
}
*/

/**@brief Function for the Power manager.
 */
static void power_manage(void)
{
    uint32_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for application main entry.
 */
int main(void)
{
    // Initialize
    leds_init();
    timers_init();
    gpiote_init();
    // vng buttons_init();
    ble_stack_init();
    scheduler_init();    
    gap_params_init();
    services_init();
    advertising_init();
    conn_params_init();
    sec_params_init();

    // Start execution
    timers_start();
    advertising_start();

    // Enter main loop
    for (;;)
    {
        app_sched_execute();
        power_manage();
    }
}

/**
 * @}
 */
