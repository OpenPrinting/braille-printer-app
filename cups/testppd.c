/*
 * "$Id$"
 *
 *   PPD test program for the Common UNIX Printing System (CUPS).
 *
 *   Copyright 1997-2006 by Easy Software Products.
 *
 *   These coded instructions, statements, and computer programs are the
 *   property of Easy Software Products and are protected by Federal
 *   copyright law.  Distribution and use rights are outlined in the file
 *   "LICENSE.txt" which should have been included with this file.  If this
 *   file is missing or damaged please contact Easy Software Products
 *   at:
 *
 *       Attn: CUPS Licensing Information
 *       Easy Software Products
 *       44141 Airport View Drive, Suite 204
 *       Hollywood, Maryland 20636 USA
 *
 *       Voice: (301) 373-9600
 *       EMail: cups-info@cups.org
 *         WWW: http://www.cups.org
 *
 *   This file is subject to the Apple OS-Developed Software exception.
 *
 * Contents:
 *
 *   main() - Main entry.
 */

/*
 * Include necessary headers...
 */

#include <stdio.h>
#include <stdlib.h>
#include <cups/string.h>
#include <errno.h>
#include "ppd.h"
#ifdef WIN32
#  include <io.h>
#else
#  include <unistd.h>
#  include <fcntl.h>
#endif /* WIN32 */


/*
 * Test data...
 */

static const char	*default_code =
			"[{\n"
			"%%BeginFeature: *PageRegion Letter\n"
			"PageRegion=Letter\n"
			"%%EndFeature\n"
			"} stopped cleartomark\n"
			"[{\n"
			"%%BeginFeature: *InputSlot Tray\n"
			"InputSlot=Tray\n"
			"%%EndFeature\n"
			"} stopped cleartomark\n";


/*
 * 'main()' - Main entry.
 */

int					/* O - Exit status */
main(int  argc,				/* I - Number of command-line arguments */
     char *argv[])			/* I - Command-line arguments */
{
  ppd_file_t	*ppd;			/* PPD file loaded from disk */
  int		status;			/* Status of tests (0 = success, 1 = fail) */
  int		conflicts;		/* Number of conflicts */
  char		*s;			/* String */


  status = 0;

  fputs("ppdOpenFile: ", stdout);

  if ((ppd = ppdOpenFile("test.ppd")) != NULL)
    puts("PASS");
  else
  {
    ppd_status_t	err;		/* Last error in file */
    int			line;		/* Line number in file */


    status ++;
    err = ppdLastError(&line);

    printf("FAIL (%s on line %d)\n", ppdErrorString(err), line);
  }

  fputs("ppdMarkDefaults: ", stdout);
  ppdMarkDefaults(ppd);

  if ((conflicts = ppdConflicts(ppd)) == 0)
    puts("PASS");
  else
  {
    status ++;
    printf("FAIL (%d conflicts)\n", conflicts);
  }

  fputs("ppdEmitString: ", stdout);
  if ((s = ppdEmitString(ppd, PPD_ORDER_ANY, 0.0)) != NULL &&
      !strcmp(s, default_code))
    puts("PASS");
  else
  {
    printf("FAIL (%d bytes instead of %d)\n", s ? strlen(s) : 0,
           strlen(default_code));

    if (s)
      puts(s);
  }

  if (s)
    free(s);

  ppdClose(ppd);

  return (status);
}


/*
 * End of "$Id$".
 */
