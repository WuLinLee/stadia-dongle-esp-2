import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import test from "node:test";

import { ACTION_NAMES, CAPABILITY, COMMAND, STATUS, USB_CONFIG } from "../main/webhid_transport.js";

async function source(path) {
  return readFile(new URL(path, import.meta.url), "utf8");
}

test("JavaScript command values match the firmware protocol header", async () => {
  const header = await source("../main/usb_config_protocol.h");
  const expected = {
    USB_CONFIG_CMD_PING: COMMAND.ping,
    USB_CONFIG_CMD_GET_DEVICE_INFO: COMMAND.getDeviceInfo,
    USB_CONFIG_CMD_GET_STATUS: COMMAND.getStatus,
    USB_CONFIG_CMD_GET_CONFIG: COMMAND.getConfig,
    USB_CONFIG_CMD_SET_CONFIG: COMMAND.setConfig,
    USB_CONFIG_CMD_GET_CONTROLLER_COUNT: COMMAND.getControllerCount,
    USB_CONFIG_CMD_GET_CONTROLLER: COMMAND.getController,
    USB_CONFIG_CMD_GET_INPUT: COMMAND.getInput,
    USB_CONFIG_CMD_START_PAIRING: COMMAND.startPairing,
    USB_CONFIG_CMD_STOP_PAIRING: COMMAND.stopPairing,
    USB_CONFIG_CMD_FORGET_CONTROLLER: COMMAND.forgetController,
    USB_CONFIG_CMD_FORGET_ALL: COMMAND.forgetAll,
    USB_CONFIG_CMD_START_WIFI: COMMAND.startWifi,
    USB_CONFIG_CMD_STOP_WIFI: COMMAND.stopWifi,
    USB_CONFIG_CMD_REBOOT: COMMAND.reboot,
  };
  for (const [name, value] of Object.entries(expected)) {
    const match = header.match(new RegExp(`${name}\\s*=\\s*(0x[0-9a-f]+|\\d+)`, "i"));
    assert.ok(match, `${name} is declared`);
    assert.equal(Number(match[1]), value, `${name} matches JavaScript`);
  }
  const notReady = header.match(/USB_CONFIG_STATUS_NOT_READY\s*=\s*(\d+)/);
  assert.ok(notReady, "USB_CONFIG_STATUS_NOT_READY is declared");
  assert.equal(
    Number(notReady[1]),
    STATUS.notReady,
    "NOT_READY matches JavaScript",
  );
  const capabilities = {
    USB_CONFIG_CAP_KEYMAP: CAPABILITY.keymap,
    USB_CONFIG_CAP_CONTROLLERS: CAPABILITY.controllers,
    USB_CONFIG_CAP_LIVE_INPUT: CAPABILITY.liveInput,
    USB_CONFIG_CAP_PAIRING: CAPABILITY.pairing,
    USB_CONFIG_CAP_FORGET: CAPABILITY.forget,
    USB_CONFIG_CAP_WIFI_CONTROL: CAPABILITY.wifiControl,
    USB_CONFIG_CAP_REBOOT: CAPABILITY.reboot,
    USB_CONFIG_CAP_BLE_READINESS: CAPABILITY.bleReadiness,
  };
  for (const [name, value] of Object.entries(capabilities)) {
    const match = header.match(
      new RegExp(`${name}\\s*=\\s*1u\\s*<<\\s*(\\d+)`),
    );
    assert.ok(match, `${name} is declared`);
    assert.equal(
      1 << Number(match[1]),
      value,
      `${name} matches JavaScript`,
    );
  }
});

test("action names stay in the same enum order as config_store", async () => {
  const store = await source("../main/config_store.c");
  const cases = [...store.matchAll(/case\s+DONGLE_ACTION_[A-Z0-9_]+:\s+return\s+"([^"]+)"/g)];
  assert.deepEqual(
    cases.map((match) => match[1]),
    ACTION_NAMES,
  );
});

test("utility HID descriptor advertises one 63-byte vendor feature report", async () => {
  const hidSource = await source("../main/hid_extra.c");
  const header = await source("../main/hid_extra.h");
  const descriptorMatch = hidSource.match(
    /s_hid_report_desc\[HID_EXTRA_REPORT_DESC_LEN\]\s*=\s*\{([\s\S]*?)\};/,
  );
  assert.ok(descriptorMatch, "HID report descriptor is present");
  const constants = new Map([
    ["RID_KEYBOARD", 1],
    ["RID_CONSUMER", 2],
    ["USB_CONFIG_REPORT_ID", USB_CONFIG.reportId],
    ["USB_CONFIG_REPORT_DATA_SIZE", USB_CONFIG.reportSize],
  ]);
  const bytes = descriptorMatch[1]
    .split(",")
    .map((token) => token.trim())
    .filter(Boolean)
    .map((token) => constants.get(token) ?? Number(token));
  assert.ok(bytes.every(Number.isFinite), "descriptor contains parseable bytes");
  const declaredLength = Number(
    header.match(/HID_EXTRA_REPORT_DESC_LEN\s+(\d+)/)[1],
  );
  assert.equal(bytes.length, declaredLength);
  assert.deepEqual(bytes.slice(-23), [
    0x06,
    0x00,
    0xff,
    0x09,
    0x01,
    0xa1,
    0x01,
    0x85,
    USB_CONFIG.reportId,
    0x15,
    0x00,
    0x26,
    0xff,
    0x00,
    0x09,
    0x02,
    0x75,
    0x08,
    0x95,
    USB_CONFIG.reportSize,
    0xb1,
    0x02,
    0xc0,
  ]);
});

test("localhost remains available for an offline USB configuration page", async () => {
  const index = await source("../main/index.html");
  assert.match(index, /new Set\(\["localhost", "127\.0\.0\.1", "::1"\]\)/);
  assert.match(index, /async function detectDevicePage\(\)/);
  assert.match(index, /fetch\("\/api\/status"/);
  assert.match(index, /__STADIA_DEVICE_PAGE__/);
  assert.doesNotMatch(index, /location\.hostname === "192\.168\.4\.1"/);
});

test("configuration page is Chinese and exposes firmware mapping defaults", async () => {
  const index = await source("../main/index.html");
  assert.match(index, /<html lang="zh-CN">/);
  assert.match(index, /<h2>额外按键<\/h2>/);
  assert.match(index, /assistant_action:\s*"f14"/);
  assert.match(index, /capture_action:\s*"f12"/);
  assert.doesNotMatch(index, /assistant_short|assistant_long|capture_short|capture_long|long_press_ms/);
  assert.match(index, /Assistant \+ Capture<\/b> 2 秒/);
  assert.match(index, /webui_timeout_seconds:\s*120/);
  assert.match(index, /id="restoreDefaults"/);
  assert.match(index, /applyConfig\(defaultMapping\)/);
  assert.match(index, /（固件默认）/);
  assert.doesNotMatch(index, /"start_webui"/);
});

test("Assistant/Capture outputs stay held until physical key-up", async () => {
  const buttons = await source("../main/button_actions.c");
  const hid = await source("../main/hid_extra.c");
  assert.match(buttons, /hid_extra_set_keyboard_slot\(slot, 0, key, pressed\)/);
  assert.match(buttons, /bridge_set_virtual_gamepad_button\(gamepad_index, gamepad_button, true\)/);
  assert.match(buttons, /bridge_set_virtual_gamepad_button\(gamepad_index, gamepad_button, false\)/);
  assert.doesNotMatch(buttons, /vTaskDelay\(pdMS_TO_TICKS\(20\)\)/);
  assert.match(buttons, /now - setup_combo_start_us\[idx\] > 5000000/);
  assert.match(buttons, /mouse_combo_start_us\[i\] > 2000000/);
  assert.match(hid, /0x75, 0x10, 0x95, 0x04, 0x81, 0x00/);
  assert.match(hid, /Keep logical held-key state across USB bus resets/);
  const ble = await source("../main/ble_central.c");
  assert.match(ble, /const stadia_controller_state_t released_state = \{0\};[\s\S]*?button_actions_on_state\(usb, &released_state\)/);
  assert.match(ble, /ble_gattc_write_flat\(slot->conn_handle, slot->output_val_handle,[\s\S]*?slot->rumble_payload,[\s\S]*?rumble_write_fn, slot\)/);
  assert.doesNotMatch(ble, /BLE_RUMBLE_RETRY_BASE_MS|rumble_retry_delay_ms|rumble_dirty|rumble_write_in_flight/);
  const xbox = await source("../main/xbox_dev.c");
  assert.match(xbox, /\.data = \{0x00, gp->out_buf\[3\], 0x00, gp->out_buf\[4\]\}/);
  assert.doesNotMatch(xbox, /n \* 257/);
  assert.match(buttons, /The chord must stay continuously held for two seconds/);
  const bridge = await source("../main/bridge.c");
  assert.match(bridge, /s_base_report_neutral\[gamepad_index\]/);
  assert.match(bridge, /if \(!s_base_report_neutral\[gamepad_index\]\)\s*\{\s*apply_virtual_buttons/);
  assert.doesNotMatch(bridge, /s_virtual_buttons\[gamepad_index\]\s*=\s*0;/);
});

test("Wi-Fi AP is opt-in, temporary, and still recoverable", async () => {
  const store = await source("../main/config_store.c");
  const appMain = await source("../main/main.c");
  const webServer = await source("../main/web_server.c");
  assert.match(store, /CFG_SCHEMA\s+7/);
  assert.match(store, /capture_action\s*=\s*DONGLE_ACTION_KEY_F12/);
  assert.match(store, /legacy_capture_long_action\s*=\s*DONGLE_ACTION_NONE/);
  assert.match(store, /webui_auto_start_if_no_bond\s*=\s*false/);
  assert.match(
    store,
    /legacy_assistant_long_action\s*==\s*DONGLE_ACTION_START_WEBUI[\s\S]*?legacy_assistant_long_action\s*=\s*DONGLE_ACTION_NONE/,
  );
  assert.equal(
    [...appMain.matchAll(/web_server_start\(true\)/g)].length,
    2,
    "only the three-boot and five-boot recovery paths start Wi-Fi in main.c",
  );
  assert.match(appMain, /boots\s*>=\s*3[\s\S]*?web_server_start\(true\)/);
  assert.match(
    webServer,
    /if\s*\(!explicit_request\)[\s\S]*?webui_auto_start_if_no_bond[\s\S]*?controller_manager_bond_count\(\)\s*!=\s*0/,
  );
  assert.doesNotMatch(webServer, /any_controller_connected/);
});

test("F12 is appended compatibly and emits the standard keyboard usage", async () => {
  const header = await source("../main/config_store.h");
  const actions = await source("../main/button_actions.c");
  assert.match(
    header,
    /DONGLE_ACTION_CONSUMER_SCAN_PREV,\s*DONGLE_ACTION_KEY_F12,/,
  );
  assert.equal(ACTION_NAMES.indexOf("f12"), 37);
  assert.equal(ACTION_NAMES.at(-1), "gamepad_rt");
  assert.match(header, /DONGLE_ACTION_KEY_F12,\s*DONGLE_ACTION_GAMEPAD_A,/);
  assert.match(actions, /DONGLE_ACTION_KEY_F12:\s*return\s*0x45/);
  const bridge = await source("../main/bridge.c");
  const buttons = await source("../main/button_actions.c");
  assert.match(buttons, /DONGLE_ACTION_GAMEPAD_A:\s*return BRIDGE_GAMEPAD_BUTTON_A/);
  assert.match(bridge, /report\[3\] \|= 1u << 4/);
  assert.match(bridge, /s_base_report_neutral\[gamepad_index\] = true/);
  assert.doesNotMatch(bridge, /s_virtual_buttons\[gamepad_index\] = 0/);
});

test("controller polling preserves interactive nodes and gates capabilities", async () => {
  const index = await source("../main/index.html");
  assert.match(index, /function createControllerElement\(key\)/);
  assert.match(index, /host\.insertBefore\(row/);
  assert.doesNotMatch(index, /host\.appendChild\(row\)/);
  assert.match(index, /data-capability="forget"/);
  assert.match(index, /controllerPollDue/);
  assert.match(index, /min-height:44px/);
  assert.doesNotMatch(index, /\$\("controllers"\)\.innerHTML\s*=\s*list\.map/);
});

test("firmware defers Bluetooth commands until the controller manager is ready", async () => {
  const protocol = await source("../main/usb_config_protocol.c");
  const server = await source("../main/web_server.c");
  assert.match(protocol, /command_needs_controller_manager/);
  assert.match(protocol, /USB_CONFIG_STATUS_NOT_READY/);
  assert.match(server, /void web_server_request_stop\(void\)/);
  assert.match(server, /controller_manager_not_ready/);
  assert.match(protocol, /web_server_request_stop\(\)/);
});
