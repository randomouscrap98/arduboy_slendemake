import arduboy.image
import os

from PIL import Image

config = arduboy.image.TileConfig(
    width = 16,
    height = 16,
    spacing = 0,
    use_mask = True,
    separate_header_mask = True,
    add_dimensions = False
)

tilename = "tilesheet"
spritename = "spritesheet"

# The images to convert
tilesheet = os.path.join("..", "resources", f"{tilename}.png")
spritesheet = os.path.join("..", "resources", f"{spritename}.png")

# The headers to write
tileout = os.path.join("..", f"{tilename}.h")
spriteout = os.path.join("..", f"{spritename}.h")

print("Loading images")
tileimage = Image.open(tilesheet)
spriteimage = Image.open(spritesheet)

print("Validating images + config")
arduboy.image.validate_tileconfig_code(config, tileimage)
arduboy.image.validate_tileconfig_code(config, spriteimage)

print("Converting images")
tilecode, _ = arduboy.image.convert_image(tileimage, tilename, config)
spritecode , _ = arduboy.image.convert_image(spriteimage, spritename, config)

print("Writing code")
with open(tileout, "w") as f:
    f.write(arduboy.image.IMAGEHEADER_PREAMBLE + tilecode)
with open(spriteout, "w") as f:
    f.write(arduboy.image.IMAGEHEADER_PREAMBLE + spritecode)

print("Done!")

