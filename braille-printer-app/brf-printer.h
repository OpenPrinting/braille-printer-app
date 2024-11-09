#include <pappl/pappl.h>
#include <cupsfilters/filter.h>
#include <ppd/ppd-filter.h>

pappl_content_t brf_GetFileContentType(pappl_job_t *job);

extern int brf_JobIsCanceled(void *data);

void brf_JobLog(void *data,cf_loglevel_t level,const char *message,...);

typedef struct brf_spooling_conversion_s
{
    char *srctype;                         // Input data type
    char *dsttype;                         // Output data type
    cf_filter_filter_in_chain_t filters ; // List of filters with
                                           // parameters
} brf_spooling_conversion_t;

typedef struct brf_printer_app_config_s
{
  // Identification of the Printer Application
  const char *system_name;           // Name of the system
  const char *system_package_name;   // Name of Printer Application
                                     // package/executable
  const char *version;               // Program version number string
  unsigned short numeric_version[4]; // Numeric program version
  const char *web_if_footer;         // HTML Footer for web interface

  pappl_pr_autoadd_cb_t autoadd_cb;

  pappl_pr_identify_cb_t identify_cb;

  pappl_pr_testpage_cb_t testpage_cb;

  cups_array_t *spooling_conversions;

  cups_array_t *stream_formats;
  const char *backends_ignore;

  const char *backends_only;

  void *testpage_data;

} brf_printer_app_config_t;


typedef struct brf_printer_app_global_data_s
{
  brf_printer_app_config_t *config;
  pappl_system_t *system;
  int num_drivers;            // Number of drivers (from the PPDs)
  pappl_pr_driver_t *drivers; // Driver index (for menu and
                              // auto-add)
  char spool_dir[1024];       // Spool directory, customizable via
                              // SPOOL_DIR environment variable

} brf_printer_app_global_data_t;

// Data for brf_print_filter_function()
typedef struct brf_print_filter_function_data_s
// look-up table
{
  pappl_device_t *device;                     // Device
  const char *device_uri;                     // Printer device URI
  pappl_job_t *job;                           // Job
  brf_printer_app_global_data_t *global_data; // Global data
} brf_print_filter_function_data_t;

typedef struct brf_cups_device_data_s
{
  const char *device_uri; // Device URI
  int inputfd,            // FD for job data input
      backfd,             // FD for back channel
      sidefd;             // FD for side channel
  int backend_pid;        // PID of CUPS backend
  double back_timeout,    // Timeout back channel (sec)
      side_timeout;       // Timeout side channel (sec)

  cf_filter_filter_in_chain_t *chain_filter; // Filter from PPD file
  cf_filter_data_t *filter_data;             // Common data for filter functions
  cf_filter_external_t backend_params;       // Parameters for launching
                                             // backend via ppdFilterExternalCUPS()
  bool internal_filter_data;                 // Is filter_data
                                             // internal?
} brf_cups_device_data_t;


static cf_filter_external_t texttobrf_filter = {

    .filter = "/usr/lib/cups/filter/texttobrf",
    .envp =  (char *[]) {
            "PPD=/dev/null",
            "CONTENT_TYPE=text/plain",  
            NULL
        }
};

// pdf_to_brf comes in texttobrf_filter

static cf_filter_external_t brftopagedbrf_filter = {

    .filter = "/usr/lib/cups/filter/brftopagedbrf",
    .envp =   (char *[]) {
            "PPD=/dev/null", 
            "CONTENT_TYPE=application/vnd.cups-brf",  
            NULL
        }
};

static cf_filter_external_t imagetobrf_filter = {

    .filter = "/usr/lib/cups/filter/imagetobrf",
    .envp =   (char *[]) {
            "PPD=/dev/null",
            "CONTENT_TYPE=image/jpeg",  
            NULL
        }
};

static cf_filter_external_t imagetoubrl_filter = {

    .filter = "/usr/lib/cups/filter/imagetoubrl",
    .envp = (char *[]) {
            "PPD=/dev/null", 
            "CONTENT_TYPE=image/jpeg",  
            NULL
        }
};


static cf_filter_external_t svgtopdf_filter = {

    .filter = "/usr/lib/cups/filter/svgtopdf",
    .envp = (char *[]) {
            "PPD=/dev/null",
            "CONTENT_TYPE=image/svg",  
            NULL
        }
};

static cf_filter_external_t xfigtopdf_filter = {

    .filter = "/usr/lib/cups/filter/xfigtopdf",
    .envp = (char *[]) {
            "PPD=/dev/null", 
            "CONTENT_TYPE=application/x-xfig",  
            NULL
        }
};

static cf_filter_external_t wmftopdf_filter = {

    .filter = "/usr/lib/cups/filter/wmftopdf",
    .envp = (char *[]) {
            "PPD=/dev/null",
            "CONTENT_TYPE=image/x-wmf",  
            NULL
        }
};

static cf_filter_external_t emftopdf_filter = {

    .filter = "/usr/lib/cups/filter/emftopdf",
    .envp = (char *[]) {
            "PPD=/dev/null",
            "CONTENT_TYPE=image/emf",  
            NULL
        }
};

static cf_filter_external_t cgmtopdf_filter = {

    .filter = "/usr/lib/cups/filter/cgmtopdf",
    .envp = (char *[]) {
            "PPD=/dev/null",
            "CONTENT_TYPE=image/cgm",  
            NULL
        }
};


static cf_filter_external_t cmxtopdf_filter = {

    .filter = "/usr/lib/cups/filter/cmxtopdf",
    .envp = (char *[]) {
            "PPD=/dev/null", 
            "CONTENT_TYPE=image/x-cmx",  
            NULL
        }
};


static cf_filter_external_t vectortobrf_filter = {

    .filter = "/usr/lib/cups/filter/vectortobrf",
    .envp = (char *[]) {
            "PPD=/dev/null", 
            "CONTENT_TYPE=image/vnd.cups-pdf",  
            NULL
        }
};


static cf_filter_external_t vectortoubrl_filter = {

    .filter = "/usr/lib/cups/filter/vectortoubrl",
    .envp = (char *[]) {
           "PPD=/dev/null", 
            "CONTENT_TYPE=image/vnd.cups-pdf",  
            NULL
        }
};


static brf_spooling_conversion_t converts[] =
{
    {
        "text/plain",
        "application/vnd.cups-brf",
            {cfFilterExternal, &texttobrf_filter,"texttobrf"}
    },

    {
        "text/html",
        "application/vnd.cups-brf",
            {cfFilterExternal, &texttobrf_filter,"texttobrf"}
    },
    {
        "application/xhtml",
        "application/vnd.cups-brf",
            {cfFilterExternal, &texttobrf_filter,"texttobrf"}
    },
    {
        "application/xml",
        "application/vnd.cups-brf",
            {cfFilterExternal, &texttobrf_filter,"texttobrf"}
    },
    {
        "application/sgml",
        "application/vnd.cups-brf",
            {cfFilterExternal, &texttobrf_filter,"texttobrf"}
    },

    {
        "application/vnd.cups-brf",
        "application/vnd.cups-paged-brf",
            {cfFilterExternal, &brftopagedbrf_filter, "brftopagedbrf"}
    },
    {
        "application/vnd.cups-ubrl",
        "application/vnd.cups-paged-ubrl",
            {cfFilterExternal, &brftopagedbrf_filter, "brftopagedbrf"}
    },
   
    {
        "application/msword",
        "application/vnd.cups-brf",
            {cfFilterExternal, &texttobrf_filter, "texttobrf"}
    },
   {
        "text/rtf",
        "application/vnd.cups-brf",
            {cfFilterExternal, &texttobrf_filter, "texttobrf"}
    },
    {
        "application/rtf",
        "application/vnd.cups-brf",
            {cfFilterExternal, &texttobrf_filter, "texttobrf"}
    },

    {
        "application/pdf",
        "application/vnd.cups-brf",
            {cfFilterExternal, &texttobrf_filter, "texttobrf"}
    },


    {
        "image/gif",
        "application/vnd.cups-brf",
            {cfFilterExternal, &imagetobrf_filter, "imagetobrf"}
    },
    {
        "image/jpeg",
        "application/vnd.cups-brf",
            {cfFilterExternal, &imagetobrf_filter, "imagetobrf"}
    },
    {
        "image/pcx",
        "application/vnd.cups-brf",
            {cfFilterExternal, &imagetobrf_filter, "imagetobrf"}
    },
    {
        "image/png",
        "application/vnd.cups-brf",
            {cfFilterExternal, &imagetobrf_filter, "imagetobrf"}
    },
    {
        "image/tiff",
        "application/vnd.cups-brf",
            {cfFilterExternal, &imagetobrf_filter, "imagetobrf"}
    },
    {
        "image/vnd.microsoft.icon",
        "application/vnd.cups-brff",
            {cfFilterExternal, &imagetobrf_filter, "imagetobrf"}
    },
    {
        "image/x-ms-bmp",
        "application/vnd.cups-brf",
            {cfFilterExternal, &imagetobrf_filter, "imagetobrf"}
    },
{
        "image/x-portable-anymap",
        "application/vnd.cups-brf",
            {cfFilterExternal, &imagetobrf_filter, "imagetobrf"}
    },
    {
        "image/x-portable-bitmap",
        "application/vnd.cups-brf",
            {cfFilterExternal, &imagetobrf_filter, "imagetobrf"}
    },
    {
        "image/x-portable-graymap",
        "application/vnd.cups-brf",
            {cfFilterExternal, &imagetobrf_filter, "imagetobrf"}
    },
    {
        "image/x-portable-pixmap",
        "application/vnd.cups-brf",
            {cfFilterExternal, &imagetobrf_filter, "imagetobrf"}
    },
    {
        "image/x-xbitmap",
        "application/vnd.cups-brf",
            {cfFilterExternal, &imagetobrf_filter, "imagetobrf"}
    },
    {
        "image/x-xpixmap",
        "application/vnd.cups-brf",
            {cfFilterExternal, &imagetobrf_filter, "imagetobrf"}
    },
    {
        "image/x-xwindowdump",
        "application/vnd.cups-brf",
            {cfFilterExternal, &imagetobrf_filter, "imagetobrf"}
    },

    

   {
        "image/gif",
        "application/vnd.cups-ubrl",
            {cfFilterExternal, &imagetoubrl_filter, "imagetoubrl"}
    },
    {
        "image/pcx",
        "image/vnd.cups-ubrl",
            {cfFilterExternal, &imagetoubrl_filter, "imagetoubrl"}
    },
    {
        "image/png",
        "image/vnd.cups-ubrl",
            {cfFilterExternal, &imagetoubrl_filter, "imagetoubrl"}
    },
    {
        "image/tiff",
        "image/vnd.cups-ubrl",
            {cfFilterExternal, &imagetoubrl_filter, "imagetoubrl"}
    },
    {
        "image/jpeg",
        "application/vnd.cups-ubrl",
            {cfFilterExternal, &imagetoubrl_filter, "imagetoubrl"}
    },
    {
        "image/vnd.microsoft.icon",
        "image/vnd.cups-ubrl",
            {cfFilterExternal, &imagetoubrl_filter, "imagetoubrl"}
    },
    {
        "image/x-ms-bmp",
        "image/vnd.cups-ubrl",
            {cfFilterExternal, &imagetoubrl_filter, "imagetobrf"}
    },
    {
        "image/x-portable-anymap",
        "image/vnd.cups-ubrl",
            {cfFilterExternal, &imagetoubrl_filter, "imagetoubrl"}
    },
    {
        "image/x-portable-bitmap",
        "image/vnd.cups-ubrl",
            {cfFilterExternal, &imagetoubrl_filter, "imagetoubrl"}
    },
    {
        "image/x-portable-graymap",
        "image/vnd.cups-ubrl",
            {cfFilterExternal, &imagetoubrl_filter, "imagetoubrl"}
    },
    {
        "image/x-portable-pixmap",
        "image/vnd.cups-ubrl",
            {cfFilterExternal, &imagetoubrl_filter, "imagetoubrl"}
    },
    {
        "image/x-xbitmap",
        "image/vnd.cups-ubrl",
            {cfFilterExternal, &imagetoubrl_filter, "imagetoubrl"}
    },
    {
        "image/x-xpixmap",
        "image/vnd.cups-ubrl",
            {cfFilterExternal, &imagetoubrl_filter, "imagetoubrl"}
    },
    {
        "image/x-xwindowdump",
        "image/vnd.cups-ubrl",
            {cfFilterExternal, &imagetoubrl_filter, "imagetoubrl"}
    },


    {
        "image/svg",
        "image/vnd.cups-pdf",
            {cfFilterExternal, &svgtopdf_filter, "svgtopdf"}
    },

    {
        "image/svg+xml",
        "image/vnd.cups-pdf",
            {cfFilterExternal, &svgtopdf_filter, "svgtopdf"}
    },

    {
        "application/x-xfig",
        "image/vnd.cups-pdf",
            {cfFilterExternal, &xfigtopdf_filter, "xfigtopdf"}
    },

    {
        "image/wmf",
        "image/vnd.cups-pdf",
            {cfFilterExternal, &wmftopdf_filter, "wmftopdf"}
    },

    {
        "image/x-wmf",
        "image/vnd.cups-pdf",
            {cfFilterExternal, &wmftopdf_filter, "wmftopdf"}
    },

    {
        "windows/metafile",
        "image/vnd.cups-pdf",
            {cfFilterExternal, &wmftopdf_filter, "wmftopdf"}
    },
    {
        "application/x-msmetafile",
        "image/vnd.cups-pdf",
            {cfFilterExternal, &wmftopdf_filter, "wmftopdf"}
    },
    {
        "image/emf",
        "image/vnd.cups-pdf",
            {cfFilterExternal, &emftopdf_filter, "emftopdf"}
    },
    {
        "image/x-emf",
        "image/vnd.cups-pdf",
            {cfFilterExternal, &emftopdf_filter, "emftopdf"}
    },
    {
        "image/cgm",
        "image/vnd.cups-pdf",
            {cfFilterExternal, &cgmtopdf_filter, "cgmtopdf"}
    },

    {
        "image/x-cmx",
        "image/vnd.cups-pdf",
            {cfFilterExternal, &cmxtopdf_filter, "cmxtopdf"}
    },

    {
        "image/vnd.cups-pdf",
        "image/vnd.cups-brf",
            {cfFilterExternal, &vectortobrf_filter, "vectortobrf"}
    },
    {
        "image/vnd.cups-pdf",
        "image/vnd.cups-ubrl",
            {cfFilterExternal, &vectortoubrl_filter, "vectortoubrl"}
    },
    {
    NULL
    }
};
