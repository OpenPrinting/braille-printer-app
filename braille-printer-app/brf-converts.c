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

#include <cupsfilters/filter.h>
#include "brf-printer.h"

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


brf_spooling_conversion_t converts[] =
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
