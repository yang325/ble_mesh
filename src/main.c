/* main.c - Application main entry point */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <misc/printk.h>
#include <logging/log.h>
#include <settings/settings.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/mesh.h>

#include "board.h"

LOG_MODULE_REGISTER(main);

static struct bt_mesh_cfg_srv cfg_srv = {
	.relay = BT_MESH_RELAY_DISABLED,
	.beacon = BT_MESH_BEACON_ENABLED,
#if defined(CONFIG_BT_MESH_FRIEND)
	.frnd = BT_MESH_FRIEND_ENABLED,
#else
	.frnd = BT_MESH_FRIEND_NOT_SUPPORTED,
#endif
#if defined(CONFIG_BT_MESH_GATT_PROXY)
	.gatt_proxy = BT_MESH_GATT_PROXY_ENABLED,
#else
	.gatt_proxy = BT_MESH_GATT_PROXY_NOT_SUPPORTED,
#endif
	.default_ttl = 7,

	/* 3 transmissions with 20ms interval */
	.net_transmit = BT_MESH_TRANSMIT(2, 20),
	.relay_retransmit = BT_MESH_TRANSMIT(2, 20),
};

static struct bt_mesh_health_srv health_srv = {
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);

static struct bt_mesh_model_pub gen_onoff_pub;

static void gen_onoff_get(struct bt_mesh_model *model,
						struct bt_mesh_msg_ctx *ctx,
						struct net_buf_simple *buf)
{
	NET_BUF_SIMPLE_DEFINE(msg, 2 + 1 + 4);
	led_state_t old_state;

	board_led_get(LED_ALIAS_BULB, &old_state);

	LOG_INF("addr 0x%04x current state %s", bt_mesh_model_elem(model)->addr, (LED_STATE_ON == old_state) ? "on" : "off");

	bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_OP_2(0x82, 0x04));
	net_buf_simple_add_u8(&msg, (LED_STATE_ON == old_state) ? 0x01 : 0x00);
	bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
}

static void gen_onoff_set_unack(struct bt_mesh_model *model,
								struct bt_mesh_msg_ctx *ctx,
								struct net_buf_simple *buf)
{
	struct net_buf_simple *msg;
	led_state_t old_state, new_state;

	new_state = (0x01 == net_buf_simple_pull_u8(buf)) ? LED_STATE_ON : LED_STATE_OFF;

	LOG_INF("addr 0x%04x set to %s", bt_mesh_model_elem(model)->addr, (LED_STATE_ON == new_state) ? "on" : "off");

	board_led_get(LED_ALIAS_BULB, &old_state);
	if (old_state == new_state) {
		LOG_INF("no need to update");
		return;
	}
	board_led_set(LED_ALIAS_BULB, new_state);

	/*
	 * If a server has a publish address, it is required to
	 * publish status on a state change
	 *
	 * See Mesh Profile Specification 3.7.6.1.2
	 *
	 * Only publish if there is an assigned address
	 */
	if (BT_MESH_ADDR_UNASSIGNED != model->pub->addr) {
		LOG_INF("publish last %s cur %s",
				(LED_STATE_ON == old_state) ? "on" : "off",
				(LED_STATE_ON == new_state) ? "on" : "off");
		msg = model->pub->msg;
		bt_mesh_model_msg_init(msg, BT_MESH_MODEL_OP_2(0x82, 0x04));
		net_buf_simple_add_u8(msg, (LED_STATE_ON == new_state) ? 0x01 : 0x00);
		bt_mesh_model_publish(model);
	}
}

static void gen_onoff_set(struct bt_mesh_model *model,
						struct bt_mesh_msg_ctx *ctx,
						struct net_buf_simple *buf)
{
	gen_onoff_set_unack(model, ctx, buf);
	gen_onoff_get(model, ctx, buf);
}

static const struct bt_mesh_model_op gen_onoff_op[] = {
	{ BT_MESH_MODEL_OP_2(0x82, 0x01), 0, gen_onoff_get },
	{ BT_MESH_MODEL_OP_2(0x82, 0x02), 2, gen_onoff_set },
	{ BT_MESH_MODEL_OP_2(0x82, 0x03), 2, gen_onoff_set_unack },
	BT_MESH_MODEL_OP_END,
};

static struct bt_mesh_model root_models[] = {
	BT_MESH_MODEL_CFG_SRV(&cfg_srv),
	BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
	BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, gen_onoff_op, &gen_onoff_pub, NULL),
};

static struct bt_mesh_elem elements[] = {
	BT_MESH_ELEM(0, root_models, BT_MESH_MODEL_NONE),
};

static const struct bt_mesh_comp comp = {
	.cid = BT_COMP_ID_LF,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

static int output_number(bt_mesh_output_action_t action, u32_t number)
{
	LOG_INF("OOB Number: %u", number);

	return 0;
}

static void prov_complete(u16_t net_idx, u16_t addr)
{
	LOG_INF("Provisionee Index: %u", net_idx);
}

static void prov_reset(void)
{
	bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
}

static const uint8_t dev_uuid[16] = { 0xdd, 0xdd };

static const struct bt_mesh_prov prov = {
	.uuid = dev_uuid,
	.output_size = 4,
	.output_actions = BT_MESH_DISPLAY_NUMBER,
	.output_number = output_number,
	.complete = prov_complete,
	.reset = prov_reset,
};

static void bt_ready(int err)
{
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}

	LOG_INF("Bluetooth initialized");

	err = bt_mesh_init(&prov, &comp);
	if (err) {
		LOG_ERR("Initializing mesh failed (err %d)", err);
		return;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	/* This will be a no-op if settings_load() loaded provisioning info */
	bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);

	LOG_INF("Mesh initialized");
}

void main(void)
{
	int err;

	LOG_INF("Initializing ...");
	board_init();

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
	}

	while (1) {
		board_led_trigger(LED_ALIAS_INDICATOR);
		k_sleep(1000);
	}
}
