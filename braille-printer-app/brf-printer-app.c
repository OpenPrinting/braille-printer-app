

//
// Include necessary headers...
#include <strings.h>
#include <cupsfilters/log.h>
#include <cupsfilters/filter.h>
#include <ppd/ppd-filter.h>
#include <limits.h>
#include <pappl/pappl.h>




# define brf_TESTPAGE_HEADER	"T*E*S*T*P*A*G*E*"
#  define brf_TESTPAGE_MIMETYPE	"application/vnd.cups-paged-brf"


extern bool	brf_gen(pappl_system_t *system, const char *driver_name, const char *device_uri, const char *device_id, pappl_pr_driver_data_t *data, ipp_t **attrs, void *cbdata);
extern char* strdup(const char*);
//
// Local functions...
//
static bool BRFTestFilterCB(pappl_job_t *job,  pappl_device_t *device,void *cbdata) ; 
static int brf_print_filter_function(int inputfd,int outputfd, int inputseekable,cf_filter_data_t *data, void *parameters); 
static const char *autoadd_cb(const char *device_info, const char *device_uri, const char *device_id, void *cbdata);
static bool	driver_cb(pappl_system_t *system, const char *driver_name, const char *device_uri, const char *device_id, pappl_pr_driver_data_t *data, ipp_t **attrs, void *cbdata);
static int	match_id(int num_did, cups_option_t *did, const char *match_id);
static const char *mime_cb(const unsigned char *header, size_t headersize, void *data);
static bool	printer_cb(const char *device_info, const char *device_uri, const char *device_id, pappl_system_t *system);
static pappl_system_t *system_cb(int num_options, cups_option_t *options, void *data);


//
// Local globals...
//

static pappl_pr_driver_t	brf_drivers[] =
{					// Driver list
{ "gen_brf",  "Generic",
  NULL, NULL },

};
static char			brf_statefile[1024];
					// State file


//
// 'main()' - Main entry for brf.
//

int					// O - Exit status
main(int  argc,				// I - Number of command-line arguments
     char *argv[])			// I - Command-line arguments
{
  return (papplMainloop(argc, argv,
                        "1.0",
                        NULL,
                        (int)(sizeof(brf_drivers) / sizeof(brf_drivers[0])),
                        brf_drivers, autoadd_cb, driver_cb,
                        /*subcmd_name*/NULL, /*subcmd_cb*/NULL,
                        system_cb,
                        /*usage_cb*/NULL,
                        /*data*/NULL));
}


//
// 'autoadd_cb()' - Determine the proper driver for a given printer.
//

static const char *			// O - Driver name or `NULL` for none
autoadd_cb(const char *device_info,	// I - Device information/name (not used)
           const char *device_uri,	// I - Device URI
           const char *device_id,	// I - IEEE-1284 device ID
           void       *cbdata)		// I - Callback data (System)
{
  int		i,			// Looping var
		score,			// Current driver match score
	    	best_score = 0,		// Best score
		num_did;		// Number of device ID key/value pairs
  cups_option_t	*did;			// Device ID key/value pairs
  const char	*make,			// Manufacturer name
		*best_name = NULL;	// Best driver
 


  (void)device_info;

  // First parse the device ID and get any potential driver name to match...
  num_did = papplDeviceParseID(device_id, &did);

  if ((make = cupsGetOption("MANUFACTURER", num_did, did)) == NULL)
    if ((make = cupsGetOption("MANU", num_did, did)) == NULL)
      make = cupsGetOption("MFG", num_did, did);

  
  // // Then loop through the driver list to find the best match...
  // for (i = 0; i < (int)(sizeof(brf_drivers) / sizeof(brf_drivers[0])); i ++)
  {
 

    if (brf_drivers[i].device_id)
    {
      // See if we have a matching device ID...
      score = match_id(num_did, did, brf_drivers[i].device_id);
      if (score > best_score)
      {
        best_score = score;
        best_name  = brf_drivers[i].name;
      }
    }
   }

  // Clean up and return...
  cupsFreeOptions(num_did, did);

  return (best_name);
}



//
// 'match_id()' - Compare two IEEE-1284 device IDs and return a score.
//
// The score is 2 for each exact match and 1 for a partial match in a comma-
// delimited field.  Any non-match results in a score of 0.
//

static int				// O - Score
match_id(int           num_did,		// I - Number of device ID key/value pairs
         cups_option_t *did,		// I - Device ID key/value pairs
         const char    *match_id)	// I - Driver's device ID match string
{
  int		i,			// Looping var
		score = 0,		// Score
		num_mid;		// Number of match ID key/value pairs
  cups_option_t	*mid,			// Match ID key/value pairs
		*current;		// Current key/value pair
  const char	*value,			// Device ID value
		*valptr;		// Pointer into value


  // Parse the matching device ID into key/value pairs...
  if ((num_mid = papplDeviceParseID(match_id, &mid)) == 0)
    return (0);

  // Loop through the match pairs to find matches (or not)
  for (i = num_mid, current = mid; i > 0; i --, current ++)
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
        score ++;
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
}

//
// 'driver_cb()' - Main driver callback.
//

static bool				// O - `true` on success, `false` on error
driver_cb(
    pappl_system_t         *system,	// I - System
    const char             *driver_name,// I - Driver name
    const char             *device_uri,	// I - Device URI
    const char             *device_id,	// I - 1284 device ID
    pappl_pr_driver_data_t *data,	// I - Pointer to driver data
    ipp_t                  **attrs,	// O - Pointer to driver attributes
    void                   *cbdata)	// I - Callback data (not used)
{
  int	i;				// Looping var


  // Copy make/model info...
  for (i = 0; i < (int)(sizeof(brf_drivers) / sizeof(brf_drivers[0])); i ++)
  {
    if (!strcmp(driver_name, brf_drivers[i].name))
    {
      papplCopyString(data->make_and_model, brf_drivers[i].description, sizeof(data->make_and_model));
      break;
    }
  }

  // Pages per minute 
  data->ppm = 60;

 
  // Color values...
  data->color_supported   = PAPPL_COLOR_MODE_AUTO | PAPPL_COLOR_MODE_MONOCHROME;
  data->color_default   = PAPPL_COLOR_MODE_MONOCHROME;
  data->raster_types  = PAPPL_PWG_RASTER_TYPE_BLACK_8; // to be done just guess

  // "print-quality-default" value...
  data->quality_default = IPP_QUALITY_NORMAL;
  data->orient_default  = IPP_ORIENT_NONE;

  // "sides" values...
  data->sides_supported = PAPPL_SIDES_ONE_SIDED;
  data->sides_default   = PAPPL_SIDES_ONE_SIDED;

  // "orientation-requested-default" value...
  data->orient_default = IPP_ORIENT_NONE;


  // Test page callback...
 // data->testpage_cb = lprintTestPageCB;

  // Use the corresponding sub-driver callback to set things up...
  if (!strncmp(driver_name, "gen_", 4))
    return (brf_gen(system, driver_name, device_uri, device_id, data, attrs, cbdata));
 
  else
    return (false);
}

//
// 'mime_cb()' - MIME typing callback...
//

static const char *			// O - MIME media type or `NULL` if none
mime_cb(const unsigned char *header,	// I - Header data
        size_t              headersize,	// I - Size of header data
        void                *cbdata)	// I - Callback data (not used)
{
  
    return (brf_TESTPAGE_MIMETYPE);
  
  
}


//
// 'printer_cb()' - Try auto-adding printers.
//

static bool				// O - `false` to continue
printer_cb(const char     *device_info,	// I - Device information
	   const char     *device_uri,	// I - Device URI
	   const char     *device_id,	// I - IEEE-1284 device ID
	   pappl_system_t *system)	// I - System
{
  const char *driver_name = autoadd_cb(device_info, device_uri, device_id, system);
					// Driver name, if any

  if (driver_name)
  {
    char	name[128],		// Printer name
		*nameptr;		// Pointer in name
    papplCopyString(name, device_info, sizeof(name));

    if ((nameptr = strstr(name, " (")) != NULL)
      *nameptr = '\0';

    if (!papplPrinterCreate(system, 0, name, driver_name, device_id, device_uri))
    {
      // Printer already exists with this name, so try adding a number to the
      // name...
      int	i;			// Looping var
      char	newname[128],		// New name
		number[4];		// Number string
      size_t	namelen = strlen(name),	// Length of original name string
		numberlen;		// Length of number string

      for (i = 2; i < 100; i ++)
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


//
// 'system_cb()' - Setup the system object.
//

static pappl_system_t *			// O - System object
system_cb(
    int           num_options,		// I - Number options
    cups_option_t *options,		// I - Options
    void          *data)		// I - Callback data (unused)
{
  pappl_system_t	*system;	// System object
  const char		*val,		// Current option value
			*hostname,	// Hostname, if any
			*logfile,	// Log file, if any
			*system_name;	// System name, if any
  pappl_loglevel_t	loglevel;	// Log level
  int			port = 0;	// Port number, if any
  pappl_soptions_t	soptions = PAPPL_SOPTIONS_MULTI_QUEUE | PAPPL_SOPTIONS_WEB_INTERFACE | PAPPL_SOPTIONS_WEB_LOG | PAPPL_SOPTIONS_WEB_SECURITY;
					// System options
  static pappl_version_t versions[1] =	// Software versions
  {
    { "brf", "", 1.0, 1.0}};


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

  logfile     = cupsGetOption("log-file", num_options, options);
  hostname    = cupsGetOption("server-hostname", num_options, options);
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
  if ((system = papplSystemCreate(soptions, system_name ? system_name : "Braille printer app", port, "_print,_universal", cupsGetOption("spool-directory", num_options, options), logfile ? logfile : "-", loglevel, cupsGetOption("auth-service", num_options, options), /* tls_only */false)) == NULL)
    return (NULL);

  papplSystemAddListeners(system, NULL);
  papplSystemSetHostName(system, hostname);

  papplSystemSetMIMECallback(system, mime_cb, NULL);
  papplSystemAddMIMEFilter(system, "application/pdf", brf_TESTPAGE_MIMETYPE, BRFTestFilterCB, NULL);

  papplSystemSetPrinterDrivers(system, (int)(sizeof(brf_drivers) / sizeof(brf_drivers[0])), brf_drivers, autoadd_cb, /*create_cb*/NULL, driver_cb, system);

  
  papplSystemSetFooterHTML(system, "Copyright &copy; 2022 by Chandresh Soni. All rights reserved.");
  papplSystemSetSaveCallback(system, (pappl_save_cb_t)papplSystemSaveState, (void *)brf_statefile);
  papplSystemSetVersions(system, (int)(sizeof(versions) / sizeof(versions[0])), versions);

  fprintf(stderr, "brf: statefile='%s'\n", brf_statefile);

  if (!papplSystemLoadState(system, brf_statefile))
  {
    // No old state, use defaults and auto-add printers...
    papplSystemSetDNSSDName(system, system_name ? system_name : "brf");

    papplLog(system, PAPPL_LOGLEVEL_INFO, "Auto-adding printers...");
    papplDeviceList(PAPPL_DEVTYPE_USB, (pappl_device_cb_t)printer_cb, system, papplLogDevice, system);
  }

  return (system);
}


//
// 'BRFTestFilterCB()' - Print a test page.
//

// Items to configure the properties of this Printer Application
// These items do not change while the Printer Application is running
typedef struct brf_printer_app_config_s
{
  // Identification of the Printer Application
  const char        *system_name;        // Name of the system
  const char        *system_package_name;// Name of Printer Application
                                         // package/executable
  const char        *version;            // Program version number string
  unsigned short    numeric_version[4];  // Numeric program version
  const char        *web_if_footer;      // HTML Footer for web interface
  
  pappl_pr_autoadd_cb_t autoadd_cb;

  
  pappl_pr_identify_cb_t identify_cb;


  pappl_pr_testpage_cb_t testpage_cb;


  cups_array_t      *spooling_conversions;

  
  cups_array_t      *stream_formats;
  const char        *backends_ignore;

  const char        *backends_only;

  void              *testpage_data;

} pr_printer_app_config_t;

typedef struct brf_printer_app_global_data_s
{
  pr_printer_app_config_t *config;
  pappl_system_t          *system;
  int                     num_drivers;     // Number of drivers (from the PPDs)
  pappl_pr_driver_t       *drivers;        // Driver index (for menu and
                                           // auto-add)
   char              spool_dir[1024];     // Spool directory, customizable via
                                         // SPOOL_DIR environment variable                                         

} brf_printer_app_global_data_t;



// Data for brf_print_filter_function()
typedef struct brf_print_filter_function_data_s
// look-up table
{
  pappl_device_t *device;                    // Device
  const char *device_uri;                          // Printer device URI
  pappl_job_t *job;                          // Job
  brf_printer_app_global_data_t *global_data; // Global data
} brf_print_filter_function_data_t;

typedef struct brf_cups_device_data_s
{
  const char *device_uri;    // Device URI
  int inputfd,         // FD for job data input
      backfd,          // FD for back channel
      sidefd;          // FD for side channel
  int backend_pid;     // PID of CUPS backend
  double back_timeout, // Timeout back channel (sec)
      side_timeout;    // Timeout side channel (sec)

  cf_filter_filter_in_chain_t *chain_filter; // Filter from PPD file
  cf_filter_data_t *filter_data;             // Common data for filter functions
  cf_filter_external_t backend_params; // Parameters for launching
                                             // backend via ppdFilterExternalCUPS()
  bool internal_filter_data;                 // Is filter_data
                                             // internal?
} brf_cups_device_data_t;

typedef struct brf_spooling_conversion_s
{
  char *srctype;                   // Input data type
  char *dsttype;                   // Output data type
  int num_filters;                       // Number of filters
  cf_filter_filter_in_chain_t filters[]; // List of filters with
                                         // parameters
} brf_spooling_conversion_t;

static brf_spooling_conversion_t brf_convert_pdf_to_brf =
    {
        "application/pdf",
        "application/vnd.cups-paged-brf",
        1,
        {
          {
          cfFilterExternal,
          NULL,
          "txttobrf"
          }
        }
     };


bool // O - `true` on success, `false` on failure
BRFTestFilterCB(
    pappl_job_t *job,       // I - Job
    pappl_device_t *device, // I - Output device
    void *cbdata)           // I - Callback data (not used)
{
  pappl_pr_options_t *job_options = papplJobCreatePrintOptions(job, INT_MAX, 0);
  brf_spooling_conversion_t *conversion;     // Spooling conversion to use
                                             // for pre-filtering
  cf_filter_filter_in_chain_t *chain_filter, // Filter from PPD file
      *print;
  brf_cups_device_data_t *device_data = NULL;
  cf_filter_external_t *filter_data_ext;
  brf_print_filter_function_data_t *print_params;
  brf_printer_app_global_data_t *global_data;
  cf_filter_data_t *filter_data;
  cups_array_t *spooling_conversions;
  cups_array_t *chain;
  const char *informat;
  const char *filename;     // Input filename
  int fd;                   // Input file descriptor

  int nullfd;               // File descriptor for /dev/null

  bool ret = false;    // Return value
  int num_options = 0; // Number of PPD print options
  cups_option_t *options = NULL;
  cf_filter_external_t *ext_filter_params;

  pappl_pr_driver_data_t driver_data;
  pappl_printer_t *printer = papplJobGetPrinter(job);
  const char *device_uri = papplPrinterGetDeviceURI(printer);

  // Prepare job data to be supplied to filter functions/CUPS filters
  // called during job execution
  filter_data = (cf_filter_data_t *)calloc(1, sizeof(cf_filter_data_t));
  // job_data->filter_data = filter_data;
  filter_data->printer = strdup(papplPrinterGetName(printer));
  filter_data->job_id = papplJobGetID(job);
  filter_data->job_user = strdup(papplJobGetUsername(job));
  filter_data->job_title = strdup(papplJobGetName(job));
  filter_data->copies = job_options->copies;
  filter_data->job_attrs = NULL;     // We use PPD/filter options
  filter_data->printer_attrs = NULL; // We use the printer's PPD file
  filter_data->num_options = num_options;
  filter_data->options = options; // PPD/filter options
  filter_data->extension = NULL;
  filter_data->back_pipe[0] = -1;

  filter_data->back_pipe[1] = -1;
  filter_data->side_pipe[0] = -1;
  filter_data->side_pipe[1] = -1;
  filter_data->logdata = job;
  //filter_data->iscanceledfunc = papplJobIsCanceled(job); // Function to indicate
                                                         // whether the job got
                                                         // canceled
  filter_data->iscanceleddata = job;
  // //
  // // Load the printer's assigned PPD file, and find out which PPD option
  // // seetings correspond to our job options
  // //

  // papplLogJob(job, PAPPL_LOGLEVEL_DEBUG,
  //             "Printing job in spooling mode");

  // filter_data_ext =
  //     (ppd_filter_data_ext_t *)cf_filter_external_tilterDataGetExt(filter_data,
  //                                                 PPD_FILTER_DATA_EXT);

  //
  // Open the input file...
  //

  filename = papplJobGetFilename(job);
  if ((fd = open(filename, O_RDONLY)) < 0)
  {
    papplLogJob(job, PAPPL_LOGLEVEL_ERROR, "Unable to open input file '%s' for printing: %s",
                filename, strerror(errno));
    return (false);
  }

  //
  // Get input file format
  //

  informat = papplJobGetFormat(job);
  papplLogJob(job, PAPPL_LOGLEVEL_DEBUG,
              "Input file format: %s", informat);
  //
  // Passing values to ppdFilterExternalCUPS()
  //
  filter_data_ext = (cf_filter_external_t *)calloc(1, sizeof(cf_filter_external_t));
  
  filter_data_ext->filter = "usr/lib/cups/filter/txttobrf"; 
  filter_data_ext->num_options =0;
  filter_data_ext->options = NULL;
  filter_data_ext->envp= NULL;


  //
  // Find filters to use for this job
  //

  for (conversion =
           (brf_spooling_conversion_t *)
               cupsArrayFirst(spooling_conversions);
       conversion;
       conversion =
           (brf_spooling_conversion_t *)
               cupsArrayNext(spooling_conversions))
   
  {
    if (strcmp(conversion->srctype, informat) == 0)

      break;
    
  }
  if (conversion == NULL )
  {
    papplLogJob(job, PAPPL_LOGLEVEL_ERROR,
                "No pre-filter found for input format %s",
                informat);
    return (false);
  }
  // Set input and output formats for the filter chain
  filter_data->content_type = conversion->srctype;
  filter_data->final_content_type = conversion->dsttype;

  //
  // Connect the job's filter_data to the backend
  //

  if (strncmp(device_uri, "cups:", 5) == 0)
  {
    // Get the device data
    device_data = (brf_cups_device_data_t *)papplDeviceGetData(device);

    // Connect the filter_data
    device_data->filter_data = filter_data;
  }

  //
  // Set up filter function chain
  //

  chain = cupsArrayNew(NULL, NULL);

  for (int i = 0; i < conversion->num_filters; i++){
    cupsArrayAdd(chain, &(conversion->filters[i]));
    cupsArrayAdd(chain,&(filter_data_ext));
    }
    
    chain_filter = NULL;
  print =
      (cf_filter_filter_in_chain_t *)calloc(1, sizeof(cf_filter_filter_in_chain_t));
  // Put filter function to send data to PAPPL's built-in backend at the end
  // of the chain
  print_params =
      (brf_print_filter_function_data_t *)
          calloc(1, sizeof(brf_print_filter_function_data_t));
  print_params->device = device;
  print_params->device_uri = device_uri;
  print_params->job = job;
  print_params->global_data = global_data;
  print->function = brf_print_filter_function;
  print->parameters = print_params;
  print->name = "Backend";
  cupsArrayAdd(chain, print);

  // //
  // // Update status
  // //

  // pr_update_status(papplJobGetPrinter(job), device);

  //
  // Fire up the filter functions
  //

  papplJobSetImpressions(job, 1);

  // The filter chain has no output, data is going to the device
  nullfd = open("/dev/null", O_RDWR);

  if (cfFilterChain(fd, nullfd, 1, filter_data, chain) == 0)
    ret = true;

  // //
  // // Update status
  // //

  // pr_update_status(papplJobGetPrinter(job), device);

 

  return (ret);
}

int                                               // O - Error status
brf_print_filter_function(int inputfd,            // I - File descriptor input
                                                  //     stream
                          int outputfd,           // I - File descriptor output
                                                  //     stream (unused)
                          int inputseekable,      // I - Is input stream
                                                  //     seekable? (unused)
                          cf_filter_data_t *data, // I - Job and printer data
                          void *parameters)       // I - PAPPL output device
{
  ssize_t bytes;                    // Bytes read/written
  char buffer[65536];               // Read/write buffer
  cf_logfunc_t log = data->logfunc; // Log function
  void *ld = data->logdata;         // log function data
  brf_print_filter_function_data_t *params =
      (brf_print_filter_function_data_t *)parameters;
  pappl_device_t *device = params->device; // PAPPL output device
  pappl_job_t *job = params->job;
  pappl_printer_t *printer;
  brf_printer_app_global_data_t *global_data = params->global_data;
  char filename[2048]; // Name for debug copy of the
                       // job
  int debug_fd = -1;   // File descriptor for debug copy

  (void)inputseekable;

  // Remove debug copies of old jobs
 //pr_clean_debug_copies(global_data);

  if (papplSystemGetLogLevel(global_data->system) == PAPPL_LOGLEVEL_DEBUG)
  {
    // We are in debug mode
    // Debug copy file name (in spool directory)
    printer = papplJobGetPrinter(job);
    snprintf(filename, sizeof(filename), "%s/debug-jobdata-%s-%d.prn",
             global_data->spool_dir, papplPrinterGetName(printer),
             papplJobGetID(job));
    if (log)
      log(ld, CF_LOGLEVEL_DEBUG,
          "Backend: Creating debug copy of what goes to the printer: %s", filename);
    // Open the file
    debug_fd = open(filename, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
  }

  while ((bytes = read(inputfd, buffer, sizeof(buffer))) > 0)
  {
    if (debug_fd >= 0)
      if (write(debug_fd, buffer, (size_t)bytes) != bytes)
      {
        if (log)
          log(ld, CF_LOGLEVEL_ERROR,
              "Backend: Debug copy: Unable to write %d bytes, stopping debug copy, continuing job output.",
              (int)bytes);
        close(debug_fd);
        debug_fd = -1;
      }

    if (papplDeviceWrite(device, buffer, (size_t)bytes) < 0)
    {
      if (log)
        log(ld, CF_LOGLEVEL_ERROR,
            "Backend: Output to device: Unable to send %d bytes to printer.",
            (int)bytes);
      if (debug_fd >= 0)
        close(debug_fd);
      close(inputfd);
      close(outputfd);
      return (1);
    }
  }
  papplDeviceFlush(device);

  if (debug_fd >= 0)
    close(debug_fd);

  close(inputfd);
  close(outputfd);
  return (0);
}
