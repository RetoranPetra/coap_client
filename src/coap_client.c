/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/logging/log.h>
#include <ram_pwrdn.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <C:\Nordic\Git_apps\Message_On_Top_Server\coap_server\interface\coap_server_client_interface.h>
#include "coap_client_utils.h"
//L
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <stdlib.h>
#include <zephyr/drivers/ieee802154/cc1200.h>
#include <nrf_802154.h>
//L

LOG_MODULE_REGISTER(coap_client, CONFIG_COAP_CLIENT_LOG_LEVEL);

#define OT_CONNECTION_LED DK_LED1
#define BLE_CONNECTION_LED DK_LED2
#define MTD_SED_LED DK_LED3

static void on_ot_connect(struct k_work *item)
{
	ARG_UNUSED(item);

	dk_set_led_on(OT_CONNECTION_LED);
}

static void on_ot_disconnect(struct k_work *item)
{
	ARG_UNUSED(item);

	dk_set_led_off(OT_CONNECTION_LED);
}

static void on_mtd_mode_toggle(uint32_t med)
{
#if IS_ENABLED(CONFIG_PM_DEVICE)
	const struct device *cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	if (!device_is_ready(cons)) {
		return;
	}

	if (med) {
		pm_device_action_run(cons, PM_DEVICE_ACTION_RESUME);
	} else {
		pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
	}
#endif
	dk_set_led(MTD_SED_LED, med);
}

static void on_button_changed(uint32_t button_state, uint32_t has_changed)
{
	uint32_t buttons = button_state & has_changed;

	if (buttons & DK_BTN1_MSK) {
		//coap_client_toggle_one_light(); //Sends motors forward message
		
		nrf_802154_tx_power_set(-18);
		coap_client_floatSend(0.2);
		LOG_DBG("Channel power set to %d dBm\n", nrf_802154_tx_power_get());
	}

	if (buttons & DK_BTN2_MSK) {
		//coap_client_toggle_mesh_lights();

		char msg[GENERIC_PAYLOAD_SIZE] = "Generic Text";
		coap_client_genericSend(msg);
	}

	if (buttons & DK_BTN3_MSK) {
		//coap_client_toggle_minimal_sleepy_end_device();
		//If this doesn't work convert to float at the server side
		nrf_802154_tx_power_set(0);
		coap_client_floatSend(0.8);
		LOG_DBG("Channel power set to %d dBm\n", nrf_802154_tx_power_get());
	}

	if (buttons & DK_BTN4_MSK) {
		coap_client_send_provisioning_request();
	}
}

void main(void)
{
	//Sleep needed for logs to display correctly.
	k_msleep(1000);
	
	int ret;
	LOG_INF("Start CoAP-client sample");
	
	if (IS_ENABLED(CONFIG_RAM_POWER_DOWN_LIBRARY)) {
		power_down_unused_ram();
	}

	ret = dk_buttons_init(on_button_changed);
	if (ret) {
		LOG_ERR("Cannot init buttons (error: %d)", ret);
		return;
	}

	ret = dk_leds_init();
	if (ret) {
		LOG_ERR("Cannot init leds, (error: %d)", ret);
		return;
	}

	coap_client_utils_init(on_ot_connect, on_ot_disconnect, on_mtd_mode_toggle);
	gpio_init();
	//otPlatRadioSetChannelMaxTransmitPower(openthread_get_default_instance(), 11, 0);
}
