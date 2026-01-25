# PiSSTVpp

PiSSTVpp is a simple implementation of various SSTV protocols, to be used by low-frills computers running a CLI, such as with Raspberry Pis and so on.
Its intended for use by ham radio amateurs.

## Compiling

To compile PiSSTVpp, you should download this repository and also have:

* gcc
* libgd
* libmagic
* make

On Ubuntu or Debian, you should be able to install those dependencies by typing into your terminal:

`sudo apt-get install build-essential libgd libmagic`

After that, type `make pisstvpp` in the directory where PiSSTV resides in.

## Usage

`./pisstvpp2 -i <image> [OPTIONS]`

This command creates a WAVE or AIFF file that encodes an image into an SSTV audio signal.

### Supported Protocols

| Protocol | Code | VIS | Resolution | TX Time | Color Space |
|----------|------|-----|------------|---------|-------------|
| Martin 1 | m1   | 44  | 320×256   | 114 sec | RGB         |
| Martin 2 | m2   | 40  | 320×256   | 58 sec  | RGB         |
| Scottie 1 | s1  | 60  | 320×256   | 110 sec | RGB         |
| Scottie 2 | s2  | 56  | 320×256   | 71 sec  | RGB         |
| Scottie DX | sdx | 76 | 320×256   | 269 sec | RGB         |
| Robot 36 | r36 | 8   | 320×240   | 36 sec  | YUV         |
| Robot 72 | r72 | 12  | 320×240   | 72 sec  | YUV         |

### Required and Optional Parameters

```
REQUIRED OPTIONS:
  -i <file>       Input image file (PNG, JPEG, GIF, or BMP)

OPTIONAL OPTIONS:
  -a <mode>       Aspect ratio correction: 4:3, fit, or stretch (default: 4:3)
  -o <file>       Output audio file (default: input_file.wav)
  -p <protocol>   SSTV protocol to use (default: m1)
  -f <fmt>        Output format: wav or aiff (default: wav)
  -r <rate>       Audio sample rate in Hz (default: 22050, range: 8000-48000)
  -v              Enable verbose output (progress details)
  -h              Display this help message

CW SIGNATURE OPTIONS (optional):
  -C <callsign>   Add CW signature with callsign (max 31 characters)
  -W <wpm>        Set CW signature speed in WPM, range 1-50 (default: 15)
  -T <freq>       Set CW signature tone frequency in Hz, range 400-2000 (default: 800)
```

### Examples

```bash
# Encode as Martin 1 (default), save as output.wav
./pisstvpp2 -i image.jpg -o output.wav

# Encode as Scottie 2 with verbose progress
./pisstvpp2 -i image.png -p s2 -v

# Encode as Robot 72 (72-second transmission, high quality)
./pisstvpp2 -i image.jpg -p r72 -o sstv_r72.wav

# Encode with CW callsign signature at 20 WPM
./pisstvpp2 -i image.jpg -C N0CALL -W 20

# High-quality Martin 1 at 48 kHz sample rate
./pisstvpp2 -i image.jpg -r 48000 -p m1
```

### Image Requirements

- **Martin/Scottie modes:** 320×256 pixels (16-line header included)
- **Robot 36 and Robot 72:** 320×240 pixels
- Aspect correction via `-a` option: `4:3` (default), `fit`, or `stretch`


## Other things

There are some shell and Python scripts that carried over from the original repository by AgriVision (Gerrit Polder, PA3BYA). They should have been modified to work with the PiSSTVpp, but is not tested.

raspistill: to grab the image from the camera.
pisstv: to convert the image to a soundfile.
pifm_sstv: to transmit the soundfile over the air, e.g. on 144.5 MHz

pisstv is heavilly based on work from KI4MCW, which can be found here: https://sites.google.com/site/ki4mcw/Home/sstv-via-uc
I fixed some errors and made it a little bit more flexible.

pifm_sstv is based on the work of Oliver Mattos and Oskar Weigl  (http://www.icrobotics.co.uk/wiki/index.php/Turning_the_Raspberry_Pi_Into_an_FM_Transmitter).

The original program was intended for transmitting broadband stereo signals.
[AgriVision] adapted it a little bit so that the bandwidth can be set, which is very important for narrow-band ham radio transmissions. Also the timing can be tuned from the command-line, which is important for SSTV, where impropper timing results in slanted images.

sstvcatch is kind of a sstv security camera. A python script runs an endless loop, waits for image change, then transmits image data on 144.5 MHz using SSTV.

## License
All of the code contained here is licensed by the GNU General Public License v3.
A copy of the GPLv3 has been included in the repository.

## Credits

AgriVision (Gerrit Polder)

KI4MCW for SSTV

Oliver Mattos and Oskar Weigl for PiFM
