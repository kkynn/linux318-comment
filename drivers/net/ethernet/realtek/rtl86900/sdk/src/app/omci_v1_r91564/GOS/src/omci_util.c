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
 * Purpose : Definition of OMCI utilities APIs
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) OMCI utilities APIs
 */

#include "gos_linux.h"
#include "omci_util.h"


int gUsrLogLevel;
int gDrvLogLevel;

void OMCI_TaskDelay(UINT32 num)
{
    UINT32 i = 0 ;

    for (i = 0; i < num / 10; i++)
    {
        usleep(10000);
    }
}

//check input argv is digit or not
GOS_ERROR_CODE omci_util_is_digit(char *inputP)
{
    char *ptr = inputP;

    int match_num, len;
    char accept[] = "1234567890";

    if (ptr)
    {
        len       = strlen(ptr);
        match_num = strspn(ptr, accept);

        if (len == match_num)
            return GOS_OK;
    }
	return GOS_FAIL;
}

GOS_ERROR_CODE omci_util_is_allZero(const unsigned char *p, unsigned int size)
{
	unsigned int i;

	if (!p)
		return GOS_FAIL;

	for (i = 0; i < size; i++)
	{
		if (p[i] != 0)
			return GOS_FAIL;
	}

	return GOS_OK;
}

GOS_ERROR_CODE omci_util_is_all0xFF(const unsigned char *p, unsigned int size)
{
	unsigned int i;

	if (!p)
		return GOS_FAIL;

	for (i = 0; i < size; i++)
	{
		if (p[i] != 0xFF)
			return GOS_FAIL;
	}

	return GOS_OK;
}

static double __log10_subfunction(double cz, int n)
{
	int i;
	double temp2;
	temp2=cz;
	for(i=0;i<n;i++)
		temp2*=cz;
	return temp2/(n+1);
}

#define __log10_pecision	30
double omci_util_log10(double z)
{
	int i,count=0;
	double temp = 0,cz;

	if(z==0.0)
		return -INFINITY;

	if(z==1)
		return 0;

	while(z<1)
	{
		z*=10;
		count--;
	}

	while(z>1)
	{
		z/=10;
		count++;
	}

	cz=(z-1.0)/(z+1.0);

	for(i=1;i<__log10_pecision;i+=2)
		temp+=__log10_subfunction(cz,i-1);

	return (temp*2/M_LN10)+count;
}

char* omci_util_trim_space(char *s)
{
	int     len;
    int     ascii;

	if (!s)
		return NULL;

	// remove trailing spaces
	len = strlen(s);
	while ((--len) > 0 && isspace((ascii = s[len])))
		s[len] = 0;

	// remove leading spaces
	while (isspace((ascii = s[0])))
		s++;

	return s;
}

char* omci_util_space_tokenize(char *s, char **next)
{
	int		len;
	char	*pToken;
	char	*pNext;
	char	endChar[2];
    int     ascii;

	if (!s || !next || 0 == strlen(s))
		return NULL;

	pNext = *next;
	if (NULL == *next)
		pNext = s;
	endChar[1] = '\0';

	// remove leading spaces
	while (isspace((ascii = pNext[0])))
		pNext++;

	// change dilimiter for quotes
	if (pNext[0] == '"' || pNext[0] == '\'')
		endChar[0] = *pNext;
	else
		endChar[0] = ' ';

	// use strtok_r as underlying tokenizer
	if (NULL == *next)
		pToken = strtok_r(pNext, endChar, next);
	else
	{
		*next = pNext;
		pToken = strtok_r(NULL, endChar, next);
	}

	// reinsert the quotes if necessary
	if (pToken && endChar[0] != ' ')
	{
		len = strlen(pToken);
		pToken[len] = endChar[0];
		pToken[len+1] = '\0';
		pToken--;
		*next += sizeof(char);
	}

	return pToken;
}

inline
GOS_ERROR_CODE
omci_util_swap (
    UINT16* varA,
    UINT16* varB )
{
    if (!varA || ! varB)
    {
        return GOS_ERR_PARAM;
    }

    *varA = *varA ^ *varB;
    *varB = *varA ^ *varB;
    *varA = *varA ^ *varB;

     return GOS_OK;
}

