import arduboy.arduhex
import arduboy.fxcart
import arduboy.image
import arduboy.serial

from arduboy.common import *
from arduboy.constants import *
from serial import Serial

import logging
import datetime
from PIL import Image

# NOTE: this is strictly higher level than any other file! Do NOT include this in any 
# arduboy library files, it is specifically for external use!

def slot_from_category(title: str, info : str = "", image : Image = None, category_id : int = 0) -> arduboy.fxcart.FxParsedSlot:
    """Create an FxParsedSlot from category information."""
    return arduboy.fxcart.FxParsedSlot(
        category_id,
        arduboy.image.pilimage_to_bin(image) if image else bytearray(SCREEN_BYTES),
        bytearray(),
        bytearray(),
        bytearray(),
        arduboy.fxcart.FxSlotMeta(title, "", "", info)
    )

# Given a parsed arduhex file, generate a reasonable slot file. You MUST specify which binary should be used!
def slot_from_arduboy(parsed: arduboy.arduhex.ArduboyParsed, binary: arduboy.arduhex.ArduboyBinary) -> arduboy.fxcart.FxParsedSlot:
    """Create an FxParsedSlot from the given arduboy data. 
    
    You must also pass the binary to use, since the arduboy data has multiple binaries. The binaries in
    the arduboy data are fully ignored, only the binary passed in is used.
    """
    return arduboy.fxcart.FxParsedSlot(
        0, # Might not matter
        arduboy.image.pilimage_to_bin(binary.cartImage) if binary.cartImage else bytearray(SCREEN_BYTES),
        # Always trim data just in case
        arduboy.arduhex.analyze_sketch(arduboy.common.hex_to_bin(binary.hex_raw)).trimmed_data,
        binary.data_raw,
        binary.save_raw,
        arduboy.fxcart.FxSlotMeta(parsed.title if parsed.title else parsed.original_filename, parsed.version, parsed.author, parsed.description)
    )

def arduboy_from_slot(slot: arduboy.fxcart.FxParsedSlot, device: str) -> arduboy.arduhex.ArduboyParsed:
    bindevice = device if slot.fx_enabled() else arduboy.arduhex.DEVICE_ARDUBOY
    return arduboy.arduhex.ArduboyParsed(
        "unknown.arduboy",
        [
            arduboy.arduhex.ArduboyBinary(
                bindevice,
                "", # Just dont trust the titles. TODO: May change later
                arduboy.common.bin_to_hex(arduboy.arduhex.analyze_sketch(slot.program_raw).trimmed_data),
                slot.data_raw,
                slot.save_raw,
                arduboy.image.bin_to_pilimage(slot.image_raw)
            )
        ],
        [],
        slot.meta.title,
        slot.meta.version,
        slot.meta.developer,
        slot.meta.info,
        datetime.datetime.now().strftime("%Y/%m/%d")
    )


def detect_device_type(s_port: Serial):
    """
    Using (perhaps faulty) logic, attempt to figure out what kind of device is connected. This function may
    take some time, as it has to read from the device. Must be in bootloader, as usual!
    """
    logging.info(f"Detecting device on: {s_port.name}")
    bootloader = arduboy.serial.read_bootloader(s_port)
    analysis = arduboy.arduhex.analyze_sketch(bootloader, bootloader=True)
    logging.debug(f"Device on {s_port.name} is: {analysis.detected_device}")
    return analysis.detected_device