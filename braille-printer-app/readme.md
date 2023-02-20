Braille Printer Application
======================

`brf-printer-app` implements printing for a variety of common Braille printers
connected via network or USB.  Features include:

- A single executable handles spooling, status, and server functionality.
- Multiple printer support.
- Each printer implements an IPP Everywhere™ print service and is compatible
  with the driverless printing support in Linux®.
- Each printer can directly print "raw", pdf,ubrl files.


> Note: Please use the Github issue tracker to report issues or request
> features/improvements in `brf-printer-app`:
>
> <https://github.com/michaelrsweet/hp-printer-app/issues>


Requirements
------------

`brf-printer-app` depends on:

- A POSIX-compliant "make" program (both GNU and BSD make are known to work),
- A C99 compiler (both Clang and GCC are known to work),
- [PAPPL](https://www.msweet.org/pappl) 1.1 or later.
- [CUPS](https://openprinting.github.io/cups) 2.2 or later (for libcups).
- [CUPS-FILTER](https://github.com/OpenPrinting/cups-filters) 1.28.16 or later.


Installing
----------
To install `brf-printer-app` from source, you'll need a "make"
program, a C99 compiler (Clang and GCC work), the CUPS developer files, and the
PAPPL developer files.  Once the prerequisites are installed on your system,
use the following commands to install `brf-printer-app` to "/usr/local/bin":

    make
    sudo make install

You can change the destination directory by setting the `prefix` variable on
the second command, for example:

    sudo make prefix=/opt/brf-printer-app install
    


Basic Usage
-----------

`brf-printer-app` uses a single executable to perform all functions.  The normal
syntax is:

    brf-printer-app SUB-COMMAND [OPTIONS] [FILES]

where "SUB-COMMAND" is one of the following:

- "add": Add a printer
- "cancel": Cancel one or more jobs
- "default": Get or set the default printer
- "delete": Delete a printer
- "devices": List available printers
- "drivers": List available drivers
- "jobs": List queued jobs
- "modify": Modify a printer
- "options": Lists the supported options and values
- "printers": List added printer queues
- "server": Run in server mode
- "shutdown": Shutdown a running server
- "status": Show server or printer status
- "submit": Submit one or more files for printing

You can omit the sub-command if you just want to print something, for example:

    brf-printer-app somefile.brf

The options vary based on the sub-command, but most commands support "-d" to
specify a printer and "-o" to specify a named option with a value, for example:

- `brf-printer-app -d myprinter somefile.brf`: Print a file to the printer named
  "myprinter".
- `brf-printer-app -o media=na_letter_8.5x11in picture.brf`: Print a media to a US
  letter sheet.
- `brf-printer-app default -d myprinter`: Set "myprinter" as the default printer.

See the `brf-printer-app` man page for more examples.


Running the Server
------------------

Normally you'll run `brf-printer-app` in the background as a service for your
printer(s), using the systemd service file:

    sudo systemctl enable brf-printer-app.service
    sudo systemctl start brf-printer-app.service

You can start it in the foreground with the following command:

    sudo brf-printer-app server -o log-file=- -o log-level=debug

Root access is needed on Linux when talking to USB printers, otherwise you can
run `brf-printer-app` without the "sudo" on the front.


Supported Printers
------------------

The following printers are currently supported:

- Generic Braille embosser.


Legal Stuff
-----------

The Braille Printer Application is Copyright © 2022 by Chandresh Soni.
Some of the refrence have been taken from pappl-retrofit.(https://github.com/OpenPrinting/pappl-retrofit) which is Copyright © 2022 by openprinting.

Filter used to convert various file format are in cups-filter is Copyright © 2022 by Samuel Thibault (samuel.thibault@ens-lyon.org).

It is derived from the HP PCL Printer Application, a first working model of
a raster Printer Application using PAPPL. It is available here:

https://github.com/michaelrsweet/hp-printer-apps 

The HP PCL Printer Application is Copyright © 2019-2020 by Michael R Sweet.

This software is licensed under the Apache License Version 2.0.  See the files
"LICENSE" and "NOTICE" for more information.
