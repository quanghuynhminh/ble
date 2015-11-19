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

#ifndef BLE_CIS_H__
#define BLE_CIS_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"

#define CIS_UUID_BASE {0x32, 0x92, 0xF2, 0xEF, 0x2B, 0xD2, 0x6F, 0xBC, 0x55, 0x4F,0xFE, 0xF5, 0xAB, 0xDE, 0x8E, 0x2B}
#define CIS_UUID_SERVICE 0x0000
#define CIS_UUID_LED_CHAR 0x0001
#define CIS_UUID_BUTTON_CHAR 0x1524



// Forward declaration of the ble_cis_t type. 
typedef struct ble_cis_s ble_cis_t;

/**@brief Identify Service event handler type. */
typedef void (*ble_cis_brew_coffee_handler_t) (ble_cis_t * p_cis, uint8_t new_state);

/**@brief Identify Service init structure. This contains all options and data needed for
 *        initialization of the service.*/
typedef struct
{
    ble_cis_brew_coffee_handler_t         brew_coffee_handler;        /**< Event handler to be called for handling events in the Identify Service. */
} ble_cis_init_t;

/**@brief Identify Service structure. This contains various status information for the service. */
typedef struct ble_cis_s
{
    uint16_t                      service_handle;                 /**< Handle of Identify Service (as provided by the BLE stack). */
    ble_gatts_char_handles_t      led_char_handles;               /**< Handles related to the Identify Level characteristic. */
    ble_gatts_char_handles_t      button_char_handles;
    uint8_t                       uuid_type;                      /**< Last Identify Level measurement passed to the Identify Service. */
    uint16_t                      conn_handle;                    /**< Handle of the current connection (as provided by the BLE stack, is BLE_CONN_HANDLE_INVALID if not in a connection). */
    ble_cis_brew_coffee_handler_t   brew_coffee_handler;      /**< TRUE if notification of Identify Level is supported. */
} ble_cis_t;

/**@brief Function for initializing the Identify Service.
 *
 * @param[out]  p_cis       Identify Service structure. This structure will have to be supplied by
 *                          the application. It will be initialized by this function, and will later
 *                          be used to identify this particular service instance.
 * @param[in]   p_cis_init  Information needed to initialize the service.
 *
 * @return      NRF_SUCCESS on successful initialization of service, otherwise an error code.
 */
uint32_t ble_cis_init(ble_cis_t * p_cis, const ble_cis_init_t * p_cis_init);

/**@brief Function for handling the Application's BLE Stack events.
 *
 * @details Handles all events from the BLE stack of interest to the Identify Service.
 *
 *
 * @param[in]   p_cis      Identify Service structure.
 * @param[in]   p_ble_evt  Event received from the BLE stack.
 */
void ble_cis_on_ble_evt(ble_cis_t * p_cis, ble_evt_t * p_ble_evt);

/**@brief Function for sending a button state notification.
 */
// uint32_t ble_cis_on_button_change(ble_cis_t * p_cis, uint8_t button_state);
#endif // BLE_CIS_H__

/** @} */
