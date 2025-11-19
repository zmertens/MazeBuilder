"""
Script to invert the colors of a PNG image with various inversion options.
Supports multiple color inversion methods including RGB, HSV, and selective channel inversions.
"""

import argparse
import sys
from pathlib import Path
from PIL import Image, ImageOps
import numpy as np

class ImageColorInverter:
    """Class to handle various types of color inversions on images."""
    
    def __init__(self, image_path):
        """Initialize with image path."""
        self.image_path = Path(image_path)
        self.image = None
        self.load_image()
    
    def load_image(self):
        """Load the image from file."""
        if not self.image_path.exists():
            raise FileNotFoundError(f"Image file not found: {self.image_path}")
        
        try:
            self.image = Image.open(self.image_path)
            print(f"Loaded image: {self.image_path}")
            print(f"Image mode: {self.image.mode}, Size: {self.image.size}")
        except Exception as e:
            raise ValueError(f"Error loading image: {e}")
    
    def simple_invert(self):
        """Simple color inversion using PIL's built-in function."""
        if self.image.mode == 'P':
            # Handle palette mode images
            if 'transparency' in self.image.info:
                # Convert to RGBA to preserve transparency
                rgba_image = self.image.convert('RGBA')
                r, g, b, a = rgba_image.split()
                rgb_image = Image.merge('RGB', (r, g, b))
                inverted_rgb = ImageOps.invert(rgb_image)
                r_inv, g_inv, b_inv = inverted_rgb.split()
                inverted_image = Image.merge('RGBA', (r_inv, g_inv, b_inv, a))
            else:
                # Convert to RGB and invert
                rgb_image = self.image.convert('RGB')
                inverted_image = ImageOps.invert(rgb_image)
        elif self.image.mode in ('RGBA', 'LA'):
            # For images with alpha channel, split and invert only RGB channels
            if self.image.mode == 'RGBA':
                r, g, b, a = self.image.split()
                rgb_image = Image.merge('RGB', (r, g, b))
                inverted_rgb = ImageOps.invert(rgb_image)
                r_inv, g_inv, b_inv = inverted_rgb.split()
                inverted_image = Image.merge('RGBA', (r_inv, g_inv, b_inv, a))
            else:  # LA mode
                l, a = self.image.split()
                inverted_l = ImageOps.invert(Image.merge('L', [l]))
                inverted_image = Image.merge('LA', (inverted_l, a))
        else:
            inverted_image = ImageOps.invert(self.image)
        
        return inverted_image
    
    def rgb_channel_invert(self, invert_red=True, invert_green=True, invert_blue=True):
        """Invert specific RGB channels."""
        if self.image.mode == 'P':
            # Handle palette mode by converting to appropriate format
            if 'transparency' in self.image.info:
                working_image = self.image.convert('RGBA')
            else:
                working_image = self.image.convert('RGB')
        elif self.image.mode not in ('RGB', 'RGBA'):
            # Convert to RGB if needed
            working_image = self.image.convert('RGBA' if self.image.mode in ('RGBA', 'LA') else 'RGB')
        else:
            working_image = self.image.copy()
        
        # Convert to numpy array for easier manipulation
        img_array = np.array(working_image)
        
        # Invert specific channels
        if invert_red and len(img_array.shape) >= 3:
            img_array[:, :, 0] = 255 - img_array[:, :, 0]
        if invert_green and len(img_array.shape) >= 3:
            img_array[:, :, 1] = 255 - img_array[:, :, 1]
        if invert_blue and len(img_array.shape) >= 3:
            img_array[:, :, 2] = 255 - img_array[:, :, 2]
        
        return Image.fromarray(img_array)
    
    def hsv_invert(self, invert_hue=False, invert_saturation=False, invert_value=True):
        """Invert image in HSV color space."""
        # Convert to HSV
        if self.image.mode == 'P':
            # Handle palette mode
            if 'transparency' in self.image.info:
                rgba_image = self.image.convert('RGBA')
                rgb_image = rgba_image.convert('RGB')
                alpha = rgba_image.split()[-1]
                has_alpha = True
            else:
                rgb_image = self.image.convert('RGB')
                has_alpha = False
        elif self.image.mode == 'RGBA':
            rgb_image = self.image.convert('RGB')
            alpha = self.image.split()[-1]
            has_alpha = True
        else:
            rgb_image = self.image.convert('RGB')
            has_alpha = False
        
        hsv_image = rgb_image.convert('HSV')
        hsv_array = np.array(hsv_image)
        
        # Invert specific HSV components
        if invert_hue:
            hsv_array[:, :, 0] = 255 - hsv_array[:, :, 0]
        if invert_saturation:
            hsv_array[:, :, 1] = 255 - hsv_array[:, :, 1]
        if invert_value:
            hsv_array[:, :, 2] = 255 - hsv_array[:, :, 2]
        
        # Convert back to RGB
        inverted_hsv = Image.fromarray(hsv_array, 'HSV')
        inverted_rgb = inverted_hsv.convert('RGB')
        
        if has_alpha:
            inverted_rgba = inverted_rgb.convert('RGBA')
            # Restore alpha channel
            inverted_array = np.array(inverted_rgba)
            alpha_array = np.array(alpha)
            inverted_array[:, :, 3] = alpha_array
            return Image.fromarray(inverted_array, 'RGBA')
        
        return inverted_rgb
    
    def negative_invert(self):
        """Create a photographic negative effect."""
        return self.simple_invert()
    
    def luminance_invert(self):
        """Invert only the luminance while preserving color information."""
        if self.image.mode == 'P':
            # Handle palette mode by converting to RGB
            working_image = self.image.convert('RGB')
        elif self.image.mode not in ('RGB', 'RGBA'):
            working_image = self.image.convert('RGB')
        else:
            working_image = self.image.copy()
        
        # Convert to numpy array
        img_array = np.array(working_image)
        
        # Calculate luminance (grayscale)
        if len(img_array.shape) == 3 and img_array.shape[2] >= 3:
            luminance = 0.299 * img_array[:, :, 0] + 0.587 * img_array[:, :, 1] + 0.114 * img_array[:, :, 2]
            inverted_luminance = 255 - luminance
            
            # Apply the luminance change
            for channel in range(min(3, img_array.shape[2])):
                img_array[:, :, channel] = np.clip(
                    img_array[:, :, channel] + (inverted_luminance - luminance), 0, 255
                )
        
        return Image.fromarray(img_array.astype(np.uint8))


def main():
    """Main function to handle command line arguments and process image."""
    parser = argparse.ArgumentParser(description="Invert colors of a PNG image with various options")
    parser.add_argument("input_image", help="Path to input PNG image")
    parser.add_argument("-o", "--output", help="Output file path (default: adds '_inverted' to input name)")
    parser.add_argument("-m", "--method", choices=['simple', 'rgb', 'hsv', 'negative', 'luminance'], 
                       default='simple', help="Inversion method (default: simple)")
    
    # RGB channel options
    parser.add_argument("--no-red", action="store_true", help="Don't invert red channel (for RGB method)")
    parser.add_argument("--no-green", action="store_true", help="Don't invert green channel (for RGB method)")
    parser.add_argument("--no-blue", action="store_true", help="Don't invert blue channel (for RGB method)")
    
    # HSV options
    parser.add_argument("--hsv-hue", action="store_true", help="Invert hue in HSV method")
    parser.add_argument("--hsv-saturation", action="store_true", help="Invert saturation in HSV method")
    parser.add_argument("--hsv-no-value", action="store_true", help="Don't invert value in HSV method")
    
    args = parser.parse_args()
    
    try:
        # Initialize the inverter
        inverter = ImageColorInverter(args.input_image)
        
        # Apply the selected inversion method
        if args.method == 'simple':
            result_image = inverter.simple_invert()
        elif args.method == 'rgb':
            result_image = inverter.rgb_channel_invert(
                invert_red=not args.no_red,
                invert_green=not args.no_green,
                invert_blue=not args.no_blue
            )
        elif args.method == 'hsv':
            result_image = inverter.hsv_invert(
                invert_hue=args.hsv_hue,
                invert_saturation=args.hsv_saturation,
                invert_value=not args.hsv_no_value
            )
        elif args.method == 'negative':
            result_image = inverter.negative_invert()
        elif args.method == 'luminance':
            result_image = inverter.luminance_invert()
        
        # Determine output path
        if args.output:
            output_path = Path(args.output)
        else:
            input_path = Path(args.input_image)
            output_path = input_path.parent / f"{input_path.stem}_inverted{input_path.suffix}"
        
        # Save the result
        result_image.save(output_path)
        print(f"Inverted image saved to: {output_path}")
        
        # Display some statistics
        print(f"Inversion method used: {args.method}")
        print(f"Original image size: {inverter.image.size}")
        print(f"Output image mode: {result_image.mode}")
        
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()



