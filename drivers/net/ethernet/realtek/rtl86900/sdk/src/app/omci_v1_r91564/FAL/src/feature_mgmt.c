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
 * Purpose : Feature API layer for hooking APIs prvoided by internal shared library or external shared library
 *           internal shared libaray is used to dynamically load it that is customzied behavior for customer's requirement
 *           external shared libaray is used to dynamically load it that is communication with third-party application. ex: igmp, voip, etc.
 * Feature : The file includes the following modules and sub-modules
 *           (1)
 */

#include <dlfcn.h>

#include "feature_mgmt.h"

typedef enum
{
    FEATURE_INTERNAL_MODULE_USER,
    FEATURE_INTERNAL_MODULE_KERNEL,
    FEATURE_INTERNAL_MODULE_END
} FEATURE_INTERNAL_MODULE_TYPE;

#define INIT_SYMBOL_NAME    "feature_module_init"
#define INIT_OPT_SYMBOL_NAME "feature_module_opt_init"
#define EXIT_SYMBOL_NAME    "feature_module_exit"
#define OMCI_CUSTOMOPTCONF_FILE_PATH "/var/config/omci_custom_opt.conf"

static unsigned int gFeatureMgmtInit      = 0;
static unsigned int gFeatureModuleCount   = 0;
static unsigned int gFeatureApiMax        = 0;
static feature_api_t *p_feature_api_db;
static unsigned int **pp_internal_feature_set = NULL;
static char **pp_prefix;
static unsigned gFeatureOptCount = 0;
static omci_custom_option_t* omciCustomOptCfg;

unsigned int feature_api_is_registered(unsigned int apiID)
{
    if (gFeatureApiMax <= apiID)
        return FAL_ERR_NOT_REGISTER;

    if (!(p_feature_api_db[apiID].regCB))
        return FAL_ERR_NOT_REGISTER;
    else
        return FAL_OK;
}

static unsigned int feature_reg_module_name_get(unsigned int moduleID, char *buf)
{
    feature_module_t *pModule = NULL;

    LIST_FOREACH(pModule, &featureRegModuleHead, entries)
    {
        if (pModule->moduleID == moduleID)
        {
            strncpy(buf, pModule->moduleName, strlen(pModule->moduleName) + 1);
            return FAL_OK;
        }
    }
    return FAL_ERR_NOT_FOUND;
}

#if 0
static unsigned int feature_reg_module_id_get(char  *name, unsigned int *p_moduleID)
{
    feature_module_t *pModule = NULL;

    LIST_FOREACH(pModule, &featureRegModuleHead, entries)
    {
        if (!strncmp(pModule->moduleName, name, FEATURE_MODULE_MAX_LENGTH))
        {
            *p_moduleID = pModule->moduleID;
            return FAL_OK;
        }
    }
    return FAL_ERR_NOT_FOUND;
}
#endif

static unsigned int 
feature_module_register(
    char *myModuleName, 
    unsigned int moduleId, 
    void *pHandle,
    signed opt)
{
    feature_module_t *pModule = (feature_module_t *)malloc(sizeof(feature_module_t));

    if (!pModule)
        return FAL_FAIL;

    gFeatureModuleCount++;

    if (moduleId != gFeatureModuleCount)
    {
        free(pModule);
        return FAL_FAIL;
    }
    pModule->moduleID = gFeatureModuleCount;
    memset(pModule->moduleName, 0, sizeof(pModule->moduleName));
    strncpy(pModule->moduleName, myModuleName, strlen(myModuleName) + 1);
    pModule->p_handle = pHandle;
    pModule->option   = opt;
    LIST_INSERT_HEAD(&featureRegModuleHead, pModule, entries);

    return FAL_OK;
}

static unsigned int feature_module_unregister(char *name)
{
    feature_module_t *pModule = NULL;
    unsigned int (*moduleExitFunc)(void);

    LIST_FOREACH(pModule, &featureRegModuleHead, entries)
    {
        if (!strncmp(pModule->moduleName, name, FEATURE_MODULE_MAX_LENGTH))
        {
            if (pModule->p_handle && (NULL != (moduleExitFunc = dlsym(pModule->p_handle, "feature_module_exit"))))
            {
                moduleExitFunc();
                dlclose(pModule->p_handle);
            }
            pModule->p_handle = NULL;
            LIST_REMOVE(pModule, entries);
            gFeatureModuleCount--;
            return FAL_OK;
        }
    }
    return FAL_ERR_NOT_FOUND;
}

unsigned int feature_api_register(unsigned int apiID, unsigned int moduleID, unsigned int (*regFun)(va_list argp))
{
    if (p_feature_api_db[apiID].regCB)
    {
        char modName[FEATURE_MODULE_MAX_LENGTH];

        if (FAL_OK != feature_reg_module_name_get(apiID, modName))
            return FAL_FAIL;

        printf("Feature API:%u has already been register by %s, you can't register it twice...\n", apiID, modName);

        return FAL_FAIL;
    }
    else
    {
        p_feature_api_db[apiID].regApiId      = apiID;
        p_feature_api_db[apiID].regCB         = regFun;
        p_feature_api_db[apiID].regModuleId   = moduleID;
    }
    return FAL_OK;
}

void feature_api_unregister(unsigned int apiID)
{
    p_feature_api_db[apiID].regApiId      = 0;
    p_feature_api_db[apiID].regCB         = NULL;
    p_feature_api_db[apiID].regModuleId   = 0;
}

void feature_reg_module_show(void)
{
    feature_module_t *pModule = NULL;

    if (0 == gFeatureMgmtInit)
    {
        printf("Feature Mgmt not init!!\n");
        return;
    }
    printf("\nFeature Module Information\n");
    printf("[Module ID]\t[Module Name]\t\t\t\t\t[Option]\n");
    printf("==========================================================================\n");

    LIST_FOREACH(pModule, &featureRegModuleHead, entries)
    {
        if (pModule->option >= 0)
        {
            printf("%u\t\t%s\t\t%d\n", pModule->moduleID, pModule->moduleName, pModule->option);
        } 
        else 
        {
            printf("%u\t\t%s\n", pModule->moduleID, pModule->moduleName);
        }
    }
    printf("==========================================================================\n");
    return;
}

void feature_reg_api_show(void)
{
    unsigned int i = 0;
    char modName[FEATURE_MODULE_MAX_LENGTH];

    if (0 == gFeatureMgmtInit)
    {
        printf("Feature Mgmt not init!!\n");
        return;
    }

    printf("feature api status\n");
    printf("======================================================\n");

    for (i = FEATURE_API_START; i < gFeatureApiMax; i++)
    {
        printf("API:%u\t\n", i);
        if (!p_feature_api_db[i].regCB)
        {
            printf("Not Registered\n");
        }
        else
        {
            if (FAL_OK != feature_reg_module_name_get(p_feature_api_db[i].regModuleId, modName))
                continue;

            printf("Registered by %s\n", modName);
        }
    }
}

unsigned int feature_api(unsigned int apiID, ...)
{
    va_list argptr;
    va_list argToAPI;
    unsigned int ret;

    if (0 == gFeatureMgmtInit)
        return FAL_ERR_NOT_INIT;

    if (gFeatureApiMax <= apiID)
        return FAL_ERR_NOT_REGISTER;

    if (!p_feature_api_db[apiID].regCB)
        return FAL_ERR_NOT_REGISTER;

    va_start(argptr, apiID );
    va_copy(argToAPI, argptr);
    va_end (argptr);

    ret = p_feature_api_db[apiID].regCB(argToAPI);
    va_end (argToAPI);

    return ret;
}

signed fMgmInit_findOption (
    unsigned type,
    unsigned customNum
)
{
    unsigned idx = 0;
    signed   opt = -1;

    for (idx = 0; idx < gFeatureOptCount; idx ++)
    {
        if (!omciCustomOptCfg)
        {
            break;
        }
        if (omciCustomOptCfg[idx].customMod == type &&
                omciCustomOptCfg[idx].customNum & customNum)
        {
            opt = omciCustomOptCfg[idx].customOpt;
            break;
        }
    }

    return opt;
}

void fMgmInit_loadOption (
    void
) 
{
    char* line = NULL;
    size_t len =  0;
    ssize_t read;
    char* token = NULL;
    unsigned idx = 0;
    unsigned tokIdx = 0;
    unsigned type   = 0xFFFFFFFF;
    FILE* optCfgFp = NULL;

    if (access(OMCI_CUSTOMOPTCONF_FILE_PATH, F_OK) == -1)
    {
        printf("%s doesn't exist\n", OMCI_CUSTOMOPTCONF_FILE_PATH);
        return;
    }

    if ((optCfgFp = fopen(OMCI_CUSTOMOPTCONF_FILE_PATH, "r")) == NULL)
    {
        printf("%s: Failed to load %s\n", __FUNCTION__, OMCI_CUSTOMOPTCONF_FILE_PATH);
        return;
    }

    while ((read = getline(&line, &len, optCfgFp)) != -1)
    {

        if (*line != '#' && *line != '\n' && *line != 0xA)
        {
            gFeatureOptCount++;
        }
    }

    omciCustomOptCfg = (omci_custom_option_t *)malloc(sizeof(omci_custom_option_t) * gFeatureOptCount);
    if (!omciCustomOptCfg)
    {
        printf("Failed to create the configuration of OMCI Custom Option\n");
        fclose(optCfgFp);
        optCfgFp = NULL;
        return;
    }
    else
    {
        memset(omciCustomOptCfg, 0x00, sizeof(omci_custom_option_t) * gFeatureOptCount);
        fclose(optCfgFp);
        optCfgFp = NULL;
    }

    if ((optCfgFp = fopen (OMCI_CUSTOMOPTCONF_FILE_PATH, "r")) == NULL)
    {
        printf("%s: Failed to load %s\n", __FUNCTION__, OMCI_CUSTOMOPTCONF_FILE_PATH);
        return;
    } 
    else 
    {
        while ((read = getline (&line, &len, optCfgFp)) != -1)
        {
            token = strtok (line, " ");

            if (!token || *token == '#' || *token == '\n')
            {
                idx = 0;
                continue;
            }
            
            if (!omciCustomOptCfg)
            {
                break;
            }
            
            type = strtol(token, NULL, 16); 
            omciCustomOptCfg[idx].customMod = type;
            while (token != NULL)
            {
                tokIdx ++;
                token = strtok(NULL, " ");
                if (!token )
                {
                    break;
                }

                if (tokIdx == 1)
                {
                    omciCustomOptCfg[idx].customNum = strtol(token, NULL, 16);
                }
                else if (tokIdx == 2)
                {
                    omciCustomOptCfg[idx].customOpt = strtol(token, NULL, 16);
                }
            }
            tokIdx = 0;
            idx++;
        }
    }

    fclose (optCfgFp);
    optCfgFp = NULL;
    if (line)
    {
        free (line);
    }
}

void fMgmtInit_external(unsigned char isLoad, unsigned int mask)
{
    char cmd[FEATURE_MODULE_MAX_LENGTH];

    if (mask & pp_internal_feature_set[0][4])
    {
        sprintf(cmd, "%s %s",
                (!isLoad ? "/bin/rmmod" : "/bin/insmod"),
                FEATURE_MODULE_EXTERNAL_PATH_NAME);
        system(cmd);
    }
    return;
}

void fMgmtInit_internal(unsigned int old_mask, unsigned int new_mask, unsigned int type)
{
    unsigned int idx, del_mask = 0, add_mask = 0;
    void        *handle = NULL;
    struct stat fileStat;
    char        strModuleFile[FEATURE_INTERNAL_MODULE_END][FEATURE_MODULE_MAX_LENGTH];
    char        cmd[FEATURE_MODULE_MAX_LENGTH];
    unsigned int (*moduleInitFunc)(unsigned int);
    unsigned int (*moduleOptInitFunc)(unsigned int, ...);
    unsigned int  (*moduleExitFunc)(void);

    if (new_mask == old_mask && new_mask)
    {
        add_mask = new_mask;
        fMgmtInit_external(1, add_mask);
        goto hook_internal_feature;
    }
    else if (old_mask != new_mask)
    {
        // remove customized behavior
        del_mask = (old_mask & (~new_mask));
        if (del_mask)
        {
            for (idx = 0; idx < feature_max_col_get(); idx++)
            {
                if (0 == pp_internal_feature_set[type][idx])
                    continue;

                if (del_mask & pp_internal_feature_set[type][idx])
                {
                    memset(strModuleFile[FEATURE_INTERNAL_MODULE_USER], 0, FEATURE_MODULE_MAX_LENGTH);
                    memset(strModuleFile[FEATURE_INTERNAL_MODULE_KERNEL], 0, FEATURE_MODULE_MAX_LENGTH);
                    sprintf(strModuleFile[FEATURE_INTERNAL_MODULE_USER], "%s%s%08x.so", FEATURE_MODULE_INTERNAL_USER_PATH, pp_prefix[type], pp_internal_feature_set[type][idx]);
                    sprintf(strModuleFile[FEATURE_INTERNAL_MODULE_KERNEL], "%s%s%08x.ko", FEATURE_MODULE_INTERNAL_KERNEL_PATH, pp_prefix[type], pp_internal_feature_set[type][idx]);
                    // handle with kernel module
                    if (stat(strModuleFile[FEATURE_INTERNAL_MODULE_KERNEL], &fileStat) == 0)
                    {
                        memset(cmd, 0, FEATURE_MODULE_MAX_LENGTH);
                        sprintf(cmd, "/bin/rmmod %s", strModuleFile[FEATURE_INTERNAL_MODULE_KERNEL]);
                        system(cmd);
                    }
                    // handle with shared library
                    if (FAL_OK != feature_module_unregister(strModuleFile[FEATURE_INTERNAL_MODULE_USER]))
                        continue;

                    printf("Module unregister successfully %s\n", strModuleFile[FEATURE_INTERNAL_MODULE_USER]);

                }
            }
            fMgmtInit_external(0, del_mask);
        }
        // insert customized behavior
        add_mask = ((~old_mask) & new_mask);
        if (add_mask)
        {
            fMgmtInit_external(1, add_mask);
            goto hook_internal_feature;
        }
    }
    return;

hook_internal_feature:

    for (idx = 0; idx < feature_max_col_get(); idx++)
    {
        signed  opt = -1;

        if (0 == pp_internal_feature_set[type][idx])
            continue;

        if (add_mask & pp_internal_feature_set[type][idx])
        {
            memset(strModuleFile[FEATURE_INTERNAL_MODULE_USER], 0, FEATURE_MODULE_MAX_LENGTH);
            memset(strModuleFile[FEATURE_INTERNAL_MODULE_KERNEL], 0, FEATURE_MODULE_MAX_LENGTH);
            sprintf(strModuleFile[FEATURE_INTERNAL_MODULE_USER], "%s%s%08x.so", FEATURE_MODULE_INTERNAL_USER_PATH, pp_prefix[type], pp_internal_feature_set[type][idx]);
            sprintf(strModuleFile[FEATURE_INTERNAL_MODULE_KERNEL], "%s%s%08x.ko", FEATURE_MODULE_INTERNAL_KERNEL_PATH, pp_prefix[type], pp_internal_feature_set[type][idx]);
            /* get option of feature */
            opt = fMgmInit_findOption(type, pp_internal_feature_set[type][idx]);
            // handle with kernel module
            memset(&fileStat, 0, sizeof(struct stat));
            if (stat(strModuleFile[FEATURE_INTERNAL_MODULE_KERNEL], &fileStat) == 0)
            {
                // file exist
                memset(cmd, 0, FEATURE_MODULE_MAX_LENGTH);
                
                if (-1 != opt)
                {
                    sprintf(cmd, "/bin/insmod %s customOpt=%d", strModuleFile[FEATURE_INTERNAL_MODULE_KERNEL], opt);
                }
                else
                {
                    sprintf(cmd, "/bin/insmod %s", strModuleFile[FEATURE_INTERNAL_MODULE_KERNEL]);
                }
                system(cmd);
            }

            // handle with shared library
            memset(&fileStat, 0, sizeof(struct stat));
            if (stat(strModuleFile[FEATURE_INTERNAL_MODULE_USER], &fileStat) < 0)
            {
                printf("stat failed: %s\n", strModuleFile[FEATURE_INTERNAL_MODULE_USER]);
                continue;
            }

            if (!S_ISREG(fileStat.st_mode))
                continue;

            if (!(handle = dlopen(strModuleFile[FEATURE_INTERNAL_MODULE_USER], RTLD_LAZY)))
            {
                printf("dlopen failed: %s\n", strModuleFile[FEATURE_INTERNAL_MODULE_USER]);
                continue;
            }

            if ((moduleInitFunc = dlsym(handle, INIT_SYMBOL_NAME)))
            {
                if (FAL_OK != moduleInitFunc(gFeatureModuleCount + 1))
                {
                    dlclose(handle);
                    printf("Module:%s feature_module_init failed\n", strModuleFile[FEATURE_INTERNAL_MODULE_USER]);
                    continue;
                }
            } 
            else if ((moduleOptInitFunc = dlsym(handle, INIT_OPT_SYMBOL_NAME)))
            {
                printf("load moduleOptInitFunc\n");
                if (FAL_OK != moduleOptInitFunc(gFeatureModuleCount + 1, opt))
                {
                    dlclose(handle);
                    printf("Module:%s feature_module_init failed\n", strModuleFile[FEATURE_INTERNAL_MODULE_USER]);
                    continue;
                }
            } 
            else 
            {
                dlclose(handle);
                printf("Module:%s doesn't implement feature_module_init function\n", strModuleFile[FEATURE_INTERNAL_MODULE_USER]);
                continue;
            }

            if (FAL_OK != feature_module_register(strModuleFile[FEATURE_INTERNAL_MODULE_USER], gFeatureModuleCount + 1, handle, opt))
            {
                if (NULL != (moduleExitFunc = dlsym(handle, EXIT_SYMBOL_NAME)))
                {
                    moduleExitFunc();
                    dlclose(handle);
                    printf("resource is not enough dlclose %s\n", strModuleFile[FEATURE_INTERNAL_MODULE_USER]);
                    continue;
                }
            }

            printf("Module %s is inited\n", strModuleFile[FEATURE_INTERNAL_MODULE_USER]);
        }
    }
    return;
}

void fMgmtInit(omci_customer_feature_flag_t flag, unsigned int api_cnt_max)
{
    DIR *dir;
    void *handle;
    struct dirent *ptr;
    struct stat fileStat;
    char strModuleFile[FEATURE_MODULE_MAX_LENGTH];
    unsigned int (*moduleInitFunc)(unsigned int);
    unsigned int (*moduleExitFunc)(void);

    pp_prefix = feature_prefix_info_get();
    pp_internal_feature_set = (unsigned int **)malloc(sizeof(unsigned int *) * feature_max_col_get());

    if (!pp_internal_feature_set)
        return;

    if (0 > feature_info_get(pp_internal_feature_set))
        return;

    gFeatureApiMax = api_cnt_max;

    /* Initail feature api db */
    if (!(p_feature_api_db = (feature_api_t *)malloc(sizeof(feature_api_t) * gFeatureApiMax)))
    {
        printf("DB setup failed due to resource is not enough\n");
        return;
    }

    memset(p_feature_api_db, 0, sizeof(feature_api_t) * gFeatureApiMax);
    /* Load option of features */
    fMgmInit_loadOption ();

    /* Initail feature module link list */
    LIST_INIT(&featureRegModuleHead);

    /* loading common shared library */
    if ((dir = opendir(FEATURE_MODULE_PATH)))
    {
        while (NULL != (ptr = readdir(dir)))
        {
            sprintf(strModuleFile, "%s%s", FEATURE_MODULE_PATH, ptr->d_name);

            if(stat(strModuleFile, &fileStat) < 0)
            {
                printf("stat failed: %s", strModuleFile);
                continue;
            }

            if (!S_ISREG(fileStat.st_mode))
            {
                fprintf(stderr, "<%s:%d> is the so is not a regular file.\n", __FUNCTION__, __LINE__);
                continue;
            }
            dlerror();

            if (!(handle = dlopen(strModuleFile, RTLD_LAZY)))
            {
                fprintf(stderr, "<%s:%d> %s\n", __FUNCTION__, __LINE__, dlerror());
                continue;
            }
            moduleInitFunc = dlsym(handle, INIT_SYMBOL_NAME);

            if (!moduleInitFunc)
            {
                dlclose(handle);
                printf("Module:%s doesn't implement feature_module_init function\n", strModuleFile);
                continue;
            }

            if (FAL_OK != moduleInitFunc(gFeatureModuleCount + 1))
            {
                dlclose(handle);
                printf("Module:%s feature_module_init failed\n", strModuleFile);
                continue;
            }

            if (FAL_OK != feature_module_register(ptr->d_name, gFeatureModuleCount + 1, handle, -1))
            {
                if (NULL != (moduleExitFunc = dlsym(handle, EXIT_SYMBOL_NAME)))
                {
                    moduleExitFunc();
                    dlclose(handle);
                    printf("resource is not enough dlclose %s\n", ptr->d_name);
                    continue;
                }
            }

            printf("Module %s is loaded and inited\n", ptr->d_name);
        }
    }

    /* loading customized shared library / kernel module by BOA MIB configuration */
    fMgmtInit_internal(flag.bridgeDP, flag.bridgeDP, 0);

    fMgmtInit_internal(flag.routeDP, flag.routeDP, 1);

    fMgmtInit_internal(flag.multicast, flag.multicast, 2);

    fMgmtInit_internal(flag.me, flag.me, 3);

    gFeatureMgmtInit = 1;
    closedir((DIR *) dir);
    return;
}

void 
fMgmtDeinit(
    void
)
{
    if (omciCustomOptCfg)
    {
        printf ("Freeing the featureOption ... \n");
        free (omciCustomOptCfg);
        omciCustomOptCfg = NULL;
    }
    return;
}

