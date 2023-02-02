# OpenPrinting Braille Printer Application v2.0b1 - 2020-11-18

Looking for compile instructions? Read the file "INSTALL"
instead...

## INTRODUCTION

CUPS is a standards-based, open-source printing system used by
Apple's Mac OS® and other UNIX®-like operating systems,
especially also Linux. CUPS uses the Internet Printing Protocol
("IPP") and provides System V and Berkeley command-line
interfaces, a web interface, and a C API to manage printers and
print jobs.

This package contains backends, filters, and auxiliary files for
Braille embosser support, currently only as a classic CUPS printer
driver package. A conversion to a Printer Application is in the works
and will get committed here soon.

For compiling and using this package see INSTALL file.

Report bugs to

    https://github.com/OpenPrinting/braille-printer-app/issues

See the "COPYING", "LICENCE", and "NOTICE" files for legal
information. The license is the same as for CUPS, for a maximum of
compatibility.

## LINKS

### Converting the Braille Embosser Driver to a Printer Application

* [Converting Braille embosser support into a printer application - GSoC 2022 project by Chandresh Soni](https://gist.github.com/Chandresh2702/73923b2c686039404cdd9b050edbe995)

### The New Architecture of Printing and Scanning

* [The New Architecture - What is it?](https://openprinting.github.io/current/#the-new-architecture-for-printing-and-scanning)
* [Ubuntu Desktop Team Indaba on YouTube](https://www.youtube.com/watch?v=P22DOu_ahBo)

### Printer Applications

* [All free drivers in a PPD-less world - OR - All free drivers in Snaps](https://openprinting.github.io/achievements/#all-free-drivers-in-a-ppd-less-world---or---all-free-drivers-in-snaps)
* [Current activity on Printer Applications](https://openprinting.github.io/current/#printer-applications)

## DOCUMENTATION FROM CUPS-FILTERS 1.x

Most of this is still valid for the current cups-filters.

### BRAILLE EMBOSSING

cups-filters also provides filters and drivers for braille
embossers. It supports:

- Text on all kinds of embossers with generic support
- Text and graphics on the Index V3 embossers and above.

This is configured in CUPS just like any printer. Options can then be configured
in the standard printer panel, or passed as -o options to the lp command.


#### Text support

Text can be embossed either with no translation on the computer side (the
embosser will translate), or with translation on the computer side (thanks to
liblouis). It is a matter of running

    lp file.txt

or even

    lp file.html
    lp file.odt
    lp file.doc
    lp file.rtf
    lp file.docx
    lp file.pdf

Important: it is really preferrable to directly print the document files
themselves, and not a pdf output, or printing from the application (which
would first convert to pdf). That way, the braille conversion will have the
proper document structure (paragraphs, titles, footnotes, etc.) to produce good
quality.


#### Vector Image support

Vector images can be embossed by converting them to braille dots.

This needs the inkscape package installed. Various input formats are then
supported: .svg, .fig, .wmf, .emf, .cgm, .cmx

The conversion assumes that the input is black-on-white. If it is
white-on-black, the -o Negate option can be used.

This image support is preferred over the generic image support described below,
which has to reconstruct lines to be embossed.


#### Image support

Images can be embossed by converting them to braille dots.

The orientation of the image can be controlled. By default it will be rotate to
fit the image orientation, i.e. it will be rotate by 90 degree if it is wider
than high and the paper is higher than wide, or if vice-versa. Other rotation
modes are provided.

By default, the image will be resized to fit the size of the paper. Disabling
the resize (fitplot set to No) will crop the image to the paper size. This is
useful for instance when a carefully-drawn image was designed especially for
embossing, and thus its pixels should exactly match with braille dots. In such
case, edge detection should very probably be disabled too.

The image can be processed for edge detection. When no processing is done (edge
detection is configured to "None"), the dark pixels are embossed if the Negate
option is off, or the light pixels are embossed if the Negate option is on. When
edge processing is done, only the edges of the images will be embossed. The
Basic and the Canny algorithms bring differing results. The Basic algorithm
can be tuned thanks to the edge factor only. The Canny algorithm can also be
tuned: increasing the Upper value will reduce the amount of detected edges (and
vice-versa), increasing the Lower value will reduce the lengths of the detected
edges (and vice-versa). The Radius and Sigma parameter control the blurring
performed before edge detection, to improve the result; the Radius parameter
controls how large blurring should be performed, setting it to zero requests
autodetection; the Sigma parameter determines how strongly blurring should be
performed.

A lot of images formats are support, so one can just run

    lp file.png
    lp file.gif
    lp file.jpg
    ...

Here are complete examples for controlling the processing (all options can be
omitted, the default values are shown here):

Emboss the image without edge detection, as black on white or white on black:

    lp -o "Edge=None" file.png
    lp -o "Edge=None Negate" file.png

Emboss the image with edge detection, the default tuning parameters are set
here:

    lp -o "Edge=Edge EdgeFactor=1" file.png
    lp -o "Edge=Canny CannyRadius=0 CannySigma=1 CannyLower=10 CannyUpper=30" file.png

Emboss the image as it is, without any resize or edge detection, as black on
white or white on black:

    lp -o "nofitplot Edge=None" file.png
    lp -o "nofitplot Edge=None Negate" file.png


#### Generic embosser support

It should be possible to make all embossers use the generic driver. For this to
work, one has to:

- configure the embosser itself so that it uses an MIT/NABCC/BRF braille table
- add in CUPS a printer with the "Generic" manufacturer and "Braille embosser"
  model
- configure CUPS options according to the embosser settings, so that CUPS knows
  the page size, braille spacing, etc.

The generic driver can emboss text, as well as images, but images will probably
be distorted by the Braille interline spacing.


#### Index embossers support

Supported models:

- Basic-S V3/4
- Basic-D V3/4
- Everest-D V3/4
- 4-Waves PRO V3
- 4X4 PRO V3
- Braille Box V4

Index V3 embosser support has been well tested. It supports both text and
graphics mode.  Embossers with firmware 10.30 and above can be easily configured
from CUPS (paper dimension, braille spacing, etc.).

Index V4 embosser support has not been tested, but is very close to V3 support,
so it is probably working fine already.  Feedback would be very welcome.

To connect an Index embosser through Ethernet, gather its IP adress, select the
"AppSocket/HP JetDirect" network printer protocol, and set
socket://the.embosser.IP.address:9100 as Connection URL.

The density of dots for images can easily be chosen from the command line, for
instance:

    lp -o "GraphicDotDistance=160" file.png

to select 1.6mm dots spacing

Troubleshooting: if your embosser starts every document with spurious
"TM0,BM0,IM0,OM0" or "TM0,BI0", your embosser is most probably still using an
old 10.20 firmware.  Please either reflash the embosser with a firmware version
10.30 or above, or select the 10.20 firmware version in the "index" panel of the
cups printer options.


#### Braille output options

The output can be finely tuned from the standard printing panel, or from the
command-line, the following example selects translation tables for French and
Greek, with 2.5mm dot spacing and 5mm line spacing. All options can be omitted,
the default values are shown here.

    lp -o "LibLouis=fr-fr-g1 LibLouis2=gr-gr-g1 TextDotDistance=250 LineSpacing=500" file.txt


#### Reworking output before embossing

One may want to check and modify the .brf or .ubrl output before sending it to
the embosser.  This can be achieved by first generating the .brf file:

    /usr/sbin/cupsfilter -m application/vnd.cups-brf -p /etc/cups/ppd/yourprinter.ppd yourdocument.txt > ~/test.brf

One can choose a ppd file and additionally pass -o options to control the
generated output. One can then modify the .brf file with a text editor. One can
then emboss it:

    lp -o document-format=application/vnd.cups-brf ~/test.brf


The same can be achieved for images:

    /usr/sbin/cupsfilter -m image/vnd.cups-ubrl -p /etc/cups/ppd/yourprinter.ppd yourimage.png > ~/test.ubrl

    lp -o document-format=image/vnd.cups-ubrl ~/test.ubrl


#### BRF file output

One can generate BRF files by adding a virtual BRF printer.

When creating it in the cups interface, choose the CUPS-BRF local printer,
select the Generic maker, and choose the Generic Braille embosser model.

Printing to the resulting printer will generate a .brf file in a BRF
subdirectory of the home directory.


#### UBRL file output

One can generate Unicode braille files, not useful for embossing, but which can
be easily looked at by sighted people to check for the output.

In the cups interface, create a printer with the CUPS-BRF local printer,
the Generic maker, and choose the Generic UBRL generator model.

Printing to the resulting printer will generate a .brf file in a BRF
subdirectory of the home directory.


#### Remark about the source code

The file driver/index/ubrlto4dot.c is used to generate
the translation table in
driver/index/imageubrltoindexv[34]. It is included as
"source code" for these two files, even if actually running the
generation in the Makefile is more tedious than really useful.


#### TODO

- Test whether one wants to negate, e.g. to emboss as few dots as possible
- textubrltoindex when liblouis tools will be able to emit 8dot braille

