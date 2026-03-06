import os

from PIL import Image

INPUT_IMAGE_PATH = "logo.png"

OUTPUT_HEADER_PATH = "driver/video/splash.h"


def convert_image_to_header(image_path, output_path):
    try:
        img = Image.open(image_path).convert("RGB")
        width, height = img.size
        pixels = img.load()

        print(f"Loaded {image_path} ({width}x{height})")

        os.makedirs(os.path.dirname(output_path), exist_ok=True)

        with open(output_path, "w") as f:
            f.write("#pragma once\n\n")
            f.write("#include <misc/types.h>\n\n")

            f.write(f"#define LOGO_WIDTH  {width}\n")
            f.write(f"#define LOGO_HEIGHT {height}\n\n")

            f.write("const uint8_t splash_logo[] = {\n")

            for y in range(height):
                f.write("    ")
                for x in range(width):
                    r, g, b = pixels[x, y]
                    if (r + g + b) / 3 < 0x30:
                        r = 0
                        g = 0
                        b = 0
                    f.write(f"0x{r:02X}, 0x{g:02X}, 0x{b:02X}, ")
                f.write("\n")

            f.write("};\n")

        print(f"Successfully generated {output_path}!")

    except FileNotFoundError:
        print(f"Error: Could not find the input image '{image_path}'")
    except Exception as e:
        print(f"An error occurred: {e}")


if __name__ == "__main__":
    convert_image_to_header(INPUT_IMAGE_PATH, OUTPUT_HEADER_PATH)
