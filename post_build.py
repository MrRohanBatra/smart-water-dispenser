import os
import shutil
import subprocess
import logging
from SCons.Script import Import

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

# Define paths
PLATFORMIO_BUILD_DIR = os.path.join(os.getcwd(), ".pio", "build", "esp32dev")  # Adjust for your environment
PROJECT_NAME = "smart-water-dispenser"  # Change this to your project name
SERVER_DIR = os.path.join("C:/server/files", PROJECT_NAME)
ALLOWED_FILES = {"spiffs.bin", "firmware.bin"}

# Import PlatformIO build environment
Import("env")

def build_fs():
    logging.info("Building filesystem image...")
    result = subprocess.run(["pio", "run", "--target", "buildfs"], capture_output=True, text=True)
    logging.info(result.stdout)
    if result.stderr:
        logging.error(result.stderr)

def copy_files(source, target, env):
    logging.info("Building SPIFFS before copying files...")
    build_fs()
    
    logging.info("Copying files...")
    os.makedirs(SERVER_DIR, exist_ok=True)
    
    for filename in ALLOWED_FILES:
        src_path = os.path.join(PLATFORMIO_BUILD_DIR, filename)
        dest_path = os.path.join(SERVER_DIR, filename)
        
        if os.path.exists(src_path):
            shutil.copy(src_path, dest_path)
            logging.info(f"Copied {filename} to {SERVER_DIR}")
        else:
            logging.warning(f"File {filename} not found in {PLATFORMIO_BUILD_DIR}")

# Register post-action hooks
env.AddPostAction("$BUILD_DIR/firmware.bin", copy_files)
