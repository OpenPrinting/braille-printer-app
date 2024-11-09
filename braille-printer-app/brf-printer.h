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

#include <pappl/pappl.h>
#include <cupsfilters/filter.h>

#define brf_MIMETYPE "application/vnd.cups-brf"

extern bool brf_gen(pappl_system_t *system, const char *driver_name, const char *device_uri, const char *device_id, pappl_pr_driver_data_t *data, ipp_t **attrs, void *cbdata);

typedef struct brf_spooling_conversion_s
{
    char *srctype;                         // Input data type
    char *dsttype;                         // Output data type
    cf_filter_filter_in_chain_t filters ; // List of filters with
                                           // parameters
} brf_spooling_conversion_t;


typedef struct brf_printer_app_global_data_s
{
  pappl_system_t *system;

} brf_printer_app_global_data_t;

extern brf_printer_app_global_data_t global_data;

// Data for brf_print_filter_function()
typedef struct brf_print_filter_function_data_s
// look-up table
{
  pappl_device_t *device;                     // Device
  const char *device_uri;                     // Printer device URI
  pappl_job_t *job;                           // Job
  brf_printer_app_global_data_t *global_data; // Global data
} brf_print_filter_function_data_t;

extern brf_spooling_conversion_t converts[];
