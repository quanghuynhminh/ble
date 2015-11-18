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


#include "ble_cis.h"
#include <string.h>
#include "nordic_common.h"
#include "ble_srv_common.h"
#include "app_util.h"


/**@brief Function for handling the Connect event.
 *
 * @param[in]   p_cis       Battery Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_connect(ble_cis_t * p_cis, ble_evt_t * p_ble_evt)
{
    p_cis->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
    debug(0,"Handling on connect \n\r");

}


/**@brief Function for handling the Disconnect event.
 *
 * @param[in]   p_cis       Battery Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_disconnect(ble_cis_t * p_cis, ble_evt_t * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_cis->conn_handle = BLE_CONN_HANDLE_INVALID;
    debug(0,"Handling on disconnect \n\r");
}


/**@brief Function for handling the Write event.
 *
 * @param[in]   p_cis       Battery Service structure.
 * @param[in]   p_ble_evt   Event received from the BLE stack.
 */
static void on_write(ble_cis_t * p_cis, ble_evt_t * p_ble_evt)
{
    ble_gatts_evt_write_t * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
    debug(0,"Handling on write \n\r ");
    if (
        (p_evt_write->handle == p_cis->led_char_handles.value_handle)
        &&(p_evt_write->len == 1)&&(p_cis->led_write_handler != NULL)
       )
    {
        p_cis->led_write_handler(p_cis, p_evt_write->data[0]);
    }
}


void ble_cis_on_ble_evt(ble_cis_t * p_cis, ble_evt_t * p_ble_evt)
{
    debug(0,"Handling on BLE service event: \n\r");
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_cis, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_cis, p_ble_evt);
            break;

        case BLE_GATTS_EVT_WRITE:
            on_write(p_cis, p_ble_evt);
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for adding the LED characteristic.
 *
 */
static uint32_t led_char_add(ble_cis_t * p_cis, const ble_cis_init_t * p_cis_init)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&char_md, 0, sizeof(char_md));
    
    char_md.char_props.read   = 1;
    char_md.char_props.write  = 1;
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = NULL;
    char_md.p_sccd_md         = NULL;
	
    ble_uuid.type = p_cis->uuid_type;
    ble_uuid.uuid = CIS_UUID_LED_CHAR;
	
    memset(&attr_md, 0, sizeof(attr_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;
    
    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid       = &ble_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = sizeof(uint8_t);
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = sizeof(uint8_t);
    attr_char_value.p_value      = NULL;
    debug(0,"Led characteristic addition \n\r");

    return sd_ble_gatts_characteristic_add(p_cis->service_handle, &char_md,
                                               &attr_char_value,
                                               &p_cis->led_char_handles);
}

/**@brief Function for adding the Button characteristic.
 *
 
static uint32_t button_char_add(ble_cis_t * p_cis, const ble_cis_init_t * p_cis_init)
{
    ble_gatts_char_md_t char_md;
    ble_gatts_attr_md_t cccd_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&cccd_md, 0, sizeof(cccd_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    cccd_md.vloc = BLE_GATTS_VLOC_STACK;
    
    memset(&char_md, 0, sizeof(char_md));
    
    char_md.char_props.read   = 1;
    char_md.char_props.notify = 1;
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = &cccd_md;
    char_md.p_sccd_md         = NULL;

    ble_uuid.type = p_cis->uuid_type;
    ble_uuid.uuid = CIS_UUID_BUTTON_CHAR;

    memset(&attr_md, 0, sizeof(attr_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&attr_md.write_perm);
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 0;
    
    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid       = &ble_uuid;
    attr_char_value.p_attr_md    = &attr_md;
    attr_char_value.init_len     = sizeof(uint8_t);
    attr_char_value.init_offs    = 0;
    attr_char_value.max_len      = sizeof(uint8_t);
    attr_char_value.p_value      = NULL;
    debug(0,"Button characteristic addition \n\r");
    return sd_ble_gatts_characteristic_add(p_cis->service_handle, &char_md,
                                               &attr_char_value,
                                               &p_cis->button_char_handles);
}
*/
uint32_t ble_cis_init(ble_cis_t * p_cis, const ble_cis_init_t * p_cis_init)
{
    uint32_t   err_code;
    ble_uuid128_t base_uuid = {CIS_UUID_BASE};
    ble_uuid_t ble_uuid;

    // Initialize service structure
    p_cis->conn_handle               = BLE_CONN_HANDLE_INVALID;
    p_cis->led_write_handler         = p_cis_init->led_write_handler;
    
    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_cis->uuid_type);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    ble_uuid.type = p_cis->uuid_type;
    ble_uuid.uuid = CIS_UUID_SERVICE;
    debug(0,"Service addition \n\r");

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid,&p_cis->service_handle);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

   // vng err_code = button_char_add(p_cis, p_cis_init);
   // if (err_code != NRF_SUCCESS)
   // {
   //     return err_code;
   // }
    err_code = led_char_add(p_cis, p_cis_init);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    
    return NRF_SUCCESS;
}
/*
uint32_t ble_cis_on_button_change(ble_cis_t * p_cis, uint8_t button_state)
{
    ble_gatts_hvx_params_t params;
    uint16_t len = sizeof(button_state);
    debug(0,"Press button notify \n\r");
    
    memset(&params, 0, sizeof(params));
    params.type = BLE_GATT_HVX_NOTIFICATION;
    params.handle = p_cis->button_char_handles.value_handle;
    params.p_data = &button_state;
    params.p_len = &len;
    
    return sd_ble_gatts_hvx(p_cis->conn_handle, &params);
}
*/
