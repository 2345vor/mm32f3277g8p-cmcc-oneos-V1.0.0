/**
 ***********************************************************************************************************************
 * Copyright (c) 2020, China Mobile Communications Group Co.,Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
 * an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations under the License.
 *
 * @file        onepos_gnss_pos.c
 *
 * @brief       nmea0183 protocol parser source file
 *
 * @revision
 * Date         Author          Notes
 * 2020-06-12   OneOS Team      First Version
 ***********************************************************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <os_errno.h>
#include <os_assert.h>
#include <onepos_common.h>
#include "nmea_0183.h"

#define ONEPOS_LOG_TAG "onepos.nmea"
#define ONEPOS_LOG_LVL ONEPOS_LOG_INFO
#include <onepos_log.h>

#ifdef RVCR_USING_NMEA_0183_PROT
static os_bool_t nmea_parse_rmc(nmea_t *nmea_data, const os_uint8_t *sentence);
static os_bool_t nmea_parse_gga(nmea_t *nmea_data, const os_uint8_t *sentence);
static os_bool_t nmea_parse_gsv(nmea_t *nmea_data, const os_uint8_t *sentence);
static os_bool_t nmea_parse_gll(nmea_t *nmea_data, const os_uint8_t *sentence);
static os_bool_t nmea_parse_gsa(nmea_t *nmea_data, const os_uint8_t *sentence);
static os_bool_t nmea_parse_vtg(nmea_t *nmea_data, const os_uint8_t *sentence);
static os_bool_t nmea_parse_zda(nmea_t *nmea_data, const os_uint8_t *sentence);

const nmea_sentence_praser_t nmea_sentence_parser[] = {
    // format :
    // $--RMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,xxxxxx,x.x,a,a*hh<CR><LF>
    // RMC sentence :
    // ?????????????????????????????????????????????????????????????????????/??????????????????????????????????????????/????????????????????????????????????????????????????????????????????????????????????????????????????????????
    {"RMC", "TcfdfdffDfdc", (nmea_parser_func_t)nmea_parse_rmc},
    // format :
    // $--GGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh<CR><LF>
    // GGA sentence
    // ?????????????????????????????????????????????/??????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????M?????????????????????????????????????????????????????????M)???DGPS?????????GPS???????????????????????????DGPS???????????????
    {"GGA", "Tfdfdiiffcfcfi", (nmea_parser_func_t)nmea_parse_gga},
#if NMEA_SUPP_GSV_SATES_INFO
    // format : $--GSV,x,x,xx,xx,xx,xxx,xx,.??????.??????..,xx,xx,xxx,xx*hh<CR><LF>
    // GSV sentence :
    // ??????????????????GSV??????????????????1~9?????????????????????GSV???????????????????????????1~9?????????????????????????????????????????????????????????????????????????????????????????????????????????0??~90????????????????????????????????0??~359????????????????????????????????1~99????????????????????????...??????????????????...??????????????????...
    {"GSV", "iiiiiiiiiiiiiiiiiii", (nmea_parser_func_t)nmea_parse_gsv},
#else
    // format : $--GSV,x,x,xx*hh<CR><LF>
    // GSV sentence :
    // ????????????GSV??????????????????1~9?????????????????????GSV???????????????????????????1~9???????????????????????????????????????
    {"GSV", "iii_", (nmea_parser_func_t)nmea_parse_gsv},
#endif
    // format : $--GLL,llll.ll,a,yyyyy.yy,a,hhmmss.ss,A,a*hh<CR><LF>
    // GLL sentence :
    // ?????????????????????????????????/???????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
    {"GLL", "fdfdTcc", (nmea_parser_func_t)nmea_parse_gll},
    // format :
    // $--GSA,a,x,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,xx,x.x,x.x,x.x*hh<CR><LF>
    // GSA sentence :
    // ???????????????????????????????????????????????????????????????????????????????????????...???????????????????????????????????????????????????????????????????????????????????????????????????
    {"GSA", "ciiiiiiiiiiiiifff", (nmea_parser_func_t)nmea_parse_gsa},
    // format : $--VTG,x.x,T,x.x,M,x.x,N,x.x,K,a*hh<CR><LF>
    // VTG sentence :
    // ??????????????????????????????T:???????????????????????????????????????????????????????????????????????????M:?????????????????????????????????????????????????????????????????????????????????????????????
    // N?????????????????????????????????????????????????????? K???Km|h???
    {"VTG", "fcfcfcfcc", (nmea_parser_func_t)nmea_parse_vtg},
    // format : $--ZDA,hhmmss.ss,xx,xx,xxxx,xx,xx*hh<CR><LF>
    // ZDA sentence :
    // ???????????????(????????????????????????????????????????????????????????????)????????????????????????????????????????????????????????????????????????
    {"ZDA", "Tiiiii", (nmea_parser_func_t)nmea_parse_zda}
    // Additional information
};

/**
 ***********************************************************************************************************************
 * @def         nmea_iseffect
 *
 * @brief       check the char is effect character or not
 *
 * @param       c       char for checked
 ***********************************************************************************************************************
 */
#define nmea_iseffect(c)                                                                                               \
    ((('.' == c) || ('+' == c) || ('-' == c) || (',' == c) || (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||      \
      (c >= 'a' && c <= 'z'))                                                                                          \
         ? OS_TRUE                                                                                                     \
         : OS_FALSE)
/**
 ***********************************************************************************************************************
 * @def         get_sentence_id_str
 *
 * @brief       get the id string of the sentence, like GGA/RMC...
 *
 * @param       sentence       nmea sentence string
 * @param       pstr           buff to save the id string
 ***********************************************************************************************************************
 */
#define get_sentence_id_str(sentence, pstr)                                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        memcpy(pstr, sentence + NMEA_SENTENCE_START_LENGTH, NMEA_SENTENCE_IDS_LENGHT);                                 \
        pstr[NMEA_SENTENCE_IDS_LENGHT] = '\0';                                                                         \
    } while (0)
/**
 ***********************************************************************************************************************
 * @def         jump_sendtence_id_str
 *
 * @brief       skip the lead of the statement : $GPGAA
 *
 * @param       sentence       nmea sentence string
 ***********************************************************************************************************************
 */
#define jump_sendtence_id_str(sentence)                                                                                \
    ((os_uint8_t *)(sentence + NMEA_SENTENCE_IDS_LENGHT + NMEA_SENTENCE_START_LENGTH + 1))

#define nmea_isfield(c) ((nmea_iseffect((os_int8_t)c) && (c != ',') && (c != '*')) ? OS_TRUE : OS_FALSE)

#define next_field(sentence, field)                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        while (nmea_isfield(*sentence))                                                                                \
            sentence++;                                                                                                \
        if (*sentence == ',')                                                                                          \
        {                                                                                                              \
            sentence++;                                                                                                \
            field = sentence;                                                                                          \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            field = NULL;                                                                                              \
        }                                                                                                              \
    } while (0)

#define is_null_field(field) (NULL == field)
#define is_nona_field(c) (',' == c)

/* debug function */
#if defined(ONEPOS_DEBUG)
static void nmea_rmc_display(nmea_t *nmea);
static void nmea_gga_display(nmea_t *nmea);
static void nmea_gsv_display(nmea_t *nmea);
static void nmea_gll_display(nmea_t *nmea);
static void nmea_gsa_display(nmea_t *nmea);
static void nmea_vtg_display(nmea_t *nmea);
static void nmea_zda_display(nmea_t *nmea);
#endif

static os_int32_t hex2int(os_int8_t c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return OS_ERROR;
}

/**
 ***********************************************************************************************************************
 * @brief           check sentence is nmea0183 or not
 *
 * @param[in]       data       name0183 sentence string
 *
 * @return          is right nmea0183 sentence or nots
 * @retval          OS_FALSE       is not
 * @retval          OS_TRUE        is
 ***********************************************************************************************************************
 */

static os_bool_t nmea_check(const os_int8_t *data)
{
    os_int8_t        checksum = 0x00;
    const os_int8_t *sentence = data;

    /* check the product sentence is or not */
    if (*sentence == PRO_SENTEN_START_CHAR)
    {
        return OS_FALSE;
    }

    /* check lenght */
    if ((strlen((const char *)sentence) > NMEA_SENTENCE_CHARS_MAX_LEN) ||
        (strlen((const char *)sentence) < NMEA_SENTENCE_CHARS_MIN_LEN))
        return OS_FALSE;

    /* first character must '$' */
    if (*sentence++ != NMEA_SENTENCE_START_CHAR)
        return OS_FALSE;

    /* calculate all data checksum between '$' and  '*' */
    while (*sentence && *sentence != '*' && nmea_iseffect((os_uint8_t)*sentence))
        checksum ^= *sentence++;

    /* compare checksum */
    if (*sentence == '*')
    {
        sentence++;
        int upper = hex2int(*sentence++);
        if (upper == OS_ERROR)
            return OS_FALSE;
        int lower = hex2int(*sentence++);
        if (lower == OS_ERROR)
            return OS_FALSE;
        int expected = upper << 4 | lower;

        if (checksum != expected)
            return OS_FALSE;
    }

    /* check end character */
    if (*sentence && (NULL == strchr((const char*)sentence, NMEA_SENTENCE_END_CHAR)))
        return OS_FALSE;

    return OS_TRUE;
}
/**
 ***********************************************************************************************************************
 * @brief           get the sentence index in nmea_sentence_parser array
 *
 * @param[in]       sentence       nmea0183 sentence string
 *
 * @return          index number
 ***********************************************************************************************************************
 */
static nmea_sentence_id_t get_sentence_index(const os_int8_t *sentence)
{
    os_uint8_t   sentence_ids[NMEA_SENTENCE_IDS_LENGHT + 1];
    os_uint32_t i = 0;

    get_sentence_id_str(sentence, sentence_ids);

    for (i = 0; i < NMEA_SENTENCE_NUM; i++)
    {
        if (!strcmp((const char *)sentence_ids, (const char *)nmea_sentence_parser[i].sentence_id_str))
        {
            return (nmea_sentence_id_t)i;
        }
    }
    /* unknown */
    if (NMEA_SENTENCE_NUM == i)
        return NMEA_UNKNOWN_TYPE;

    return NMEA_INVILID_TYPE;
}

/**
 ***********************************************************************************************************************
 * @brief           according format to pase the sentence
 *
 * @attention       using right format
 *
 * @return         succ ot fail
 ***********************************************************************************************************************
 */
static os_bool_t nmea_parse(const os_uint8_t *sentence, const os_uint8_t *format, ...)
{
    os_bool_t result = OS_FALSE;
	os_int8_t type   = 0XFF;
    va_list   ap;
    va_start(ap, format);

    const os_uint8_t *field = sentence;

    while (*format)
    {
        type = *format++;

        switch (type)
        {
        /* eg: A M */
        case 'c':
        {
            os_int8_t value = '\0';

            if (field && nmea_isfield(*field))
                value = *field;
            else if (is_nona_field(*field) || is_null_field(field))    // ????????????
                value = ' ';
            else
                goto parse_error;
            *va_arg(ap, os_int8_t *) = value;
        }
        break;
        /* direction???eg: N W E S ; 1 -> W ans S???-1 -> E and N */
        case 'd':
        {
            os_int32_t value = 0;

            if (field && nmea_isfield(*field))
            {
                switch (*field)
                {
                case 'N':
                case 'E':
                    value = -1;
                    break;
                case 'S':
                case 'W':
                    value = 1;
                    break;
                default:
                    goto parse_error;
                }
            }

            *va_arg(ap, os_int32_t *) = value;
        }
        break;
        /* float??? eg: 4717.115 */
        case 'f':
        {
            os_int32_t    sign       = 0;
            os_int64_t    value      = -1;
            os_int32_t    dec_len    = -1;
            loca_float_t *nmea_float = va_arg(ap, loca_float_t *);

            if (field)
            {
                while (field && nmea_isfield(*field))
                {
                    if (sign == 0 && value == -1)
                    {
                        if (_isdigit(field))
                            sign = 1;
                        else if (*field == '+')
                        {
                            sign = 1;
                            field++;
                            continue;
                        }
                        else if (*field == '-')
                        {
                            sign = -1;
                            field++;
                            continue;
                        }
                        else
                        {
                            goto parse_error;
                        }
                    }
                    if (sign != 0 && _isdigit(field))
                    {
                        os_int32_t digit = *field - '0';
                        if (value == -1)
                            value = 0;
                        if (value > (OS_UINT64_MAX - digit) / 10)
                        {    
                            /* over max long int */
                            goto parse_error;
                        }
                        value = (10 * value) + digit;
                        if (0 <= dec_len)
                            dec_len += 1;
                    }
                    else if (*field == '.' && dec_len == -1)
                    {    
                        /* decimals */
                        dec_len = 0;
                    }
                    else if (*field == ' ')
                    {    
                        /* No Spaces allowed */
                        goto parse_error;
                    }
                    else
                    {
                        goto parse_error;
                    }
                    field++;
                }
            }

            /* all data is invalid */
            if (sign == 0 && value == -1 && dec_len == -1)
            {
                value   = 0;
                dec_len = -1;
            }

            /* sign bit */
            if (sign)
                value *= sign;
            nmea_float->value   = value;
            nmea_float->dec_len = dec_len;
        }
        break;

        /* int */
        case 'i':
        {
            os_int32_t value = 0;

            if (field)
            {
                os_int8_t *endptr;
                value = strtol((const char *)field, (char**)&endptr, 10);
                if (field && nmea_isfield(*endptr))
                    goto parse_error;
            }

            *va_arg(ap, os_int32_t *) = value;
        }
        break;
        
        /* string */
        case 's':
        {
            os_int8_t *buf = va_arg(ap, os_int8_t *);

            if (field)
            {
                while (field && nmea_isfield(*field))
                    *buf++ = *field++;
            }

            *buf = '\0';
        }
        break;
        
        /* date eg???200601 */
        case 'D':
        {
            loca_date_t *date = va_arg(ap, loca_date_t *);

            os_int32_t d = -1, m = -1, y = -1;

            if (field && nmea_isfield(*field))
            {
                for (os_int32_t f = 0; f < 6; f++)
                    if (!_isdigit(&field[f]))
                        goto parse_error;

                os_int8_t dArr[] = {field[0], field[1], '\0'};    // day
                os_int8_t mArr[] = {field[2], field[3], '\0'};    // month
                os_int8_t yArr[] = {field[4], field[5], '\0'};    // year
                d           = strtol((const char*)dArr, NULL, 10);
                m           = strtol((const char*)mArr, NULL, 10);
                y           = strtol((const char*)yArr, NULL, 10);
            }

            date->day   = d;
            date->month = m;
            date->year  = y;
        }
        break;
        
        /* time eg:130305.0 */
        case 'T':
        {
            loca_time_t *time_ = va_arg(ap, loca_time_t *);

            os_int32_t h = -1, i = -1, s = -1, us = -1;

            if (field && nmea_isfield(*field))
            {
                for (os_int32_t f = 0; f < 6; f++)
                    if (!_isdigit(&field[f]))
                        goto parse_error;

                os_int8_t iArr[] = {field[2], field[3], '\0'};    // min
                os_int8_t sArr[] = {field[4], field[5], '\0'};    // sec
                os_int8_t hArr[] = {field[0], field[1], '\0'};    // hour
                h           = strtol((const char*)hArr, NULL, 10);
                i           = strtol((const char*)iArr, NULL, 10);
                s           = strtol((const char*)sArr, NULL, 10);
                field += 6;
                /* us */
                if (*field++ == '.')
                {
                    os_uint32_t value = 0;
                    os_uint32_t scale = 1000000LU;
                    while (_isdigit(field) && scale > 1)
                    {
                        value = (value * 10) + (*field++ - '0');
                        scale /= 10;
                    }
                    us = value * scale;
                }
                else
                {
                    us = 0;
                }
            }

            time_->hours        = h;
            time_->minutes      = i;
            time_->seconds      = s;
            time_->microseconds = us;
        }
        break;

        /* ignore */
        case '_':
        {
        }
        break;

        /* jump */
        case ';':
        {
        }
        break;

        default:
        {
            goto parse_error;
        }
        }

        next_field(sentence, field);
    }

    result = OS_TRUE;

parse_error:
    va_end(ap);
    return result;
}

#if NMEA_SUPP_RMC
/*
    ?????????$--RMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,xxxxxx,x.x,a,a*hh<CR><LF>
    RMC??????????????????????????????????????????????????????????????????????????????/??????????????????????????????????????????/????????????????????????????????????????????????????????????????????????????????????????????????????????????
*/
static os_bool_t nmea_parse_rmc(nmea_t *nmea_data, const os_uint8_t *sentence)
{
    os_int8_t   status;
    os_int8_t   mode_indicat;
    os_int32_t  latitude_direction;
    os_int32_t  longitude_direction;
    os_int32_t  variation_direction;
    nmea_rmc_t *rmc_frame = &(nmea_data->rmc_frame);

    if (!nmea_parse(jump_sendtence_id_str(sentence),
                    (const os_uint8_t *)nmea_sentence_parser[NMEA_SENTENCE_RMC].sentence_format,
                    &rmc_frame->time,
                    &status,
                    &rmc_frame->latitude,
                    &latitude_direction,
                    &rmc_frame->longitude,
                    &longitude_direction,
                    &rmc_frame->speed,
                    &rmc_frame->course,
                    &rmc_frame->date,
                    &rmc_frame->variation,
                    &variation_direction,
                    &mode_indicat))
        return OS_FALSE;

    nmea_get_status_num(&status, &rmc_frame->status);
    rmc_frame->latitude.value *= latitude_direction;
    rmc_frame->longitude.value *= longitude_direction;
    rmc_frame->variation.value *= variation_direction;
    nmea_get_mode_num(&mode_indicat, &rmc_frame->mode_indicat);

    nmea_data->valid_flag |= GNSS_RMC_DATA_FLAG;

#if defined(ONEPOS_DEBUG)
    nmea_rmc_display(nmea_data);
#endif

    return OS_TRUE;
}
#endif

#if NMEA_SUPP_GGA
/*
    format???$--GGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh<CR><LF>
    GGA???????????????????????????????????????????????????/?????????????????????????????????????????????????????????????????????????????????????????????????????????M?????????????????????????????????????????????????????????M)???DGPS?????????GPS???????????????????????????DGPS???????????????
*/    
static os_bool_t nmea_parse_gga(nmea_t *nmea_data, const os_uint8_t *sentence)
{
    os_int32_t  latitude_direction;
    os_int32_t  longitude_direction;
    nmea_gga_t *gga_frame = &(nmea_data->gga_frame);

    if (!nmea_parse(jump_sendtence_id_str(sentence),
                    (const os_uint8_t *)nmea_sentence_parser[NMEA_SENTENCE_GGA].sentence_format,
                    &gga_frame->time,
                    &gga_frame->latitude,
                    &latitude_direction,
                    &gga_frame->longitude,
                    &longitude_direction,
                    &gga_frame->quality,
                    &gga_frame->satellites_used,
                    &gga_frame->hdop,
                    &gga_frame->altitude,
                    &gga_frame->altitude_units,
                    &gga_frame->geoidal_separat,
                    &gga_frame->geoidal_separa_units,
                    &gga_frame->dgps_age,
                    &gga_frame->dgps_id))
        return OS_FALSE;

    gga_frame->latitude.value *= latitude_direction;
    gga_frame->longitude.value *= longitude_direction;

    nmea_data->valid_flag |= GNSS_GGA_DATA_FLAG;

#if defined(ONEPOS_DEBUG)
    nmea_gga_display(nmea_data);
#endif

    return OS_TRUE;
}
#endif

#if NMEA_SUPP_GSV
/*
    $GPGSV,2,2,8,01,52,187,43,25,25,074,39,07,37,286,40,04,09,306???33?44???CR??????LF???
*/
static os_bool_t nmea_parse_gsv(nmea_t *nmea_data, const os_uint8_t *sentence)
{
    nmea_gsv_t *gsv_frame = &(nmea_data->gsv_frame);
    nmea_sate_t temp_sates_info[MAX_SATE_NUM_OF_ONE_GSV];

    memset(temp_sates_info, 0, sizeof(nmea_sate_t));

    if (!nmea_parse(jump_sendtence_id_str(sentence),
                    (const os_uint8_t *)nmea_sentence_parser[NMEA_SENTENCE_GSV].sentence_format,
                    &gsv_frame->total_msgs,
                    &gsv_frame->msg_nr,
                    &gsv_frame->total_sats
#if NMEA_SUPP_GSV_SATES_INFO
                    ,
                    &temp_sates_info[0].num,
                    &temp_sates_info[0].elevation,
                    &temp_sates_info[0].azimuth,
                    &temp_sates_info[0].snr,
                    &temp_sates_info[1].num,
                    &temp_sates_info[1].elevation,
                    &temp_sates_info[1].azimuth,
                    &temp_sates_info[1].snr,
                    &temp_sates_info[2].num,
                    &temp_sates_info[2].elevation,
                    &temp_sates_info[2].azimuth,
                    &temp_sates_info[2].snr,
                    &temp_sates_info[3].num,
                    &temp_sates_info[3].elevation,
                    &temp_sates_info[3].azimuth,
                    &temp_sates_info[3].snr
#endif
                    ))
    {
        return OS_FALSE;
    }
    if (0 < gsv_frame->msg_nr && MAX_GSV_NUM >= gsv_frame->msg_nr)
        memcpy((void *)(&gsv_frame->sates_info[(gsv_frame->msg_nr - 1) * MAX_SATE_NUM_OF_ONE_GSV]),
               (void *)temp_sates_info,
               sizeof(nmea_sate_t) * MAX_SATE_NUM_OF_ONE_GSV);
    else
        return OS_FALSE;

    nmea_data->valid_flag |= GNSS_GSV_DATA_FLAG;

#if defined(ONEPOS_DEBUG)
    nmea_gsv_display(nmea_data);
#endif

    return OS_TRUE;
}
#endif

#if NMEA_SUPP_GLL
/*
    $GPGLL,4717.115, N,00833.912, E,130305.0, A?32???CR??????LF???
*/
static os_bool_t nmea_parse_gll(nmea_t *nmea_data, const os_uint8_t *sentence)
{
    os_int8_t        status;
    os_int8_t        mode_indicat;
    os_int32_t         latitude_direction;
    os_int32_t         longitude_direction;

    nmea_gll_t *gll_frame = &(nmea_data->gll_frame);

    if (!nmea_parse(jump_sendtence_id_str(sentence),
                    (const os_uint8_t *)nmea_sentence_parser[NMEA_SENTENCE_GLL].sentence_format,
                    &gll_frame->latitude,
                    &latitude_direction,
                    &gll_frame->longitude,
                    &longitude_direction,
                    &gll_frame->time,
                    &status,
                    &mode_indicat))
        return OS_FALSE;

    gll_frame->latitude.value *= latitude_direction;
    gll_frame->longitude.value *= longitude_direction;
    nmea_get_status_num(&status, &gll_frame->status);
    nmea_get_mode_num(&mode_indicat, &gll_frame->mode_indicat);

    nmea_data->valid_flag |= GNSS_GLL_DATA_FLAG;

#if defined(ONEPOS_DEBUG)
    nmea_gll_display(nmea_data);
#endif

    return OS_TRUE;
}
#endif

#if NMEA_SUPP_GSA
static os_bool_t nmea_parse_gsa(nmea_t *nmea_data, const os_uint8_t *sentence)
{
    nmea_gsa_t *gsa_frame = &(nmea_data->gsa_frame);

    if (!nmea_parse(jump_sendtence_id_str(sentence),
                    (const os_uint8_t *)nmea_sentence_parser[NMEA_SENTENCE_GSA].sentence_format,
                    &gsa_frame->opera_calcu__mode,
                    &gsa_frame->calcu_mode,
                    &gsa_frame->satells_id[0],
                    &gsa_frame->satells_id[1],
                    &gsa_frame->satells_id[2],
                    &gsa_frame->satells_id[3],
                    &gsa_frame->satells_id[4],
                    &gsa_frame->satells_id[5],
                    &gsa_frame->satells_id[6],
                    &gsa_frame->satells_id[7],
                    &gsa_frame->satells_id[8],
                    &gsa_frame->satells_id[9],
                    &gsa_frame->satells_id[10],
                    &gsa_frame->satells_id[11],
                    &gsa_frame->pdop,
                    &gsa_frame->hdop,
                    &gsa_frame->vdop))
        return OS_FALSE;

    nmea_data->valid_flag |= GNSS_GSA_DATA_FLAG;

#if defined(ONEPOS_DEBUG)
    nmea_gsa_display(nmea_data);
#endif

    return OS_TRUE;
}
#endif

#if NMEA_SUPP_VTG
static os_bool_t nmea_parse_vtg(nmea_t *nmea_data, const os_uint8_t *sentence)
{
    os_uint8_t  mode_indicat;
    nmea_vtg_t *vtg_frame = &(nmea_data->vtg_frame);

    if (!nmea_parse(jump_sendtence_id_str(sentence),
                    (const os_uint8_t *)nmea_sentence_parser[NMEA_SENTENCE_VTG].sentence_format,
                    &vtg_frame->course_over_ground_map,
                    &vtg_frame->degree_true,
                    &vtg_frame->course_over_ground_mangnetic,
                    &vtg_frame->degree_magnetic,
                    &vtg_frame->speed_N,
                    &vtg_frame->speed_N_units,
                    &vtg_frame->speed_K,
                    &vtg_frame->speed_K_units,
                    &mode_indicat))
        return OS_FALSE;

    nmea_get_mode_num(&mode_indicat, &vtg_frame->mode_indoct);

    nmea_data->valid_flag |= GNSS_VTG_DATA_FLAG;

#if defined(ONEPOS_DEBUG)
    nmea_vtg_display(nmea_data);
#endif

    return OS_TRUE;
}
#endif

#if NMEA_SUPP_ZDA
static os_bool_t nmea_parse_zda(nmea_t *nmea_data, const os_uint8_t *sentence)
{
    nmea_zda_t *zda_frame = &(nmea_data->zda_frame);
    os_int32_t  month     = -1;
    os_int32_t  day       = -1;
    os_int32_t  year      = -1;

    if (!nmea_parse(jump_sendtence_id_str(sentence),
                    (const os_uint8_t *)nmea_sentence_parser[NMEA_SENTENCE_ZDA].sentence_format,
                    &zda_frame->time,
                    &day,
                    &month,
                    &year,
                    &zda_frame->local_h,
                    &zda_frame->local_m))
        return OS_FALSE;

    zda_frame->date.day   = day;
    zda_frame->date.month = month;
    zda_frame->date.year  = year;

    nmea_data->valid_flag |= GNSS_ZDA_DATA_FLAG;

#if defined(ONEPOS_DEBUG)
    nmea_zda_display(nmea_data);
#endif

    return OS_TRUE;
}
#endif

/**
 ***********************************************************************************************************************
 * @brief           run parser and save data
 *
 * @param[out]      nmea_data       nmea_t type to save date
 * @param[in]       date            nmea sentence string
 *
 * @return          succ or fail
 ***********************************************************************************************************************
 */
os_bool_t nmea(nmea_t *nmea_data, const os_int8_t *data)
{
    const os_int8_t *sentence = data;

    if ((NULL != nmea_data) && (NULL != sentence))
    {
        nmea_sentence_id_t sentence_index = NMEA_INVILID_TYPE;
        ONEPOS_LOG_D("(nmea) sentence : %s", sentence);
        sentence_index = get_sentence_index(sentence);
        if (NMEA_INVILID_TYPE != sentence_index && NMEA_UNKNOWN_TYPE != sentence_index)
        {
            if (nmea_sentence_parser[sentence_index].parse_fun(nmea_data, sentence))
            {
                return OS_TRUE;
            }
            else
            {
                ONEPOS_LOG_E("\nsentence:\n\t%s\nsentence_index: %d prase error.",
                          sentence,
                          sentence_index);
                return OS_FALSE;
            }
        }
        else
        {
            ONEPOS_LOG_E("The nmea protocol sentence : \n\t%s\n not support at this time.", sentence);
            return OS_FALSE;
        }
    }
    else
    {
        ONEPOS_LOG_E("<nmea> param is ERROR!\n");
        return OS_FALSE;
    }
}

#if defined(ONEPOS_DEBUG)

#if NMEA_SUPP_RMC
static void nmea_rmc_display(nmea_t *nmea)
{
    os_int8_t   temp_str[1024];
    os_uint32_t index = 0;
    memset(temp_str, 0, sizeof(temp_str));

    index = os_snprintf((char*)temp_str, 1024, "NMEA RMC DATA:\n");
    index += os_snprintf((char*)(temp_str + index),
                         1024 - index,
                         "\t date : 20%02d - %d - %d\n",
                         nmea->rmc_frame.date.year,
                         nmea->rmc_frame.date.month,
                         nmea->rmc_frame.date.day);
    index += os_snprintf((char*)(temp_str + index),
                         1024 - index,
                         "\t time : %d : %d : %d : %d\n",
                         nmea->rmc_frame.time.hours,
                         nmea->rmc_frame.time.minutes,
                         nmea->rmc_frame.time.seconds,
                         nmea->rmc_frame.time.microseconds);
    index += os_snprintf((char*)(temp_str + index), 1024 - index, "\t status : %d\n", nmea->rmc_frame.status);
    index += os_snprintf((char*)(temp_str + index),
                         1024 - index,
                         "\t latitude : %lld, %d\n",
                         nmea->rmc_frame.latitude.value,
                         nmea->rmc_frame.latitude.dec_len);
    index += os_snprintf((char*)(temp_str + index),
                         1024 - index,
                         "\t longitude : %lld, %d\n",
                         nmea->rmc_frame.longitude.value,
                         nmea->rmc_frame.longitude.dec_len);
    index += os_snprintf((char*)(temp_str + index),
                         1024 - index,
                         "\t speed : %lld, %d\n",
                         nmea->rmc_frame.speed.value,
                         nmea->rmc_frame.speed.dec_len);
    index += os_snprintf((char*)(temp_str + index),
                         1024 - index,
                         "\t course : %lld, %d\n",
                         nmea->rmc_frame.course.value,
                         nmea->rmc_frame.course.dec_len);
    index += os_snprintf((char*)(temp_str + index),
                         1024 - index,
                         "\t variation : %lld, %d\n",
                         nmea->rmc_frame.variation.value,
                         nmea->rmc_frame.variation.dec_len);
    os_snprintf((char*)(temp_str + index), 1024 - index, "\t mode_indicat : %d\n", nmea->rmc_frame.mode_indicat);
    ONEPOS_LOG_D("%s", temp_str);
}
#endif

#if NMEA_SUPP_GGA
static void nmea_gga_display(nmea_t *nmea)
{
    os_int8_t   temp_str[1024];
    os_uint32_t index = 0;
    memset(temp_str, 0, sizeof(temp_str));

    index = os_snprintf((char*)temp_str, 1024, "NMEA GGA DATA:\n");
    index += os_snprintf((char*)(temp_str + index),
                         1024 - index,
                         "\t time : %d : %d : %d : %d\n",
                         nmea->gga_frame.time.hours,
                         nmea->gga_frame.time.minutes,
                         nmea->gga_frame.time.seconds,
                         nmea->gga_frame.time.microseconds);
    index += os_snprintf((char*)(temp_str + index),
                         1024 - index,
                         "\t latitude : %lld, %d\n",
                         nmea->gga_frame.latitude.value,
                         nmea->gga_frame.latitude.dec_len);
    index += os_snprintf((char*)(temp_str + index),
                         1024 - index,
                         "\t longitude : %lld, %d\n",
                         nmea->gga_frame.longitude.value,
                         nmea->gga_frame.longitude.dec_len);
    index += os_snprintf((char*)(temp_str + index), 1024 - index, "\t quality : %d\n", nmea->gga_frame.quality);
    index += os_snprintf((char*)(temp_str + index), 1024 - index, "\t satellites_used : %d\n", nmea->gga_frame.satellites_used);
    index += os_snprintf((char*)(temp_str + index),
                         1024 - index,
                         "\t hdop : %lld, %d\n",
                         nmea->gga_frame.hdop.value,
                         nmea->gga_frame.hdop.dec_len);
    index += os_snprintf((char*)(temp_str + index),
                         1024 - index,
                         "\t altitude : %lld, %d (%c)\n",
                         nmea->gga_frame.altitude.value,
                         nmea->gga_frame.altitude.dec_len,
                         nmea->gga_frame.altitude_units);
    index += os_snprintf((char*)(temp_str + index),
                         1024 - index,
                         "\t geoidal_separat : %lld, %d (%c)\n",
                         nmea->gga_frame.geoidal_separat.value,
                         nmea->gga_frame.geoidal_separat.dec_len,
                         nmea->gga_frame.geoidal_separa_units);
    os_snprintf((char*)(temp_str + index),
                1024 - index,
                "\t dgps_age : %lld, %d\r\n",
                nmea->gga_frame.dgps_age.value,
                nmea->gga_frame.dgps_age.dec_len);
    ONEPOS_LOG_D("%s", temp_str);
}
#endif

#if NMEA_SUPP_GSV
static void nmea_gsv_display(nmea_t *nmea)
{
    os_int8_t   temp_str[1024];
    os_uint32_t index = 0;
    memset(temp_str, 0, sizeof(temp_str));

    index = os_snprintf((char*)temp_str, 1024, "NMEA GSV DATA:\n");
    index += os_snprintf((char*)(temp_str + index), 1024 - index, "\t total_msgs : %d\n", nmea->gsv_frame.total_msgs);
    index += os_snprintf((char*)(temp_str + index), 1024 - index, "\t msg_nr : %d\n", nmea->gsv_frame.msg_nr);
    index += os_snprintf((char*)(temp_str + index), 1024 - index, "\t total_sats : %d\n", nmea->gsv_frame.total_sats);
    index += os_snprintf((char*)(temp_str + index), 1024 - index, "\t statellites info : \n");
    ONEPOS_LOG_D("%s", temp_str);
    memset(temp_str, 0, sizeof(temp_str));
#if NMEA_SUPP_GSV_SATES_INFO
    for (os_uint32_t i = 0; i < nmea->gsv_frame.msg_nr * MAX_SATE_NUM_OF_ONE_GSV; i++)
    {
        index = os_snprintf((char*)temp_str, 1024, "\t\t sate %d num : %d\n", i + 1, nmea->gsv_frame.sates_info[i].num);
        index += os_snprintf((char*)(temp_str + index),
                             1024 - index,
                             "\t\t sate %d snr : %d\n",
                             i + 1,
                             nmea->gsv_frame.sates_info[i].snr);
        index += os_snprintf((char*)(temp_str + index),
                             1024 - index,
                             "\t\t sate %d azimuth : %d\n",
                             i + 1,
                             nmea->gsv_frame.sates_info[i].azimuth);
        index += os_snprintf((char*)(temp_str + index),
                             1024 - index,
                             "\t\t sate %d elevation : %d\n",
                             i + 1,
                             nmea->gsv_frame.sates_info[i].elevation);
        ONEPOS_LOG_D("%s", temp_str);
        memset(temp_str, 0, sizeof(temp_str));
    }
    memset(temp_str, 0, sizeof(temp_str));
    os_snprintf((char*)temp_str, 1024, "\r\n");
#endif
    ONEPOS_LOG_D("%s", temp_str);
}
#endif

#if NMEA_SUPP_GLL
static void nmea_gll_display(nmea_t *nmea)
{
    os_int8_t   temp_str[1024];
    os_uint32_t index = 0;
    memset(temp_str, 0, sizeof(temp_str));

    index = os_snprintf((char*)temp_str, 1024, "NMEA GLL DATA:\n");
    index += os_snprintf((char*)(temp_str + index),
                         1024 - index,
                         "\t time : %d : %d : %d : %d\n",
                         nmea->gll_frame.time.hours,
                         nmea->gll_frame.time.minutes,
                         nmea->gll_frame.time.seconds,
                         nmea->gll_frame.time.microseconds);
    index += os_snprintf((char*)(temp_str + index), 1024 - index, "\t status : %d\n", nmea->gll_frame.status);
    index += os_snprintf((char*)(temp_str + index),
                         1024 - index,
                         "\t latitude : %lld, %d\n",
                         nmea->gll_frame.latitude.value,
                         nmea->gll_frame.latitude.dec_len);
    index += os_snprintf((char*)(temp_str + index),
                         1024 - index,
                         "\t longitude : %lld, %d\n",
                         nmea->gll_frame.longitude.value,
                         nmea->gll_frame.longitude.dec_len);
    os_snprintf((char*)(temp_str + index), 1024 - index, "\t mode_indicat : %d\r\n", nmea->gll_frame.mode_indicat);
    ONEPOS_LOG_D("%s", temp_str);
}
#endif

#if NMEA_SUPP_GSA
static void nmea_gsa_display(nmea_t *nmea)
{
    os_int8_t   temp_str[1024];
    os_uint32_t index = 0;
    memset(temp_str, 0, sizeof(temp_str));

    index = os_snprintf((char*)temp_str, 1024, "NMEA GSA DATA:\n");
    index +=
        os_snprintf((char*)(temp_str + index), 1024 - index, "\t opera_calcu__mode : %c\n", nmea->gsa_frame.opera_calcu__mode);
    index += os_snprintf((char*)(temp_str + index), 1024 - index, "\t calcu_mode : %d\n", nmea->gsa_frame.calcu_mode);
    index += os_snprintf((char*)(temp_str + index),
                         1024 - index,
                         "\t pdop : %lld, %d\n",
                         nmea->gsa_frame.pdop.value,
                         nmea->gsa_frame.pdop.dec_len);
    index += os_snprintf((char*)(temp_str + index),
                         1024 - index,
                         "\t hdop : %lld, %d\n",
                         nmea->gsa_frame.hdop.value,
                         nmea->gsa_frame.hdop.dec_len);
    index += os_snprintf((char*)(temp_str + index),
                         1024 - index,
                         "\t vdop : %lld, %d\n",
                         nmea->gsa_frame.vdop.value,
                         nmea->gsa_frame.vdop.dec_len);

    index += os_snprintf((char*)(temp_str + index), 1024 - index, "\t GPGSA_ID statellites IDs : \n");
    ONEPOS_LOG_D("%s", temp_str);
}
#endif

#if NMEA_SUPP_VTG
static void nmea_vtg_display(nmea_t *nmea)
{
    os_int8_t   temp_str[1024];
    os_uint32_t index = 0;
    memset(temp_str, 0, sizeof(temp_str));

    index = os_snprintf((char*)temp_str, 1024, "NMEA VTG DATA:\n");
    index += os_snprintf((char*)(temp_str + index),
                         1024 - index,
                         "\t course_over_ground_map : %lld, %d (%c)\n",
                         nmea->vtg_frame.course_over_ground_map.value,
                         nmea->vtg_frame.course_over_ground_map.dec_len,
                         nmea->vtg_frame.degree_true);
    index += os_snprintf((char*)(temp_str + index),
                         1024 - index,
                         "\t course_over_ground_mangnetic : %lld, %d (%c)\n",
                         nmea->vtg_frame.course_over_ground_mangnetic.value,
                         nmea->vtg_frame.course_over_ground_mangnetic.dec_len,
                         nmea->vtg_frame.degree_magnetic);
    index += os_snprintf((char*)(temp_str + index),
                         1024 - index,
                         "\t speed_N : %lld, %d (%c)\n",
                         nmea->vtg_frame.speed_N.value,
                         nmea->vtg_frame.speed_N.dec_len,
                         nmea->vtg_frame.speed_N_units);
    index += os_snprintf((char*)(temp_str + index),
                         1024 - index,
                         "\t speed_K : %lld, %d (%c)\n",
                         nmea->vtg_frame.speed_K.value,
                         nmea->vtg_frame.speed_K.dec_len,
                         nmea->vtg_frame.speed_K_units);
    os_snprintf((char*)(temp_str + index), 1024 - index, "\t mode_indicat : %d\r\n", nmea->vtg_frame.mode_indoct);
    ONEPOS_LOG_D("%s", temp_str);
}
#endif

#if NMEA_SUPP_ZDA
static void nmea_zda_display(nmea_t *nmea)
{
    os_int8_t   temp_str[1024];
    os_uint32_t index = 0;
    memset(temp_str, 0, sizeof(temp_str));

    index = os_snprintf((char*)temp_str, 1024, "NMEA ZDA DATA:\n");
    index += os_snprintf((char*)(temp_str + index),
                         1024 - index,
                         "\t date : %d - %d - %d\n",
                         nmea->zda_frame.date.year,
                         nmea->zda_frame.date.month,
                         nmea->zda_frame.date.day);
    index += os_snprintf((char*)(temp_str + index),
                         1024 - index,
                         "\t time : %d : %d : %d : %d\n",
                         nmea->zda_frame.time.hours,
                         nmea->zda_frame.time.minutes,
                         nmea->zda_frame.time.seconds,
                         nmea->zda_frame.time.microseconds);
    index += os_snprintf((char*)(temp_str + index), 1024 - index, "\t local_h : %d\n", nmea->zda_frame.local_h);
    os_snprintf((char*)(temp_str + index), 1024 - index, "\t local_m : %d\n", nmea->zda_frame.local_m);
    ONEPOS_LOG_D("%s", temp_str);
}
#endif

/**
 ***********************************************************************************************************************
 * @def         display_nmea
 * 
 * @param       sentence       display_nmea
 ***********************************************************************************************************************
 */
void display_nmea(nmea_t *nmea)
{
#if NMEA_SUPP_RMC
    nmea_rmc_display(nmea);
#endif

#if NMEA_SUPP_GGA
    nmea_gga_display(nmea);
#endif

#ifdef NMEA_SUPP_GSV
    nmea_gsv_display(nmea);
#endif

#ifdef NMEA_SUPP_GLL
    nmea_gll_display(nmea);
#endif

#ifdef NMEA_SUPP_GSA
    nmea_gsa_display(nmea);
#endif

#ifdef NMEA_SUPP_VTG
    nmea_vtg_display(nmea);
#endif

#ifdef NMEA_SUPP_ZDA
    nmea_zda_display(nmea);
#endif
}
#endif

void display_nmea_pos_result(nmea_t *pos_data)
{
    os_int8_t   temp[1024];

    OS_ASSERT(pos_data != OS_NULL);

    if (GNSS_RMC_DATA_FLAG == (pos_data->valid_flag & GNSS_RMC_DATA_FLAG))
    {
        os_snprintf((char*)temp,
                    sizeof(temp),
                    "$RCVR 20%02d/%02d/%02d %02d:%02d:%02d:%03d %-15.15lld %02d %-15.15lld %02d "
                    "%-15.15lld %02d",
                    pos_data->rmc_frame.date.year,
                    pos_data->rmc_frame.date.month,
                    pos_data->rmc_frame.date.day,
                    pos_data->rmc_frame.time.hours,
                    pos_data->rmc_frame.time.minutes,
                    pos_data->rmc_frame.time.seconds,
                    pos_data->rmc_frame.time.microseconds,
                    pos_data->rmc_frame.latitude.value,
                    pos_data->rmc_frame.latitude.dec_len,
                    pos_data->rmc_frame.longitude.value,
                    pos_data->rmc_frame.longitude.dec_len,
                    pos_data->rmc_frame.speed.value,
                    pos_data->rmc_frame.speed.dec_len);
    }
    else if (GNSS_GGA_DATA_FLAG == (pos_data->valid_flag & GNSS_GGA_DATA_FLAG))
    {
        os_snprintf((char*)temp,
                    sizeof(temp),
                    "$RCVR 20%02d/%02d/%02d %02d:%02d:%02d:%03d %-15.15lld %02d %-15.15lld %02d "
                    "%-15.15lld %02d",
                    pos_data->rmc_frame.date.year,
                    pos_data->rmc_frame.date.month,
                    pos_data->rmc_frame.date.day,
                    pos_data->gga_frame.time.hours,
                    pos_data->gga_frame.time.minutes,
                    pos_data->gga_frame.time.seconds,
                    pos_data->gga_frame.time.microseconds,
                    pos_data->gga_frame.latitude.value,
                    pos_data->gga_frame.latitude.dec_len,
                    pos_data->gga_frame.longitude.value,
                    pos_data->gga_frame.longitude.dec_len,
                    pos_data->rmc_frame.speed.value,
                    pos_data->rmc_frame.speed.dec_len);
    }

    ONEPOS_LOG_I("%s", temp);
}

os_bool_t nmea_prot_parse(const os_int8_t *data, nmea_t* pos_data)
{
    os_bool_t result = OS_TRUE;

    OS_ASSERT(data != OS_NULL);
    OS_ASSERT(pos_data != OS_NULL);

    if (nmea_check(data))
    {
        result = nmea(pos_data, data);
        // if (nmea(pos_data, data))
        // {
        //     result = OS_TRUE;
        // }
        // else
        // {
        //     result = OS_FALSE;
        // }
    }
    else
    {
        ONEPOS_LOG_I("the sentence is not right nmea protocol sentence ");
        result = OS_FALSE;
    }

    return result;
}
#endif
