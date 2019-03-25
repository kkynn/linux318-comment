/*
 * Copyright(c) Realtek Semiconductor Corporation, 2011
 * All rights reserved.
 *
 * Purpose : Enterprise Switch RTK API mapper table
 *
 * Feature :
 *
 */

#ifndef __DAL_LUNA_G3_MAPPER_H__
#define __DAL_LUNA_G3_MAPPER_H__

/*
 * Include Files
 */
#include <common/rt_type.h>
#include <common/rt_error.h>
#include <dal/dal_mapper.h>

/*
 * Symbol Definition
 */

/*
 * Data Declaration
 */
//extern dal_mapper_t dal_luna_g3_mapper;

/*
 * Macro Declaration
 */

/*
 * Function Declaration
 */


/* Module Name    :  */

/* Function Name:
 *      dal_luna_g3_init
 * Description:
 *      Initilize DAL of enterprise switch
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      RT_ERR_FAILED   - initialize fail
 *      RT_ERR_OK       - initialize success
 * Note:
 *      RTK must call this function before do other kind of action.
 */
extern int32
dal_luna_g3_init(void);


/* Function Name:
 *      dal_luna_g3_mapper_get
 * Description:
 *      Get DAL mapper function
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      dal_mapper_t *     - mapper pointer
 * Note:
 */
extern dal_mapper_t *dal_luna_g3_mapper_get(void);

extern void dal_luna_g3_mapper_register(dal_mapper_t *pMapper);

#endif  /* __DAL_LUNA_G3_MAPPER_H__ */