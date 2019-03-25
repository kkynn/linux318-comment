/*
 * Copyright (C) 2014 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : Definition of rtk/rg or customized snooping mode and API
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1)
 */

#include "voice_mgmt.h"

omci_voice_wrapper_t empty_voice_wrapper;

static omci_voice_wrapper_info_t voice_supported_mode[VOICE_VENDOR_END] =
{
    {VOICE_VENDOR_NONE, &empty_voice_wrapper, NULL, NULL, NULL},
    {VOICE_VENDOR_RTK,  NULL, NULL, NULL, NULL},
    /* Customers could use own handlers */
};

omci_voice_wrapper_t* omci_voice_wrapper_find(omci_voice_vendor_t voice_vendor)
{
	unsigned int  mode_index;

	for (mode_index = 0; mode_index < VOICE_VENDOR_END; mode_index++)
    {
        if (voice_supported_mode[mode_index].mode_id == voice_vendor)
        {
            return voice_supported_mode[mode_index].pWrapper;
        }
    }
	return 0;
}

int omci_voice_wrapper_register(omci_voice_vendor_t v, omci_voice_wrapper_t *p)
{

    if (!p)
        return -1;

    if (VOICE_VENDOR_END <= v)
        return -1;

    voice_supported_mode[v].pWrapper = p;

    return 0;
}

int omci_voice_wrapper_unregister(omci_voice_vendor_t v)
{

    if (VOICE_VENDOR_END <= v)
        return -1;

    voice_supported_mode[v].pWrapper = &empty_voice_wrapper;


    return 0;
}


static void omci_voice_util_deinit(omci_voice_vendor_t v)
{

    if (VOICE_VENDOR_END <= v)
        return;

    voice_supported_mode[v].pHandler = NULL;

    if (voice_supported_mode[v].pUtilSoHandle)
    {
        dlclose(voice_supported_mode[v].pUtilSoHandle);
        voice_supported_mode[v].pUtilSoHandle = NULL;
    }
    return;
}

static void omci_voice_util_init(omci_voice_vendor_t v)
{
	DIR             *dir                = NULL;
	void            *handle             = NULL;
	struct dirent   *ptr;
	struct stat     fileStat;
	char            strModuleFile[256]  = "";


    if (VOICE_VENDOR_END <= v)
        return;

    if (VOICE_VENDOR_NONE == v)
        voice_supported_mode[v].pHandler = NULL;

	if ((dir = opendir("/lib/voip/")))
    {
        while ((ptr = readdir(dir)) != NULL)
        {
			sprintf(strModuleFile, "%s%s", "/lib/voip/", ptr->d_name);

			if( stat(strModuleFile, &fileStat) < 0 )
                continue; /* stat failed */

			if (!S_ISREG(fileStat.st_mode))
				continue; /* Not a file */

			/*check .so file only*/
			if (!strstr(strModuleFile, "omci_voice_util.so"))
				continue; /*Not a share library*/

			dlerror(); /* Clear any existing error */

            if (!(handle = dlopen(strModuleFile, RTLD_LAZY)))
                continue;

            voice_supported_mode[v].pUtilSoHandle = handle;

            if (!(voice_supported_mode[v].pHandler = dlsym(handle, "omci_voice_service_handler")))
                continue;
		}
		closedir(dir);
	}
    return;
}


void omci_voice_vendor_service_cb(omci_voice_vendor_t v, void *p)
{
  //  GOS_ERROR_CODE ret = GOS_OK;

    if (voice_supported_mode[v].pHandler)
    {
        (voice_supported_mode[v].pHandler)(p);
    }
    return;
}

void omci_voice_vendor_init(omci_voice_vendor_t vendor_id)
{
	int ret = 0;
	DIR *dir;
	void *handle = NULL;
	struct dirent *ptr;
	struct stat fileStat;
	char strModuleFile[256] = "";

    if (VOICE_VENDOR_END <= vendor_id)
        return;

    if (VOICE_VENDOR_NONE == vendor_id)
    {
        memset(&empty_voice_wrapper, 0, sizeof(omci_voice_wrapper_t));
        return;
    }
	int (*omci_voice_init)(omci_voice_vendor_t);

	if ((dir = opendir("/lib/voip/")))
    {
        while ((ptr = readdir(dir)) != NULL)
        {
			sprintf(strModuleFile, "%s%s", "/lib/voip/", ptr->d_name);

			if (stat(strModuleFile, &fileStat) < 0)
                continue; /* stat failed*/

			if (!S_ISREG(fileStat.st_mode))
				continue; /* Not a file */

			/*check .so file only*/
			if (!strstr(strModuleFile, "voice_rtk_wrapper.so"))
				continue; /*Not a share library*/

			dlerror(); /* Clear any existing error */

            if (!(handle = dlopen(strModuleFile, RTLD_LAZY)))
                continue;

            voice_supported_mode[vendor_id].pWrapSoHandle = handle;

            if (!(omci_voice_init = dlsym(handle, "omci_voice_init")))
                continue;

            if (0 != (ret = omci_voice_init(vendor_id)))
                continue;

            omci_voice_util_init(vendor_id);

		}
		closedir(dir);
	}
    return;
}

void omci_voice_vendor_deinit(omci_voice_vendor_t vendor_id)
{
    int ret = 0;
    int (*omci_voice_deinit)(omci_voice_vendor_t);

    if (VOICE_VENDOR_END <= vendor_id)
        return;

    // remove dependency: omci_voice_util_deinit
    omci_voice_util_deinit(vendor_id);

    // remove dependency: omci_voice_deinit
    if (voice_supported_mode[vendor_id].pWrapSoHandle &&
        (NULL != (omci_voice_deinit = dlsym(voice_supported_mode[vendor_id].pWrapSoHandle, "omci_voice_deinit"))))
    {
        if (0 != (ret = omci_voice_deinit(vendor_id)))
        {
            printf("\n omci_voice_deinit failed\n");
        }
        dlclose(voice_supported_mode[vendor_id].pWrapSoHandle);
        voice_supported_mode[vendor_id].pWrapSoHandle = NULL;
    }
    return;
}

