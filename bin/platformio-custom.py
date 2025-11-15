#!/usr/bin/env python3
# trunk-ignore-all(ruff/F821)
# trunk-ignore-all(flake8/F821): For SConstruct imports
import sys
from os.path import join
import subprocess
import json
import re
import time
from datetime import datetime

from readprops import readProps

Import("env")
platform = env.PioPlatform()

# --- KODE OS MODIFICATION START ---
# We override the standard Meshtastic binary merging logic because
# Kode OS handles the bootloader and partition table. We only need
# the application binary to be flashed at 0x400000.

def kode_os_post_process(source, target, env):
    print("--- Kode OS: Post-processing for App Build ---")
    
    firmware_name = env.subst("$BUILD_DIR/${PROGNAME}.bin")
    
    # NOTE: The actual offset handling is usually done in the linker script 
    # or partition table (partitions.csv), but we can rename the file here
    # to make it clear this is a Kode App.
    
    print(f"firmware binary: {firmware_name}")
    print("Ready for upload to 0x400000 via custom upload command in platformio.ini")

# We disable the standard esp32_create_combined_bin for Kode OS builds
# to prevent overwriting the custom bootloader/partitions of Kode OS.
# --- KODE OS MODIFICATION END ---

if platform.name == "espressif32":
    sys.path.append(join(platform.get_package_dir("tool-esptoolpy")))
    import esptool

    # KODE OS: Changed from esp32_create_combined_bin to custom handler
    # or simply do nothing if we just want the raw .bin
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", kode_os_post_process)

    esp32_kind = env.GetProjectOption("custom_esp32_kind")
    if esp32_kind == "esp32":
        # Free up some IRAM by removing auxiliary SPI flash chip drivers.
        # Wrapped stub symbols are defined in src/platform/esp32/iram-quirk.c.
        env.Append(
            LINKFLAGS=[
                "-Wl,--wrap=esp_flash_chip_gd",
                "-Wl,--wrap=esp_flash_chip_issi",
                "-Wl,--wrap=esp_flash_chip_winbond",
            ]
        )
    else:
        # For newer ESP32 targets, using newlib nano works better.
        env.Append(LINKFLAGS=["--specs=nano.specs", "-u", "_printf_float"])

if platform.name == "nordicnrf52":
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.hex",
                      env.VerboseAction(f"\"{sys.executable}\" ./bin/uf2conv.py \"$BUILD_DIR/firmware.hex\" -c -f 0xADA52840 -o \"$BUILD_DIR/firmware.uf2\"",
                                        "Generating UF2 file"))

Import("projenv")

prefsLoc = projenv["PROJECT_DIR"] + "/version.properties"
verObj = readProps(prefsLoc)
print("Using meshtastic platformio-custom.py, firmware version " + verObj["long"] + " on " + env.get("PIOENV"))

# get repository owner if git is installed
try:
    r_owner = (
        subprocess.check_output(["git", "config", "--get", "remote.origin.url"])
        .decode("utf-8")
        .strip().split("/")
    )
    repo_owner = r_owner[-2] + "/" + r_owner[-1].replace(".git", "")
except subprocess.CalledProcessError:
    repo_owner = "unknown"

jsonLoc = env["PROJECT_DIR"] + "/userPrefs.jsonc"
# Handle case where userPrefs might not exist or fail to parse
try:
    with open(jsonLoc) as f:
        jsonStr = re.sub("//.*","", f.read(), flags=re.MULTILINE)
        userPrefs = json.loads(jsonStr)
except Exception as e:
    print(f"Warning: Could not load userPrefs.jsonc: {e}")
    userPrefs = {}

pref_flags = []
# Pre-process the userPrefs
for pref in userPrefs:
    if userPrefs[pref].startswith("{"):
        pref_flags.append("-D" + pref + "=" + userPrefs[pref])
    elif userPrefs[pref].lstrip("-").replace(".", "").isdigit():
        pref_flags.append("-D" + pref + "=" + userPrefs[pref])
    elif userPrefs[pref] == "true" or userPrefs[pref] == "false":
        pref_flags.append("-D" + pref + "=" + userPrefs[pref])
    elif userPrefs[pref].startswith("meshtastic_"):
        pref_flags.append("-D" + pref + "=" + userPrefs[pref])
    # If the value is a string, we need to wrap it in quotes
    else:
        pref_flags.append("-D" + pref + "=" + env.StringifyMacro(userPrefs[pref]) + "")

# General options that are passed to the C and C++ compilers
# Calculate unix epoch for current day (midnight)
current_date = datetime.now().replace(hour=0, minute=0, second=0, microsecond=0)
build_epoch = int(current_date.timestamp())

flags = [
        "-DAPP_VERSION=" + verObj["long"],
        "-DAPP_VERSION_SHORT=" + verObj["short"],
        "-DAPP_ENV=" + env.get("PIOENV"),
        "-DAPP_REPO=" + repo_owner,
        "-DBUILD_EPOCH=" + str(build_epoch),
    ] + pref_flags

print ("Using flags:")
for flag in flags:
    print(flag)
    
projenv.Append(
    CCFLAGS=flags,
)

for lb in env.GetLibBuilders():
    if lb.name == "meshtastic-device-ui":
        lb.env.Append(CPPDEFINES=[("APP_VERSION", verObj["long"])])
        break

# Get the display resolution from macros
def get_display_resolution(build_flags):
    # Check "DISPLAY_SIZE" to determine the screen resolution
    for flag in build_flags:
        if isinstance(flag, tuple) and flag[0] == "DISPLAY_SIZE":
            screen_width, screen_height = map(int, flag[1].split("x"))
            return screen_width, screen_height
    # Default fallback if not found to prevent crash
    return 240, 240 

def load_boot_logo(source, target, env):
    build_flags = env.get("CPPDEFINES", [])
    logo_w, logo_h = get_display_resolution(build_flags)
    print(f"TFT build with {logo_w}x{logo_h} resolution detected")

    # Load the boot logo from `branding/logo_<width>x<height>.png` if it exists
    source_path = join(env["PROJECT_DIR"], "branding", f"logo_{logo_w}x{logo_h}.png")
    dest_dir = join(env["PROJECT_DIR"], "data", "boot")
    dest_path = join(dest_dir, "logo.png")
    if env.File(source_path).exists():
        print(f"Loading boot logo from {source_path}")
        # Prepare the destination
        env.Execute(f"mkdir -p {dest_dir} && rm -f {dest_path}")
        # Copy the logo to the `data/boot` directory
        env.Execute(f"cp {source_path} {dest_path}")

# Load the boot logo on TFT builds
if ("HAS_TFT", 1) in env.get("CPPDEFINES", []):
    # Check if littlefs.bin target exists before adding action
    env.AddPreAction('$BUILD_DIR/littlefs.bin', load_boot_logo)