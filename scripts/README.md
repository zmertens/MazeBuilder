# Scripts

## `make_icon.rb`

It's really useful for making an app icon.

### Installation

`bundle install`

### Run

`ruby make_icon.rb`


## `invert_image_colors.py`

Invert the colors of an image with several options.

### Prerequisites
The script requires Python with Pillow and NumPy libraries installed.

Setup a virtual env:
  * `python -m venv .venv`
  * `source .venv/bin/activate`
  * `pip install numpy pillow`

### Features

✅ **Multiple inversion methods**: Simple, RGB channel-selective, HSV, luminance-only  
✅ **Preserves transparency**: Alpha channels are maintained  
✅ **Command-line interface**: Easy to use with various options  
✅ **Programmatic API**: Can be imported and used in other Python scripts  
✅ **Error handling**: Clear error messages for common issues  

### Method Recommendations

- **Simple**: Best for general purpose color inversion
- **RGB selective**: Great for artistic effects or correcting specific color casts
- **HSV**: Excellent for creative color manipulation
- **Luminance**: Useful when you want to maintain color relationships but invert brightness

## `from_png_to_bmp.py`

Converts PNG images to BMP format while preserving image dimensions and properly handling transparency.

### Features

✅ **Preserves dimensions**: Output BMP has identical width and height  
✅ **Handles transparency**: Converts transparent areas to specified background color  
✅ **Batch processing**: Convert multiple PNG files at once  
✅ **Format validation**: Ensures input is valid PNG and output is proper BMP  
✅ **Detailed reporting**: Shows file sizes, dimensions, and conversion status  

### File Size Differences
- **PNG**: Compressed, smaller file size
- **BMP**: Uncompressed, much larger file size
- Example: 200KB PNG → 42MB BMP (for 3751×3751 image)

### Error Handling
The script provides clear error messages for:
- Missing input files
- Invalid PNG format
- Write permission issues
- Corrupted image files

## `secure_http_server.py`

Provides a secure local HTTP server for testing.

## `test_cli.js`

This script is copied automatically in Web builds to the CLI binary directory.

Run it with: `node test_cli.js <mazebuilder args>`
