import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import test from "node:test";
async function source(path) { return readFile(new URL(path, import.meta.url), "utf8"); }

test("three gamma profiles are wired into the existing XInput-to-BLE rumble path", async () => {
  const mapping = await source("../main/rumble_map.c");
  const header = await source("../main/rumble_map.h");
  const component = await source("../main/CMakeLists.txt");
  const xbox = await source("../main/xbox_dev.c");
  const workflow = await source("../.github/workflows/release.yml");
  assert.match(mapping, /gamma = 0\.90f/);
  assert.match(mapping, /gamma = 0\.78f/);
  assert.match(mapping, /input_8 == 0/);
  assert.match(mapping, /input_8 == 255/);
  assert.match(mapping, /powf\(normalized, gamma\)/);
  assert.match(header, /#define STADIA_RUMBLE_PROFILE 0/);
  assert.match(component, /"rumble_map\.c"/);
  assert.match(component, /target_compile_definitions\(\$\{COMPONENT_LIB\} PRIVATE STADIA_RUMBLE_PROFILE=\$\{STADIA_RUMBLE_PROFILE\}\)/);
  assert.match(xbox, /rumble_map_xbox360\(gp->out_buf\[3\], gp->out_buf\[4\], msg\.data\)/);
  assert.doesNotMatch(xbox, /\.data = \{gp->out_buf\[3\], gp->out_buf\[3\]/);
  assert.match(workflow, /STADIA_RUMBLE_PROFILE=\$profile_id/);
  for (const name of ["reference", "gentle", "punch"]) assert.ok(workflow.includes(`build-${name}/firmware-${name}-merged.bin`));
});

test("install manifests each select the matching full image", async () => {
  const landing = await source("../docs/index.html");
  for (const name of ["reference", "gentle", "punch"]) {
    const manifest = JSON.parse(await source(`../docs/manifest-${name}.json`));
    assert.equal(manifest.builds[0].chipFamily, "ESP32-S3");
    assert.equal(manifest.builds[0].parts[0].offset, 0);
    assert.equal(manifest.builds[0].parts[0].path, `firmware-${name}-merged.bin`);
    assert.match(landing, new RegExp(`manifest-${name}\\.json`));
    assert.match(landing, new RegExp(`firmware-${name}-merged\\.bin`));
  }
  const legacy = JSON.parse(await source("../docs/manifest.json"));
  assert.equal(legacy.builds[0].parts[0].path, "firmware-merged.bin");
});

test("release and Pages keep publishing the three curves and legacy reference alias", async () => {
  const workflow = await source("../.github/workflows/release.yml");
  assert.match(workflow, /path: firmware-output\/\*\.bin/);
  assert.match(workflow, /files: release\/\*\.bin/);
  assert.match(workflow, /path: docs\//);
  assert.match(workflow, /cp build-reference\/firmware-reference-merged\.bin firmware-output\/firmware-merged\.bin/);
});
