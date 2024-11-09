//
// BRF-Printer app for the Printer Application Framework
//
// Copyright © 2020-2024 by Michael R Sweet.
// Copyright © 2022 by Chandresh Soni.
// Copyright © 2024 by Arun Patwa.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

#include "config.h"

#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <cups/backend.h>
#include <cups/ipp.h>
#include <cupsfilters/log.h>
#include <cupsfilters/filter.h>
#include <pappl/pappl.h>

#ifdef HAVE_LIBLOUISUTDML
#include <liblouisutdml/liblouisutdml.h>
#endif

#include <magic.h>

#include "brf-printer.h"

static bool BRFFilterCB(pappl_job_t *job, pappl_device_t *device, void *cbdata);

static int brf_print_filter_function(int inputfd, int outputfd, int inputseekable, cf_filter_data_t *data, void *parameters);

static const char *autoadd_cb(const char *device_info, const char *device_uri, const char *device_id, void *cbdata);

static bool driver_cb(pappl_system_t *system, const char *driver_name, const char *device_uri, const char *device_id, pappl_pr_driver_data_t *data, ipp_t **attrs, void *cbdata);

static int match_id(int num_did, cups_option_t *did, const char *match_id);

static const char *mime_cb(const unsigned char *header, size_t headersize, void *data);

static bool printer_cb(const char *device_info, const char *device_uri, const char *device_id, pappl_system_t *system);

static pappl_system_t *system_cb(int num_options, cups_option_t *options, void *data);
static int create_brf_printer(pappl_system_t *system);

static int brf_JobIsCanceled(void *data);

static void brf_JobLog(void *data,cf_loglevel_t level,const char *message,...);

// Local globals...

static pappl_pr_driver_t brf_drivers[] =
    {
        // Driver list
        {"gen_brf", "Generic Braille embosser",
         NULL, NULL},

};

// State file

// 'main()' - Main entry for brf.

brf_printer_app_global_data_t global_data;

int                // O - Exit status
main(int argc,     // I - Number of command-line arguments
     char *argv[]) // I - Command-line arguments
{
  return (papplMainloop(argc, argv,
                        "1.0",
                        NULL,
                        (int)(sizeof(brf_drivers) / sizeof(brf_drivers[0])),
                        brf_drivers, autoadd_cb, driver_cb,
                        /*subcmd_name*/ NULL, /*subcmd_cb*/ NULL,
                        system_cb,
                        /*usage_cb*/ NULL,
                        /*data*/ &global_data));
}

// 'autoadd_cb()' - Determine the proper driver for a given printer.

static const char *                 // O - Driver name or `NULL` for none
autoadd_cb(const char *device_info, // I - Device information/name (not used)
           const char *device_uri,  // I - Device URI
           const char *device_id,   // I - IEEE-1284 device ID
           void *cbdata)            // I - Callback data (System)
{
  int i,                 // Looping var
      score,             // Current driver match score
      best_score = 0,    // Best score
      num_did;           // Number of device ID key/value pairs
  cups_option_t *did;    // Device ID key/value pairs
  const char *make,      // Manufacturer name
      *best_name = NULL; // Best driver

  (void)device_info;

  // First parse the device ID and get any potential driver name to match

  num_did = papplDeviceParseID(device_id, &did);

  if ((make = cupsGetOption("MANUFACTURER", num_did, did)) == NULL)
    if ((make = cupsGetOption("MANU", num_did, did)) == NULL)
      make = cupsGetOption("MFG", num_did, did);

  // Then loop through the driver list to find the best match...
  for (i = 0; i < (int)(sizeof(brf_drivers) / sizeof(brf_drivers[0])); i ++)
  {

    if (brf_drivers[i].device_id)
    {
      // See if we have a matching device ID...
      score = match_id(num_did, did, brf_drivers[i].device_id);
      if (score > best_score)
      {
        best_score = score;
        best_name = brf_drivers[i].name;
      }
    }
  }

  // Clean up and return...
  cupsFreeOptions(num_did, did);

  return (best_name);
}

// 'match_id()' - Compare two IEEE-1284 device IDs and return a score.

// The score is 2 for each exact match and 1 for a partial match in a comma

// delimited field.  Any non-match results in a score of 0.

static int                     // O - Score
match_id(int num_did,          // I - Number of device ID key/value pairs
         cups_option_t *did,   // I - Device ID key/value pairs
         const char *match_id) // I - Driver's device ID match string
{
  int i,              // Looping var
      score = 0,      // Score
      num_mid;        // Number of match ID key/value pairs
  cups_option_t *mid, // Match ID key/value pairs
      *current;       // Current key/value pair
  const char *value,  // Device ID value
      *valptr;        // Pointer into value

  // Parse the matching device ID into key/value pairs...
  if ((num_mid = papplDeviceParseID(match_id, &mid)) == 0)
    return (0);

  // Loop through the match pairs to find matches (or not)
  for (i = num_mid, current = mid; i > 0; i--, current++)
  {
    if ((value = cupsGetOption(current->name, num_did, did)) == NULL)
    {
      // No match
      score = 0;
      break;
    }

    if (!strcasecmp(current->value, value))
    {
      // Full match!
      score += 2;
    }
    else if ((valptr = strstr(value, current->value)) != NULL)
    {
      // Possible substring match, check
      size_t mlen = strlen(current->value);
      // Length of match value
      if ((valptr == value || valptr[-1] == ',') && (!valptr[mlen] || valptr[mlen] == ','))
      {
        // Partial match!
        score++;
      }
      else
      {
        // No match
        score = 0;
        break;
      }
    }
    else
    {
      // No match
      score = 0;
      break;
    }
  }
  return score;
}

// 'driver_cb()' - Main driver callback

static bool // O - `true` on success, `false` on error
driver_cb(
    pappl_system_t *system,       // I - System
    const char *driver_name,      // I - Driver name
    const char *device_uri,       // I - Device URI
    const char *device_id,        // I - 1284 device ID
    pappl_pr_driver_data_t *data, // I - Pointer to driver data
    ipp_t **attrs,                // O - Pointer to driver attributes
    void *cbdata)                 // I - Callback data (not used)
{
  int i; // Looping var

  // Copy make/model info...
  for (i = 0; i < (int)(sizeof(brf_drivers) / sizeof(brf_drivers[0])); i++)
  {
    if (!strcmp(driver_name, brf_drivers[i].name))
    {
      papplCopyString(data->make_and_model, brf_drivers[i].description, sizeof(data->make_and_model));
      break;
    }
  }

  // Pages per minute
  data->ppm = 1;

  // Color values...
  data->color_supported = PAPPL_COLOR_MODE_AUTO | PAPPL_COLOR_MODE_MONOCHROME;
  data->color_default = PAPPL_COLOR_MODE_MONOCHROME;
  data->raster_types = PAPPL_PWG_RASTER_TYPE_BLACK_1;

  if (*attrs == NULL)
    *attrs = ippNew();

  // Adding TopMargin,BottomMargin,LeftMargin,RightMargin

  const char *margin_names[] = {"TopMargin", "BottomMargin", "LeftMargin", "RightMargin"};

  for (int i = 0; i < sizeof(margin_names)/sizeof(*margin_names); i++)
  {
    char attribute_name[50]; // Buffer to hold the formatted attribute names

    // Add margin name to vendor array
    data->vendor[data->num_vendor++] = margin_names[i];

    // Format and add the default integer attribute
    snprintf(attribute_name, sizeof(attribute_name), "%s-default", margin_names[i]);
    ippAddInteger(*attrs, IPP_TAG_PRINTER, IPP_TAG_INTEGER, attribute_name, 2);

    // Format and add the range attribute
    snprintf(attribute_name, sizeof(attribute_name), "%s-supported", margin_names[i]);
    ippAddRange(*attrs, IPP_TAG_PRINTER, attribute_name, 0, 10);
  }

  data->vendor[data->num_vendor++] = "SendFF";
  ippAddBoolean(*attrs, IPP_TAG_PRINTER, "SendFF-default", 0);

  data->vendor[data->num_vendor++] = "SendSUB";
  ippAddBoolean(*attrs, IPP_TAG_PRINTER, "SendSUB-default", 1);

  data->vendor[data->num_vendor++] = "TextDotDistance";
  ippAddInteger(*attrs, IPP_TAG_PRINTER, IPP_TAG_INTEGER, "TextDotDistance-default", 250);

  data->vendor[data->num_vendor++] = "TextDots";
  ippAddInteger(*attrs, IPP_TAG_PRINTER, IPP_TAG_INTEGER, "TextDots-default", 6);

  data->vendor[data->num_vendor++] = "LineSpacing";
  ippAddInteger(*attrs, IPP_TAG_PRINTER, IPP_TAG_INTEGER, "LineSpacing-default", 500);

  data->vendor[data->num_vendor++] = "GraphicDotDistance";
  ippAddInteger(*attrs, IPP_TAG_PRINTER, IPP_TAG_INTEGER, "GraphicDotDistance-default", 200);

  data->vendor[data->num_vendor++] = "LibLouis";
  ippAddString(*attrs, IPP_TAG_PRINTER, IPP_TAG_TEXT, "LibLouis-default", NULL, "Locale");

  data->vendor[data->num_vendor++] = "LibLouis2";
  ippAddString(*attrs, IPP_TAG_PRINTER, IPP_TAG_TEXT, "LibLouis2-default", NULL, "HyphLocale");

  data->vendor[data->num_vendor++] = "LibLouis3";
  ippAddString(*attrs, IPP_TAG_PRINTER, IPP_TAG_TEXT, "LibLouis3-default", NULL, "None");

  data->vendor[data->num_vendor++] = "LibLouis4";
  ippAddString(*attrs, IPP_TAG_PRINTER, IPP_TAG_TEXT, "LibLouis4-default", NULL, "None");

  data->vendor[data->num_vendor++] = "BraillePageNumber";
  ippAddString(*attrs, IPP_TAG_PRINTER, IPP_TAG_TEXT, "BraillePageNumber-default", NULL, "BottomMargin");

  data->vendor[data->num_vendor++] = "PrintPageNumber";
  ippAddString(*attrs, IPP_TAG_PRINTER, IPP_TAG_TEXT, "PrintPageNumber-default", NULL, "TopMargin");

  data->vendor[data->num_vendor++] = "PageSeparator";
  ippAddBoolean(*attrs, IPP_TAG_PRINTER, "PageSeparator-default", 1);

  data->vendor[data->num_vendor++] = "PageSeparatorNumber";
  ippAddBoolean(*attrs, IPP_TAG_PRINTER, "PageSeparatorNumber-default", 1);

  data->vendor[data->num_vendor++] = "ContinuePages";
  ippAddBoolean(*attrs, IPP_TAG_PRINTER, "ContinuePages-default", 1);

  data->vendor[data->num_vendor++] = "Negate";
  ippAddBoolean(*attrs, IPP_TAG_PRINTER, "Negate-default", 0);

  data->vendor[data->num_vendor++] = "EdgeFactor";
  ippAddInteger(*attrs, IPP_TAG_PRINTER, IPP_TAG_INTEGER, "EdgeFactor-default", 1);

  data->vendor[data->num_vendor++] = "CannyRadius";
  ippAddInteger(*attrs, IPP_TAG_PRINTER, IPP_TAG_INTEGER, "CannyRadius-default", 0);

  data->vendor[data->num_vendor++] = "CannySigma";
  ippAddInteger(*attrs, IPP_TAG_PRINTER, IPP_TAG_INTEGER, "CannySigma-default", 1);

  data->vendor[data->num_vendor++] = "CannyLower";
  ippAddInteger(*attrs, IPP_TAG_PRINTER, IPP_TAG_INTEGER, "CannyLower-default", 10);

  data->vendor[data->num_vendor++] = "CannyUpper";
  ippAddInteger(*attrs, IPP_TAG_PRINTER, IPP_TAG_INTEGER, "CannyUpper-default", 30);

  data->vendor[data->num_vendor++] = "Rotate";
  ippAddString(*attrs, IPP_TAG_PRINTER, IPP_TAG_TEXT, "Rotate-default", NULL,"90>");

  data->vendor[data->num_vendor++] = "Edge";
  ippAddString(*attrs, IPP_TAG_PRINTER, IPP_TAG_TEXT, "Edge-default", NULL,"Canny");

  ippAddBoolean(*attrs, IPP_TAG_PRINTER, "mirror-default", 0);

  ippAddBoolean(*attrs, IPP_TAG_PRINTER, "fitplot-default", 1);

  ippAddString(*attrs, IPP_TAG_PRINTER, IPP_TAG_TEXT, "PageSize-default", NULL, "A4");

  ippAddInteger(*attrs, IPP_TAG_PRINTER, IPP_TAG_INTEGER, "page-left-default", 15);

  ippAddInteger(*attrs, IPP_TAG_PRINTER, IPP_TAG_INTEGER, "page-right-default", 15);

  ippAddInteger(*attrs, IPP_TAG_PRINTER, IPP_TAG_INTEGER, "page-top-default", 15);
  
  ippAddInteger(*attrs, IPP_TAG_PRINTER, IPP_TAG_INTEGER, "page-bottom-default", 15);

  // "print-quality-default" value...
  data->quality_default = IPP_QUALITY_NORMAL;
  data->orient_default = IPP_ORIENT_NONE;

  // "sides" values...
  data->sides_supported = PAPPL_SIDES_ONE_SIDED;
  data->sides_default = PAPPL_SIDES_ONE_SIDED;

  // "orientation-requested-default" value...
  data->orient_default = IPP_ORIENT_NONE;

  data->scaling_default = PAPPL_SCALING_AUTO;

  // Use the corresponding sub-driver callback to set things up...
  if (!strncmp(driver_name, "gen_", 4))
    return (brf_gen(system, driver_name, device_uri, device_id, data, attrs, cbdata));

  else
  {
    papplLog(global_data.system, PAPPL_LOGLEVEL_ERROR, "****************brfgen-not called***************\n");
    return (false);
  }
}

void BRFSetup(pappl_system_t *system, brf_printer_app_global_data_t *global_data)
{
  for (int i = 0; converts[i].srctype != NULL; i++)
  {
    papplSystemAddMIMEFilter(system, converts[i].srctype, brf_MIMETYPE, BRFFilterCB, global_data);
  }
}

// 'mime_cb()' - MIME typing callback...

static const char *                  // O - MIME media type or `NULL` if none
mime_cb(const unsigned char *header, // I - Header data
        size_t headersize,           // I - Size of header data
        void *cbdata)                // I - Callback data (not used)
{
  magic_t magic;
  const char *mime_type = NULL;

  // Step 1: Open a magic_t object for MIME type detection
  magic = magic_open(MAGIC_MIME_TYPE);
  if (magic == NULL)
  {
    papplLog(global_data.system, PAPPL_LOGLEVEL_ERROR, "Failed to initialize libmagic.\n");
    return NULL;
  }

  // Step 2: Load the magic database
  if (magic_load(magic, NULL) != 0)
  {
    papplLog(global_data.system, PAPPL_LOGLEVEL_ERROR, "Failed to load magic database: %s\n", magic_error(magic));
    magic_close(magic);
    return NULL;
  }

  // Step 3: Use magic_buffer to determine MIME type from the header data
  mime_type = magic_buffer(magic, header, headersize);
  if (mime_type == NULL)
  {
    papplLog(global_data.system, PAPPL_LOGLEVEL_ERROR, "Failed to determine MIME type: %s\n", magic_error(magic));
  }
  else
  {
    // Duplicate the MIME type string, as magic_buffer may free it after magic_close
    mime_type = strdup(mime_type);
  }

  // Step 4: Clean up
  magic_close(magic);

  return mime_type; // Return the MIME type (or NULL if undetected)
}

// 'printer_cb()' - Try auto-adding printers.

static bool                         // O - `false` to continue
printer_cb(const char *device_info, // I - Device information
           const char *device_uri,  // I - Device URI
           const char *device_id,   // I - IEEE-1284 device ID
           pappl_system_t *system)  // I - System
{
  const char *driver_name = autoadd_cb(device_info, device_uri, device_id, system);

  // Driver name, if any

  if (driver_name)
  {
    char name[128], // Printer name
        *nameptr;   // Pointer in name
    papplCopyString(name, device_info, sizeof(name));

    if ((nameptr = strstr(name, " (")) != NULL)
      *nameptr = '\0';

    if (!papplPrinterCreate(system, 0, name, driver_name, device_id, device_uri))
    {
      // Printer already exists with this name, so try adding a number to the name
      int i;                         // Looping var
      char newname[128],             // New name
          number[4];                 // Number string
      size_t namelen = strlen(name), // Length of original name string
          numberlen;                 // Length of number string

      for (i = 2; i < 100; i++)
      {
        // Append " NNN" to the name, truncating the existing name as needed to
        // include the number at the end...

        snprintf(number, sizeof(number), " %d", i);
        numberlen = strlen(number);

        papplCopyString(newname, name, sizeof(newname));
        if ((namelen + numberlen) < sizeof(newname))
          memcpy(newname + namelen, number, numberlen + 1);
        else
          memcpy(newname + sizeof(newname) - numberlen - 1, number, numberlen + 1);

        // Try creating with this name...

        if (papplPrinterCreate(system, 0, newname, driver_name, device_id, device_uri))
          break;
      }
    }
  }

  return (false);
}

// 'system_cb()' - Setup the system object.

static pappl_system_t * // O - System object
system_cb(
    int num_options,        // I - Number options
    cups_option_t *options, // I - Options
    void *data)             // I - Callback data (unused)
{
  static char brf_statefile[1024];

  brf_printer_app_global_data_t *global_data = (brf_printer_app_global_data_t *)data;
  pappl_system_t *system;    // System object
  const char *val,           // Current option value
      *hostname,             // Hostname, if any
      *logfile,              // Log file, if any
      *system_name;          // System name, if any
  pappl_loglevel_t loglevel; // Log level
  int port = 0;              // Port number, if any
  pappl_soptions_t soptions = PAPPL_SOPTIONS_MULTI_QUEUE | PAPPL_SOPTIONS_WEB_INTERFACE | PAPPL_SOPTIONS_WEB_LOG | PAPPL_SOPTIONS_WEB_SECURITY;
  // System options
  static pappl_version_t versions[1] = // Software versions
      { { "brf", "", "1.0", { 1, 0 } } };

  // Parse standard log and server options...
  if ((val = cupsGetOption("log-level", num_options, options)) != NULL)
  {
    if (!strcmp(val, "fatal"))
      loglevel = PAPPL_LOGLEVEL_FATAL;
    else if (!strcmp(val, "error"))
      loglevel = PAPPL_LOGLEVEL_ERROR;
    else if (!strcmp(val, "warn"))
      loglevel = PAPPL_LOGLEVEL_WARN;
    else if (!strcmp(val, "info"))
      loglevel = PAPPL_LOGLEVEL_INFO;
    else if (!strcmp(val, "debug"))
      loglevel = PAPPL_LOGLEVEL_DEBUG;
    else
    {
      fprintf(stderr, "brf: Bad log-level value '%s'.\n", val);
      return (NULL);
    }
  }
  else
    loglevel = PAPPL_LOGLEVEL_UNSPEC;

  logfile = cupsGetOption("log-file", num_options, options);
  hostname = cupsGetOption("server-hostname", num_options, options);
  system_name = cupsGetOption("system-name", num_options, options);

  if ((val = cupsGetOption("server-port", num_options, options)) != NULL)
  {
    if (!isdigit(*val & 255))
    {
      fprintf(stderr, "brf: Bad server-port value '%s'.\n", val);
      return (NULL);
    }
    else
      port = atoi(val);
  }

  // State file...
  if ((val = getenv("SNAP_DATA")) != NULL)
  {
    snprintf(brf_statefile, sizeof(brf_statefile), "%s/brf.conf", val);
  }
  else if ((val = getenv("XDG_DATA_HOME")) != NULL)
  {
    snprintf(brf_statefile, sizeof(brf_statefile), "%s/.brf.conf", val);
  }
#ifdef _WIN32
  else if ((val = getenv("USERPROFILE")) != NULL)
  {
    snprintf(brf_statefile, sizeof(brf_statefile), "%s/AppData/Local/brf.conf", val);
  }
  else
  {
    papplCopyString(brf_statefile, "/brf.ini", sizeof(brf_statefile));
  }
#else
  else if ((val = getenv("HOME")) != NULL)
  {
    snprintf(brf_statefile, sizeof(brf_statefile), "%s/.brf.conf", val);
  }
  else
  {
    papplCopyString(brf_statefile, "/etc/brf.conf", sizeof(brf_statefile));
  }
#endif // _WIN32

  // Create the system object...
  if ((system = papplSystemCreate(soptions, system_name ? system_name : "Braille printer app", port, "_print,_universal", cupsGetOption("spool-directory", num_options, options), logfile ? logfile : "-", loglevel, cupsGetOption("auth-service", num_options, options), /* tls_only */ false)) == NULL)
    return (NULL);

  global_data->system = system;

  papplSystemAddListeners(system, NULL);
  papplSystemSetHostName(system, hostname);

  papplSystemSetMIMECallback(system, mime_cb, NULL);

  BRFSetup(system, global_data);

  papplSystemSetPrinterDrivers(system, (int)(sizeof(brf_drivers) / sizeof(brf_drivers[0])), brf_drivers, autoadd_cb, /*create_cb*/ NULL, driver_cb, system);

  papplSystemSetFooterHTML(system, "Copyright &copy; 2024 by Arun Patwa. All rights reserved.");

  papplLog(system, PAPPL_LOGLEVEL_INFO, "brf: statefile='%s'\n", brf_statefile);

  papplSystemSetSaveCallback(system, (pappl_save_cb_t)papplSystemSaveState, (void *)brf_statefile);

  papplSystemSetVersions(system, (int)(sizeof(versions) / sizeof(versions[0])), versions);

  papplSystemSetDNSSDName(system, system_name ? system_name : "brf");

  papplLog(system, PAPPL_LOGLEVEL_INFO, "Auto-adding printers...");

  papplDeviceList(PAPPL_DEVTYPE_USB, (pappl_device_cb_t)printer_cb, system, papplLogDevice, system);

  create_brf_printer(system);

  return (system);
}

// creating uri for cups-brf printer

int create_brf_printer(pappl_system_t *system)
{
  char *dir;
  struct passwd *pw;
  int ret;

  // Get the current user's information
  pw = getpwuid(getuid());
  if (pw == NULL)
  {
    fprintf(stderr, "ERROR: could not get user information\n");
    return 1;
  }

  // Create the directory path in the user's home directory
  if (asprintf(&dir, "%s/BRF", pw->pw_dir) < 0)
  {
    fprintf(stderr, "ERROR: could not allocate memory\n");
    return 1;
  }

  // Try creating the "BRF" directory with permissions 0700
  fprintf(stderr, "DEBUG: creating directory \"%s\"\n", dir);
  ret = mkdir(dir, 0700);
  if (ret == -1 && errno != EEXIST)
  {
    fprintf(stderr, "ERROR: could not create directory \"%s\": %s\n",
            dir, strerror(errno));
    free(dir);
    return 1;
  }
  else if (ret == -1)
  {
    fprintf(stderr, "DEBUG: directory \"%s\" already exists\n", dir);
  }
  else
  {
    fprintf(stderr, "DEBUG: directory \"%s\" created successfully\n", dir);
  }

  // Construct the device URI
  char device_uri[1024];
  snprintf(device_uri, sizeof(device_uri), "file://%s", dir);
  printf("[INFO] Device URI for printer: %s\n", device_uri);

  // Create the printer with the new device URI

  if (papplPrinterCreate(system, 0, "cups-brf", "gen_brf", NULL, device_uri) != NULL)
  {
    printf("[SUCCESS] Printer created with device URI: %s\n", device_uri);
  }
  else
  {
    fprintf(stderr, "[ERROR] Failed to create printer with device URI: %s\n", device_uri);
  }

  free(dir);

  return 0;
}

// 'BRFFilterCB()' - Print a document

// Items to configure the properties of this Printer Application
// These items do not change while the Printer Application is running

bool // O - `true` on success, `false` on failure
BRFFilterCB(
    pappl_job_t *job,       // I - Job
    pappl_device_t *device, // I - Output device
    void *cbdata)           // I - Callback data (not used)
{
  brf_printer_app_global_data_t *global_data = (brf_printer_app_global_data_t *)cbdata;

  // brf_job_data_t * job_data;
  const char *informat;
  const char *filename;                  // Input filename
  int fd;                                // Input file descriptor
  brf_spooling_conversion_t *conversion = NULL; // Spooling conversion to use for pre-filtering
  cf_filter_filter_in_chain_t *print;
  brf_print_filter_function_data_t *print_params;
  cf_filter_data_t *filter_data;
  cups_array_t *chain;
  int nullfd; // File descriptor for /dev/null
  char paramstr[1024];
  char buf[1024];

  bool ret = false;    // Return value

  pappl_pr_options_t *job_options = papplJobCreatePrintOptions(job, INT_MAX, 0);

  pappl_printer_t *printer = papplJobGetPrinter(job);
  const char *device_uri = papplPrinterGetDeviceURI(printer);

  ipp_t *driver_attrs = papplPrinterGetDriverAttributes(printer);

  paramstr[sizeof(paramstr) - 1] = 0;

  // Define an array of option names
  const char *option_names[] = {"PageSize","mirror","fitplot",
                                  "SendFF", "SendSUB",
                                 "LibLouis", "LibLouis2", "LibLouis3", "LibLouis4",
                                "TextDotDistance", "TextDots", "LineSpacing", "TopMargin", "BottomMargin",
                                "LeftMargin", "RightMargin", "BraillePageNumber", "PrintPageNumber",
                                "PageSeparator", "PageSeparatorNumber", "ContinuePages", "GraphicDotDistance",
                                "Rotate", "Edge", "Negate", "EdgeFactor", "CannyRadius", "CannySigma",
                                "CannyLower", "CannyUpper", "page-left", "page-right", "page-top", "page-bottom"};

  // Loop through each option and process them
  for (size_t i = 0; i < sizeof(option_names) / sizeof(option_names[0]); i++)
  {
    const char *option_name = option_names[i];
    ipp_attribute_t *attribute = papplJobGetAttribute(job, option_name);

   // If attribute is not found, look for the default attribute
    if (attribute == NULL) {
        snprintf(buf, sizeof(buf), "%s-default", option_name);
        attribute = ippFindAttribute(driver_attrs, buf, IPP_TAG_ZERO);
        if (attribute != NULL) {
            printf("Default attribute for %s: %s\n", option_name, ippGetName(attribute));
        }
    }

    if (attribute != NULL)
    {
      ipp_tag_t tagcheck = ippGetValueTag(attribute);

      if (tagcheck == IPP_TAG_INTEGER)
      {
        int value = ippGetInteger(attribute, 0);
        snprintf(paramstr, sizeof(paramstr) - 1, "%d", value);
      }
      else if (tagcheck == IPP_TAG_BOOLEAN)
      {
        bool boolean = ippGetBoolean(attribute, 0);
        strcpy(paramstr, boolean ? "True" : "False");
      }
      else
      {
        const char *str = ippGetString(attribute, 0, NULL);
        strncpy(paramstr, str, sizeof(paramstr) - 1);
      }

      // Add the option to job_options
      job_options->num_vendor = cupsAddOption(option_name, paramstr, job_options->num_vendor, &(job_options->vendor));
    }

    papplLogJob(job, PAPPL_LOGLEVEL_DEBUG, "****** Value for %s ****** %s\n", option_name, paramstr);
  }

  // Prepare job data to be supplied to filter functions/CUPS filters called during job execution
  filter_data = (cf_filter_data_t *)calloc(1, sizeof(cf_filter_data_t));
  if (!filter_data)
  {
    papplLogJob(job, PAPPL_LOGLEVEL_ERROR, "Failed to allocate memory for filter_data");
    return false;
  }

  papplLogJob(job, PAPPL_LOGLEVEL_DEBUG, "Allocated memory for filter_data");

  // Initialize filter_data fields
  filter_data->printer = strdup(papplPrinterGetName(printer));
  if (!filter_data->printer)
  {
    papplLogJob(job, PAPPL_LOGLEVEL_ERROR, "Failed to allocate memory for printer name");
    return false;
  }

  filter_data->job_id = papplJobGetID(job);
  filter_data->job_user = strdup(papplJobGetUsername(job));
  if (!filter_data->job_user)
  {
    papplLogJob(job, PAPPL_LOGLEVEL_ERROR, "Failed to allocate memory for job user");
    return false;
  }

  filter_data->job_title = strdup(papplJobGetName(job));
  if (!filter_data->job_title)
  {
    papplLogJob(job, PAPPL_LOGLEVEL_ERROR, "Failed to allocate memory for job title");
    return false;
  }

  papplLogJob(job, PAPPL_LOGLEVEL_DEBUG, "Job ID: %d, Job User: %s, Job Title: %s",
              filter_data->job_id, filter_data->job_user, filter_data->job_title);

  filter_data->copies = job_options->copies;
  filter_data->num_options = job_options->num_vendor;
  filter_data->options = job_options->vendor;
  filter_data->extension = NULL;
  filter_data->back_pipe[0] = -1;
  filter_data->back_pipe[1] = -1;
  filter_data->side_pipe[0] = -1;
  filter_data->side_pipe[1] = -1;
  filter_data->logdata = job;

  filter_data->logfunc = cfCUPSLogFunc;

  filter_data->logfunc = brf_JobLog; // Job log function catching page counts
                                     // ("PAGE: XX YY" messages)
  filter_data->logdata = job;
  filter_data->iscanceledfunc = brf_JobIsCanceled; // Function to indicate
                                                   // whether the job got
  // canceled
  filter_data->iscanceleddata = job;

  // Open the input file...
  filename = papplJobGetFilename(job);
  papplLogJob(job, PAPPL_LOGLEVEL_DEBUG, "Opening input file: %s", filename);
  if ((fd = open(filename, O_RDONLY)) < 0)
  {
    papplLogJob(job, PAPPL_LOGLEVEL_ERROR, "Unable to open input file '%s': %s", filename, strerror(errno));
    return false;
  }

  papplLogJob(job, PAPPL_LOGLEVEL_DEBUG, "Input file opened successfully");

  // Set up filter function chain
  chain = cupsArrayNew(NULL, NULL);

  // Get input file format
  informat = papplJobGetFormat(job);
  papplLogJob(job, PAPPL_LOGLEVEL_DEBUG, "Input file format: %s", informat);

  const char *currentFormat = informat;

  filter_data->content_type = strdup(currentFormat);
  filter_data->final_content_type = strdup(brf_MIMETYPE);

  // Progress with filters until the brf type
  while (strcmp(currentFormat, brf_MIMETYPE) != 0)
  {
    for (int i = 0; converts[i].srctype != NULL; i++)
    {
      conversion = &converts[i];
      // Compare source type with informat
      if (strcmp(conversion->srctype, informat) == 0)
      {
        break; // Exit the loop when a match is found
      }
    }

    if (conversion == NULL)
    {
      papplLogJob(job, PAPPL_LOGLEVEL_ERROR, "No pre-filter found for input format %s", informat);
      close(fd);
      return false;
    }
    papplLogJob(job, PAPPL_LOGLEVEL_DEBUG, "Using spooling conversion from %s to %s", conversion->srctype, conversion->dsttype);

    cupsArrayAdd(chain, &(conversion->filters));

    currentFormat = conversion->dsttype;
  }

  // Add print filter function at the end of the chain
  print = (cf_filter_filter_in_chain_t *)calloc(1, sizeof(cf_filter_filter_in_chain_t));

  if (!print)
  {
    papplLogJob(job, PAPPL_LOGLEVEL_ERROR, "Failed to allocate memory for print filter");
    close(fd);
    return false;
  }

  print_params = (brf_print_filter_function_data_t *)calloc(1, sizeof(brf_print_filter_function_data_t));
  if (!print_params)
  {
    papplLogJob(job, PAPPL_LOGLEVEL_ERROR, "Failed to allocate memory for print_params");
    close(fd);
    return false;
  }

  print_params->device = device;
  print_params->device_uri = device_uri;
  print_params->job = job;
  print_params->global_data = global_data;
  print->function = brf_print_filter_function;
  print->parameters = print_params;
  print->name = "Backend";

  cupsArrayAdd(chain, print);

  papplLogJob(job, PAPPL_LOGLEVEL_DEBUG, "Filter chain set up");

  // Fire up the filter functions
  papplJobSetImpressions(job, 1);
  nullfd = open("/dev/null", O_RDWR);

  if (cfFilterChain(fd, nullfd, 1, filter_data, chain) == 0)
  {
    papplLogJob(job, PAPPL_LOGLEVEL_DEBUG, "cfFilterChain() completed successfully");
    ret = true;
  }
  else
  {
    papplLogJob(job, PAPPL_LOGLEVEL_ERROR, "cfFilterChain() failed");
  }

  papplJobDeletePrintOptions(job_options);

  close(fd);
  close(nullfd);
  return ret;
}

int brf_print_filter_function(int inputfd, int outputfd, int inputseekable, cf_filter_data_t *data, void *parameters)
{
  ssize_t bytes;
  char buffer[65536];
  brf_print_filter_function_data_t *params = (brf_print_filter_function_data_t *)parameters;
  pappl_device_t *device = params->device;
  pappl_job_t *job = params->job;
  //brf_printer_app_global_data_t *global_data = params->global_data;
  //char filename[2048];
  int debug_fd = -1;

  // if (papplSystemGetLogLevel(global_data->system) == PAPPL_LOGLEVEL_DEBUG) {
  //     pappl_printer_t *printer = papplJobGetPrinter(job);
  //     snprintf(filename, sizeof(filename), "/tmp/brf-printer-app.%d.log", papplPrinterGetID(printer));
  //     debug_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  // }

  while ((bytes = read(inputfd, buffer, sizeof(buffer))) > 0)
  {
    if (debug_fd >= 0)
    {
      ssize_t done;

      for (done = 0; done < bytes; )
      {
        ssize_t storeBuffer = write(debug_fd, buffer + done, bytes - done);

        if (storeBuffer < 0)
        {
          papplLogJob(job, PAPPL_LOGLEVEL_ERROR, "Couldn't write %zd bytes to device", bytes);
          return 1;
        }
        done += storeBuffer;
      }
    }

    if (papplDeviceWrite(device, buffer, (size_t)bytes) < 0)
    {
      return 1;
    }
  }

  papplDeviceFlush(device);

  if (debug_fd >= 0)
    close(debug_fd);
  return 0;
}


//
// 'brf_JobIsCanceled()' - Return 1 if the job is canceled, which is
//                        the case when papplJobIsCanceled() returns
//                        true.
//

static int
brf_JobIsCanceled(void *data)
{
  pappl_job_t *job = (pappl_job_t *)data;

  return (papplJobIsCanceled(job) ? 1 : 0);
}

//
// 'brf_JobLog()' - Job log function which calls
//                 papplJobSetImpressionsCompleted() on page logs of
//                 filter functions
//

static void
brf_JobLog(void *data,
           cf_loglevel_t level,
           const char *message,
           ...)
{
  va_list arglist;
  pappl_job_t *job = (pappl_job_t *)data;
  char buf[1024];
  int page, copies;

  va_start(arglist, message);
  vsnprintf(buf, sizeof(buf) - 1, message, arglist);
  fflush(stdout);
  va_end(arglist);
  if (level == CF_LOGLEVEL_CONTROL)
  {
    if (sscanf(buf, "PAGE: %d %d", &page, &copies) == 2)
    {
      papplJobSetImpressionsCompleted(job, copies);
      papplLogJob(job, PAPPL_LOGLEVEL_DEBUG, "Printing page %d, %d copies",
                  page, copies);
    }
    else
      papplLogJob(job, PAPPL_LOGLEVEL_DEBUG, "Unused control message: %s",
                  buf);
  }
  else
    papplLogJob(job, (pappl_loglevel_t)level, "%s", buf);
}
