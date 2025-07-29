#include "bluetooth_stack.h"
#include "bluetooth_message_service.h"
#include "app_error.h"
#include "app_error_weak.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "nrf_log.h"
#include "nrf_ble_qwr.h"
#include "ble_conn_state.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "nrf_ble_gatt.h"
#include "app_timer.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"
#include "config/settings.h"
#include "drivers_nrf/power_manager.h"
#include "drivers_nrf/timers.h"
#include "core/delegate_array.h"

#include "pixel.h"
#include "bluetooth_custom_advertising_data.h"

using namespace Config;
using namespace DriversNRF;
using namespace Modules;

namespace Bluetooth::Stack
{
    #define APP_ADV_INTERVAL                MSEC_TO_UNITS(200, UNIT_0_625_MS)       /**< The advertising interval (0.2s). */
    #define APP_ADV_DURATION                BLE_GAP_ADV_TIMEOUT_GENERAL_UNLIMITED   /**< The advertising duration (180 seconds) in units of 10 milliseconds. */

    #define APP_BLE_OBSERVER_PRIO           3                                       /**< Application's BLE observer priority. You shouldn't need to modify this value. */
    #define APP_BLE_CONN_CFG_TAG            1                                       /**< A tag identifying the SoftDevice BLE configuration. */

    #define MIN_CONN_INTERVAL               MSEC_TO_UNITS(15, UNIT_1_25_MS)         /**< Minimum acceptable connection interval (0.015 seconds). */
    #define MAX_CONN_INTERVAL               MSEC_TO_UNITS(30, UNIT_1_25_MS)         /**< Maximum acceptable connection interval (0.030 seconds). */
    #define SLAVE_LATENCY                   0                                       /**< Slave latency. */
    #define CONN_SUP_TIMEOUT                MSEC_TO_UNITS(3000, UNIT_10_MS)         /**< Connection supervisory timeout (4 seconds). */

    #define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(5000)                   /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
    #define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(30000)                  /**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
    #define MAX_CONN_PARAMS_UPDATE_COUNT    3                                       /**< Number of attempts before giving up the connection parameter negotiation. */

    #define SEC_PARAM_BOND                  1                                       /**< No bonding. */
    #define SEC_PARAM_MITM                  0                                       /**< Man In The Middle protection not required. */
    #define SEC_PARAM_LESC                  0                                       /**< LE Secure Connections not enabled. */
    #define SEC_PARAM_KEYPRESS              0                                       /**< Keypress notifications not enabled. */
    #define SEC_PARAM_IO_CAPABILITIES       BLE_GAP_IO_CAPS_NONE                    /**< No I/O capabilities. */
    #define SEC_PARAM_OOB                   0                                       /**< Out Of Band data not available. */
    #define SEC_PARAM_MIN_KEY_SIZE          7                                       /**< Minimum encryption key size. */
    #define SEC_PARAM_MAX_KEY_SIZE          16                                      /**< Maximum encryption key size. */

    #define MAX_CLIENTS 8
    #define MAX_RSSI_CLIENTS 2

    #define RSSI_THRESHOLD_DBM 1
    #define RSSI_NOTIFY_MIN_INTERVAL 1000 // In ms

    static uint16_t connectionHandle = BLE_CONN_HANDLE_INVALID;                     /**< Handle of the current connection. */
    //NRF_BLE_QWR_DEF(nrfQwr);                                                        /**< Context for the Queued Write module.*/
    NRF_BLE_GATT_DEF(nrfGatt);                                                      /**< GATT module instance. */

    BLE_ADVERTISING_DEF(advertisingModule);                                         /**< Advertising module instance. */

    static bool connected = false;
    static bool resetOnDisconnectPending = false;
    static bool sleepOnDisconnectPending = false;

    /**< Universally unique service identifiers. */
    static ble_uuid_t advertisedUuids[] = {
        {BLE_UUID_DEVICE_INFORMATION_SERVICE, BLE_UUID_TYPE_BLE},
    };

    static ble_uuid_t advertisedUuidsExtended[] = {
        {GENERIC_DATA_SERVICE_UUID_SHORT, BLE_UUID_TYPE_VENDOR_BEGIN},
    };

    DelegateArray<ConnectionEventMethod, MAX_CLIENTS> clients;
    DelegateArray<RssiEventMethod, MAX_RSSI_CLIENTS> rssiClients;

#pragma pack( push, 1)
    struct CustomServiceData {
        uint32_t deviceId;
        uint32_t buildTimestamp;
    };
#pragma pack(pop)

    // Global custom manufacturer and service data
    static CustomServiceData customServiceData;

    // Buffers pointing to the custom advertising and service data
    static ble_advdata_manuf_data_t advertisedManufData = {
        .company_identifier = 0x0D39, // Systemic Games company identifier
        .data = {
            .size   = 0,        // Initialized later by the custom data handler
            .p_data = nullptr
        }
    };
    static ble_advdata_service_data_t advertisedServiceData = {
        .service_uuid = BLE_UUID_DEVICE_INFORMATION_SERVICE,
        .data = {
            .size   = sizeof(customServiceData),
            .p_data = (uint8_t*)&customServiceData
        }
    };

    // Advertising data structs
    static ble_advdata_t advertisementPacket;
    static ble_advdata_t scanResponsePacket;

    /**@brief Function for handling BLE events.
     *
     * @param[in]   p_ble_evt   Bluetooth stack event.
     * @param[in]   p_context   Unused.
     */
    void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context) {
        ret_code_t err_code = NRF_SUCCESS;

        switch (p_ble_evt->header.evt_id) {
            case BLE_GAP_EVT_DISCONNECTED:
                NRF_LOG_INFO("Disco: 0x%02x", p_ble_evt->evt.gap_evt.params.disconnected.reason);
                connected = false;
                for (int i = 0; i < clients.Count(); ++i) {
                    clients[i].handler(clients[i].token, false);
                }

                if (resetOnDisconnectPending) {
                    resetOnDisconnectPending = false;
                    sleepOnDisconnectPending = false;
                    PowerManager::reset();
                } else if (sleepOnDisconnectPending) {
                    sleepOnDisconnectPending = false;
                    PowerManager::goToSleep();
                }

                break;

            case BLE_GAP_EVT_CONNECTED:
                NRF_LOG_INFO("Connected");
                connectionHandle = p_ble_evt->evt.gap_evt.conn_handle;
                // err_code = nrf_ble_qwr_conn_handle_assign(&nrfQwr, connectionHandle);
                // APP_ERROR_CHECK(err_code);
                connected = true;
                for (int i = 0; i < clients.Count(); ++i) {
                    clients[i].handler(clients[i].token, true);
                }

                CustomAdvertisingDataHandler::stop();
                break;

            case BLE_GAP_EVT_PHY_UPDATE_REQUEST: {
                NRF_LOG_DEBUG("PHY update request");
                ble_gap_phys_t const phys =
                {
                    .tx_phys = BLE_GAP_PHY_AUTO,
                    .rx_phys = BLE_GAP_PHY_AUTO,
                };
                err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
                APP_ERROR_CHECK(err_code);
                break;
            }

            case BLE_GAP_EVT_RSSI_CHANGED: {
                static uint32_t nextUpdate = 0;
                // No more often than once a second
                const uint32_t time = DriversNRF::Timers::millis();
                if (nextUpdate <= time) {
                    nextUpdate = time + RSSI_NOTIFY_MIN_INTERVAL;
                    auto rssi = p_ble_evt->evt.gap_evt.params.rssi_changed.rssi;
                    auto chIndex = p_ble_evt->evt.gap_evt.params.rssi_changed.ch_index;
                    for (int i = 0; i < rssiClients.Count(); ++i) {
                        rssiClients[i].handler(rssiClients[i].token, rssi, chIndex);
                    }
                }
                break;
            }

            case BLE_GATTC_EVT_TIMEOUT:
                // Disconnect on GATT Client timeout event.
                NRF_LOG_DEBUG("GATT Client Timeout");
                err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
                                                BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
                APP_ERROR_CHECK(err_code);
                break;

            case BLE_GATTS_EVT_TIMEOUT:
                // Disconnect on GATT Server timeout event.
                NRF_LOG_DEBUG("GATT Server Timeout");
                err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
                                                BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
                APP_ERROR_CHECK(err_code);
                break;

            case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
                // Pairing not supported
                NRF_LOG_DEBUG("Pairing not supported!");
                err_code = sd_ble_gap_sec_params_reply(connectionHandle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
                APP_ERROR_CHECK(err_code);
                break;

            case BLE_GATTS_EVT_SYS_ATTR_MISSING:
                // No system attributes have been stored.
                NRF_LOG_DEBUG("System Attributes Missing!");
                err_code = sd_ble_gatts_sys_attr_set(connectionHandle, NULL, 0, 0);
                APP_ERROR_CHECK(err_code);
                break;

            case BLE_GATTS_EVT_HVN_TX_COMPLETE:
                // Last notification was cleared!
                NRF_LOG_DEBUG("Notification Complete!");
                break;

            case BLE_GATTS_EVT_HVC:
                // Last notification was cleared!
                NRF_LOG_DEBUG("Confirmation Received!");
                break;

            case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST:
                NRF_LOG_DEBUG("Exchange MTU request!");
                break;

            default:
                // No implementation needed.
                break;
        }

        PowerManager::feed();
    }

    // Function for handling advertising events.
    // This function will be called for advertising events which are passed to the application.
    void on_adv_evt(ble_adv_evt_t ble_adv_evt) {
        switch (ble_adv_evt)
        {
            case BLE_ADV_EVT_FAST: {
                NRF_LOG_INFO("Fast adv.");
                ret_code_t err_code = ble_advertising_advdata_update(&advertisingModule, &advertisementPacket, &scanResponsePacket);
                APP_ERROR_CHECK(err_code);
                CustomAdvertisingDataHandler::start();
            }
            break;

            case BLE_ADV_EVT_IDLE:
                NRF_LOG_INFO("Adv. idle");
                CustomAdvertisingDataHandler::stop();
                break;

            default:
                // No implementation needed.
                break;
        }
    }

    void nrf_qwr_error_handler(uint32_t nrf_error) {
        APP_ERROR_HANDLER(nrf_error);
    }

    /**@brief Function for handling a Connection Parameters error.
     *
     * @param[in] nrf_error  Error code containing information about what went wrong.
     */
    void conn_params_error_handler(uint32_t nrf_error) {
        APP_ERROR_HANDLER(nrf_error);
    }

    /**@brief Function for handling Peer Manager events.
     *
     * @param[in] p_evt  Peer Manager event.
     */
    void pm_evt_handler(pm_evt_t const * p_evt) {
        pm_handler_on_pm_evt(p_evt);
        pm_handler_flash_clean(p_evt);
    }

    void advertising_config_get(ble_adv_modes_config_t * p_config) {
        memset(p_config, 0, sizeof(ble_adv_modes_config_t));

        p_config->ble_adv_fast_enabled  = true;
        p_config->ble_adv_fast_interval = APP_ADV_INTERVAL;
        p_config->ble_adv_fast_timeout  = APP_ADV_DURATION;
    }

    void init() {
        ret_code_t err_code;

        err_code = nrf_sdh_enable_request();
        APP_ERROR_CHECK(err_code);

        // Configure the BLE stack using the default settings.
        // Fetch the start address of the application RAM.
        uint32_t ram_start = 0;
        err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
        APP_ERROR_CHECK(err_code);

        // Enable BLE stack.
        err_code = nrf_sdh_ble_enable(&ram_start);
        APP_ERROR_CHECK(err_code);

        // Register a handler for BLE events.
        // Nothing to validate
        NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);

        //  GAP Params
        ble_gap_conn_params_t   gap_conn_params;

        memset(&gap_conn_params, 0, sizeof(gap_conn_params));

        gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
        gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
        gap_conn_params.slave_latency     = SLAVE_LATENCY;
        gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;

        err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
        APP_ERROR_CHECK(err_code);

        err_code = nrf_ble_gatt_init(&nrfGatt, NULL);
        APP_ERROR_CHECK(err_code);

        NRF_LOG_DEBUG("BLE Stack init, RAM start: 0x%X", ram_start);
    }

    void initAdvertising() {
        ble_advertising_init_t init;
        memset(&init, 0, sizeof(init));

        init.advdata.name_type               = BLE_ADVDATA_FULL_NAME;
        // We removed advertising the appearance to save 2 bytes of data so the advertised name can be longer
        //init.advdata.include_appearance      = true; // Let Central know what kind of the device we are
        init.advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
        init.advdata.uuids_complete.uuid_cnt = sizeof(advertisedUuids) / sizeof(advertisedUuids[0]);
        init.advdata.uuids_complete.p_uuids  = advertisedUuids;

        // Update device Id and build now even though it'll only be used on Scan Response
        // These values never change.
        customServiceData.deviceId = Pixel::getDeviceID();
        customServiceData.buildTimestamp = Pixel::getBuildTimestamp();

        init.srdata.uuids_complete.uuid_cnt = sizeof(advertisedUuidsExtended) / sizeof(advertisedUuidsExtended[0]);
        init.srdata.uuids_complete.p_uuids  = advertisedUuidsExtended;
        init.srdata.p_service_data_array    = &advertisedServiceData;
        init.srdata.service_data_count      = 1;

        advertising_config_get(&init.config);

        init.evt_handler = on_adv_evt;

        ret_code_t err_code = ble_advertising_init(&advertisingModule, &init);
        APP_ERROR_CHECK(err_code);

        ble_advertising_conn_cfg_tag_set(&advertisingModule, APP_BLE_CONN_CFG_TAG);

        ble_conn_params_init_t cp_init;

        memset(&cp_init, 0, sizeof(cp_init));

        cp_init.p_conn_params                  = NULL;
        cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
        cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
        cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
        cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
        cp_init.disconnect_on_fail             = true; // Disconnect on a failed connection parameters update, use evt_handler to add a custom behavior
        cp_init.error_handler                  = conn_params_error_handler;

        err_code = ble_conn_params_init(&cp_init);
        APP_ERROR_CHECK(err_code);

        // Set TX Power to max
        err_code = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV, advertisingModule.adv_handle, 4);
        APP_ERROR_CHECK(err_code);

        // Copy advertising data for later, when we update the manufacturer data
        memcpy(&advertisementPacket, &init.advdata, sizeof(ble_advdata_t));
        memcpy(&scanResponsePacket, &init.srdata, sizeof(ble_advdata_t));
        advertisementPacket.p_manuf_specific_data = &advertisedManufData;

        ble_gap_conn_sec_mode_t sec_mode;

        BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

        // Generate our name
        auto name = SettingsManager::getSettings()->name;
        err_code = sd_ble_gap_device_name_set(&sec_mode, (const uint8_t *)name, strlen(name));
        APP_ERROR_CHECK(err_code);

        NRF_LOG_DEBUG("Advertisement payload size: %d, and scan response payload size: %d", advertisingModule.adv_data.adv_data.len, advertisingModule.adv_data.scan_rsp_data.len);
    }

    void updateCustomAdvertisingData(uint8_t* data, uint16_t size) {
        advertisedManufData.data.p_data = data;
        advertisedManufData.data.size = size;
        ret_code_t err_code = ble_advertising_advdata_update(&advertisingModule, &advertisementPacket, &scanResponsePacket);
        APP_ERROR_CHECK(err_code);
    }

    void disconnectLink(uint16_t conn_handle, void * p_context) {
        UNUSED_PARAMETER(p_context);

        ret_code_t err_code = sd_ble_gap_disconnect(conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
        if (err_code != NRF_SUCCESS) {
            NRF_LOG_WARNING("Failed to disconnect connection. Connection handle: %d Error: %d", conn_handle, err_code);
        } else {
            NRF_LOG_DEBUG("Disconnected connection handle %d", conn_handle);
        }
    }

    void disconnect() {
        // Disconnect all other bonded devices that currently are connected.
        // This is required to receive a service changed indication
        // on boot after a successful (or aborted) Device Firmware Update.
        uint32_t conn_count = ble_conn_state_for_each_connected(disconnectLink, NULL);
        NRF_LOG_DEBUG("Disconnected %d links.", conn_count);
    }

    void startAdvertising() {
        ret_code_t err_code = ble_advertising_advdata_update(&advertisingModule, &advertisementPacket, &scanResponsePacket);
        APP_ERROR_CHECK(err_code);

        err_code = ble_advertising_start(&advertisingModule, BLE_ADV_MODE_FAST);
        APP_ERROR_CHECK(err_code);
        NRF_LOG_INFO("Adv. with name=%s and deviceId=0x%x", SettingsManager::getSettings()->name, customServiceData.deviceId);
    }

    void disableAdvertisingOnDisconnect() {
        // Prevent device from advertising on disconnect.
        ble_adv_modes_config_t config;
        advertising_config_get(&config);
        config.ble_adv_on_disconnect_disabled = true;
        ble_advertising_modes_config_set(&advertisingModule, &config);
    }

    void enableAdvertisingOnDisconnect() {
        // Setup device to re-start advertising on disconnect.
        ble_adv_modes_config_t config;
        advertising_config_get(&config);
        config.ble_adv_on_disconnect_disabled = false;
        ble_advertising_modes_config_set(&advertisingModule, &config);
    }

    void resetOnDisconnect() {
        resetOnDisconnectPending = true;
    }

    void sleepOnDisconnect() {
        sleepOnDisconnectPending = true;
    }

    SendResult send(uint16_t handle, const uint8_t* data, uint16_t len) {
        PowerManager::feed();
        if (connected) {
            ble_gatts_hvx_params_t hvx_params;
            memset(&hvx_params, 0, sizeof(hvx_params));
            hvx_params.handle = handle;
            hvx_params.p_data = data;
            hvx_params.p_len = &len;
            hvx_params.type = BLE_GATT_HVX_NOTIFICATION;
            ret_code_t err_code = sd_ble_gatts_hvx(connectionHandle, &hvx_params);
            if (err_code == NRF_SUCCESS) {
                // Message was sent!
                NRF_LOG_DEBUG("Send message type %d of size %d", data[0], len);
                return SendResult_Ok;
            } else if (err_code == NRF_ERROR_BUSY || err_code == NRF_ERROR_RESOURCES) {
                return SendResult_Busy;
            } else {
                // Some other error happened
                NRF_LOG_ERROR("Could not send notification for message type %d of size %d, Error %s(0x%x)", data[0], len, NRF_LOG_ERROR_STRING_GET(err_code), err_code);

                // We get this sys_attr_missing when trying to send a message right after the connect event
                // but before the server has enabled characteristic notifications.
                // There might be other errors we would get before the stack is ready to send messages...
                return err_code == BLE_ERROR_GATTS_SYS_ATTR_MISSING ? SendResult_NotReady : SendResult_Error;
            }
        } else {
            return SendResult_NotConnected;
        }
    }

    void slowAdvertising() {
        ret_code_t err_code = ble_advertising_start(&advertisingModule, BLE_ADV_MODE_SLOW);
        APP_ERROR_CHECK(err_code);
    }

    void stopAdvertising() {
        ret_code_t err_code = sd_ble_gap_adv_stop(advertisingModule.adv_handle);
        APP_ERROR_CHECK(err_code);
        CustomAdvertisingDataHandler::stop();
    }

    bool isConnected() {
        return connected;
    }

    void hook(ConnectionEventMethod method, void* param) {
        if (!clients.Register(param, method)) {
            NRF_LOG_ERROR("Too many connection state hooks registered.");
        }
    }

    void unHook(ConnectionEventMethod method) {
        clients.UnregisterWithHandler(method);
    }

    void unHookWithParam(void* param) {
        clients.UnregisterWithToken(param);
    }

    void hookRssi(RssiEventMethod method, void* param) {
        if (rssiClients.Count() == 0) {
            sd_ble_gap_rssi_start(connectionHandle, RSSI_THRESHOLD_DBM, 1); 
        }
        if (!rssiClients.Register(param, method)) {
            NRF_LOG_ERROR("Too many RSSI hooks registered.");
        }
    }
    
    void unHookRssi(RssiEventMethod client) {
        rssiClients.UnregisterWithHandler(client);
        if (rssiClients.Count() == 0) {
            // No longer need rssi levels
            sd_ble_gap_rssi_stop(connectionHandle);
        }
    }
}
