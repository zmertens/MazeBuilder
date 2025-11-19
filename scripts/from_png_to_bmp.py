"""
This script converts PNG images to BMP format.
It reads a PNG image file and saves it as a BMP file in the same directory.
Preserves image dimensions and handles transparency appropriately.
"""

import argparse
import sys
from pathlib import Path
from PIL import Image
import os


class PngToBmpConverter:
    """Class to handle PNG to BMP conversion with various options."""
    
    def __init__(self, png_path):
        """Initialize with PNG file path."""
        self.png_path = Path(png_path)
        self.png_image = None
        self.validate_input()
        self.load_png()
    
    def validate_input(self):
        """Validate the input PNG file."""
        if not self.png_path.exists():
            raise FileNotFoundError(f"PNG file not found: {self.png_path}")
        
        if not self.png_path.suffix.lower() == '.png':
            raise ValueError(f"Input file must be a PNG image: {self.png_path}")
    
    def load_png(self):
        """Load the PNG image and display information."""
        try:
            self.png_image = Image.open(self.png_path)
            print(f"Loaded PNG: {self.png_path.name}")
            print(f"  Mode: {self.png_image.mode}")
            print(f"  Size: {self.png_image.size}")
            print(f"  Format: {self.png_image.format}")
            
            # Check if image has transparency
            if self.png_image.mode in ('RGBA', 'LA') or 'transparency' in self.png_image.info:
                print(f"  Has transparency: Yes")
            else:
                print(f"  Has transparency: No")
                
        except Exception as e:
            raise ValueError(f"Error loading PNG image: {e}")
    
    def convert_to_bmp(self, output_path=None, background_color=(255, 255, 255), preserve_alpha=False):
        """
        Convert PNG to BMP format.
        
        Args:
            output_path: Custom output path (optional)
            background_color: Background color for transparent areas (RGB tuple)
            preserve_alpha: Whether to try preserving alpha (not recommended for BMP)
        
        Returns:
            Path to the created BMP file
        """
        # Determine output path
        if output_path is None:
            output_path = self.png_path.parent / f"{self.png_path.stem}.bmp"
        else:
            output_path = Path(output_path)
        
        # Create a copy to work with
        working_image = self.png_image.copy()
        
        # Handle transparency for BMP format
        if working_image.mode in ('RGBA', 'LA'):
            if preserve_alpha:
                print("Warning: BMP format doesn't support transparency. Alpha channel will be lost.")
                # Convert to RGB, losing alpha
                rgb_image = Image.new('RGB', working_image.size, background_color)
                if working_image.mode == 'RGBA':
                    rgb_image.paste(working_image, mask=working_image.split()[-1])  # Use alpha as mask
                else:  # LA mode
                    rgb_image.paste(working_image.convert('RGB'))
                working_image = rgb_image
            else:
                # Create background and composite
                background = Image.new('RGB', working_image.size, background_color)
                if working_image.mode == 'RGBA':
                    background.paste(working_image, (0, 0), working_image)
                else:  # LA mode
                    background.paste(working_image.convert('RGB'), (0, 0))
                working_image = background
        
        elif working_image.mode == 'P' and 'transparency' in working_image.info:
            # Handle palette mode with transparency
            working_image = working_image.convert('RGBA')
            background = Image.new('RGB', working_image.size, background_color)
            background.paste(working_image, (0, 0), working_image)
            working_image = background
        
        # Ensure RGB mode for BMP
        if working_image.mode != 'RGB':
            working_image = working_image.convert('RGB')
        
        # Save as BMP
        try:
            working_image.save(output_path, format='BMP')
            
            # Verify the conversion
            self.verify_conversion(output_path)
            
            return output_path
            
        except Exception as e:
            raise RuntimeError(f"Error saving BMP file: {e}")
    
    def verify_conversion(self, bmp_path):
        """Verify the converted BMP file."""
        try:
            bmp_image = Image.open(bmp_path)
            print(f"\nConversion successful!")
            print(f"Output BMP: {bmp_path.name}")
            print(f"  Mode: {bmp_image.mode}")
            print(f"  Size: {bmp_image.size}")
            print(f"  Format: {bmp_image.format}")
            
            # Compare dimensions
            if bmp_image.size == self.png_image.size:
                print(f"  ✓ Dimensions preserved: {bmp_image.size}")
            else:
                print(f"  ⚠ Dimension mismatch! PNG: {self.png_image.size}, BMP: {bmp_image.size}")
            
            # File size information
            png_size = self.png_path.stat().st_size
            bmp_size = bmp_path.stat().st_size
            print(f"  File sizes - PNG: {png_size:,} bytes, BMP: {bmp_size:,} bytes")
            
            bmp_image.close()
            
        except Exception as e:
            print(f"Warning: Could not verify BMP file: {e}")


def convert_png_to_bmp(png_file, output_file=None, background_color=(255, 255, 255)):
    """
    Simple function to convert a single PNG file to BMP.
    
    Args:
        png_file: Path to PNG file
        output_file: Optional output path
        background_color: RGB tuple for transparent areas
    
    Returns:
        Path to created BMP file
    """
    converter = PngToBmpConverter(png_file)
    return converter.convert_to_bmp(output_file, background_color)


def batch_convert(input_directory, output_directory=None, background_color=(255, 255, 255)):
    """
    Convert all PNG files in a directory to BMP format.
    
    Args:
        input_directory: Directory containing PNG files
        output_directory: Optional output directory (defaults to same as input)
        background_color: RGB tuple for transparent areas
    """
    input_dir = Path(input_directory)
    if not input_dir.exists() or not input_dir.is_dir():
        raise ValueError(f"Input directory not found: {input_dir}")
    
    if output_directory:
        output_dir = Path(output_directory)
        output_dir.mkdir(parents=True, exist_ok=True)
    else:
        output_dir = input_dir
    
    png_files = list(input_dir.glob("*.png"))
    if not png_files:
        print(f"No PNG files found in {input_dir}")
        return
    
    print(f"Found {len(png_files)} PNG files to convert...")
    
    for png_file in png_files:
        try:
            output_path = output_dir / f"{png_file.stem}.bmp"
            converter = PngToBmpConverter(png_file)
            converter.convert_to_bmp(output_path, background_color)
            print(f"✓ Converted: {png_file.name} → {output_path.name}")
        except Exception as e:
            print(f"✗ Failed to convert {png_file.name}: {e}")


def main():
    """Main function to handle command line arguments and process conversion."""
    parser = argparse.ArgumentParser(
        description="Convert PNG images to BMP format",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python from_png_to_bmp.py image.png
  python from_png_to_bmp.py image.png -o output.bmp
  python from_png_to_bmp.py image.png --background 255 0 0  # Red background for transparency
  python from_png_to_bmp.py --batch input_folder/
  python from_png_to_bmp.py --batch input_folder/ --output-dir output_folder/
        """
    )
    
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("input_png", nargs="?", help="Input PNG file to convert")
    group.add_argument("--batch", metavar="DIR", help="Convert all PNG files in directory")
    
    parser.add_argument("-o", "--output", help="Output BMP file path")
    parser.add_argument("--output-dir", help="Output directory for batch conversion")
    parser.add_argument("--background", nargs=3, type=int, metavar=("R", "G", "B"), 
                       default=[255, 255, 255], help="Background color for transparent areas (default: white)")
    
    args = parser.parse_args()
    
    try:
        background_color = tuple(args.background)
        
        if args.batch:
            # Batch conversion
            batch_convert(args.batch, args.output_dir, background_color)
        else:
            # Single file conversion
            if not args.input_png:
                parser.error("Input PNG file is required for single file conversion")
            
            output_path = convert_png_to_bmp(args.input_png, args.output, background_color)
            print(f"\n✓ Conversion completed: {output_path}")
    
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()