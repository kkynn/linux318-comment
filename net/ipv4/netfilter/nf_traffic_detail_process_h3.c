#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/netfilter/nf_conntrack_tuple_common.h>
#include <linux/etherdevice.h>

#include <linux/netfilter/nf_traffic_detail_process.h>

const char *RFC2616_METHODS[] =
    { "OPTIONS", "GET", "HEAD", "POST", "PUT", "DELETE", "TRACE",
"CONNECT" };
#define  METHOD_LEN      10
#define  HTTP_VER_LEN     8     // HTTP-Version   = "HTTP" "/" 1*DIGIT "." 1*DIGIT
#define  STATUS_CODE_LEN  3     //The Status-Code element is a 3-digit integer result code
/************************************************************************************
const char *GENERAL_HEADER_FIELDS[] =
    { "Connection", "Date", "Pragma", "Trailer", "Transfer-Encoding",
    "Upgrade",
    "Via", "Warning"
};

const char *ENTITY_HEADER_FIELDS[] =
    { "Allow", "Content-Encoding", "Content-Language", "Content-Length",
    "Content-Location", "Content-MD5", "Content-Range", "Content-Type",
    "Expires", "Last-Modified"
};

const char *REQUEST_HEADER_FIELDS[] =
    { "Accept", "Accept-Charset", "Accept-Encoding", "Accept-Language",
    "Authorization", "Expect", "From", "Host", "If-Match",
    "If-Modified-Since",
    "If-None-Match", "If-Range",
    "If-Unmodified-Since", "Max-Forwards", "Proxy-Authorization", "Range",
    "Referer", "TE", "User-Agent"
};

const char *RESPONSE_HEADER_FIELDS[] =
    { "Accept-Ranges", "Age", "ETag", "Location", "Proxy-Authenticate",
    "Retry-After", "Server", "Vary", "WWW-Authenticate"
};
*****************************************************************************************/

RequestHeader *h3_request_header_new(void)
{
    RequestHeader *h = kmalloc(sizeof(RequestHeader), GFP_ATOMIC);
    if (h) {
        memset(h, 0, sizeof(RequestHeader));
    }
    return h;
}

ResponseHeader *h3_response_header_new(void)
{
    ResponseHeader *h = kmalloc(sizeof(ResponseHeader), GFP_ATOMIC);
    if (h) {
        memset(h, 0, sizeof(ResponseHeader));
    }
    return h;
}

void h3_request_header_free(RequestHeader * header)
{
    kfree(header);
    header = NULL;
}

void h3_response_header_free(ResponseHeader * header)
{
    kfree(header);
    header = NULL;
}

void tf_dump_mem(const char *p, int len)
{
    int i;
    if (p) {
        printk("data:\n");
        for (i = 0; i < len; i++) {
            char c = *(char *) (p + i);
            printk("%c", c);
        }
        printk("\n");
        printk("---------------------------------------------------\n");
    }
}

/**************************************************************/
static int h3_is_request_line(char *body, int bodyLength)
{
    int i;
    int ary_size;
    int method_len;
    char *space;
    if (bodyLength > METHOD_LEN) {
        ary_size = sizeof(RFC2616_METHODS) / sizeof(const char *);
        for (i = 0; i < ary_size; i++) {
            method_len = strlen(RFC2616_METHODS[i]);
            space = (char *) body + method_len;
            if (memcmp(body, RFC2616_METHODS[i], method_len) == 0
                && *(space) == ' ') {
                return 1;
            }
        }
    }

    return 0;

}



/*********************************************************
 * This function returns a char pointer, which is the end of the request line.
 *
 * Return
 *  NULL : if parse failed.
 *  
 **************************************************************************/
char *h3_request_line_parse(RequestHeader * header, char *body,
                            int bodyLength)
{
    // Parse the request-line
    // http://tools.ietf.org/html/rfc2616#section-5.1
    // Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
    char *p = body;
    char *bend = body + bodyLength;

    /* If the Clint SKB data is not started with Known Method, we do nothing */

    if (h3_is_request_line(body, bodyLength) == 0) {
        return NULL;
    }

    header->RequestLineStart = body;

    while (notend(p) && !isspace(*p)) {
        p++;
        if ((p == bend - 1)) {
            return NULL;
        }
    }
    if (end(p) || iscrlf(p)) {
        // set error
        return NULL;
    }


    header->RequestMethod = body;
    header->RequestMethodLen = p - body;

    if (header->RequestMethodLen <= 0) {
        DBG_DYN_PRINT("method is null");
        return NULL;
    }
    // parse RequestURI
    // Skip space
    while (isspace(*p) && notcrlf(p) && notend(p) && (p < (bend - 2))) {
        p++;
    }

    header->RequestURI = p;
    while (!isspace(*p) && notcrlf(p) && notend(p)) {
        if (p >= (bend - 2)) {
            return NULL;
        }
        p++;
    }
    header->RequestURILen = p - header->RequestURI;

    if (header->RequestURILen <= 0) {
        return NULL;
    }

    // Skip space and parse HTTP-Version
    if (iscrlf(p) || end(p)) {
        //HTTPVersion is H3_DEFAULT_HTTP_VERSION;
        return p;
    } else {
        while (isspace(*p) && notcrlf(p)) {
            if (p >= (bend - 2)) {
                return NULL;
            }
            p++;
        }
        header->HTTPVersion = p;
        while (!isspace(*p) && notcrlf(p)) {
            if (p >= (bend - 2)) {
                return NULL;
            }
            p++;
        }
        header->HTTPVersionLen = p - header->HTTPVersion;
        if (header->HTTPVersionLen <= 0) {
            return NULL;
        }
    }

    DBG_DYN_PRINT("h3_request_line_parse ok\n");
    return p;
}

/**
 * Parse header body
 */
int h3_request_header_parse(RequestHeader * header, char *body,
                            int bodyLength)
{

    char *p = h3_request_line_parse(header, body, bodyLength);

    if (p == NULL) {
        return H3_ERR_REQUEST_LINE_PARSE_FAIL;
    }
    // should be ended with CR-LF
    if (end(p))
        return H3_ERR_REQUEST_LINE_PARSE_FAIL;

    if (p >= body + bodyLength - 2) {
        return H3_ERR_INCOMPLETE_HEADER;
    }
    // skip CR-LF
    if (iscrlf(p)) {
        p += 2;
    } else {
        DBG_DYN_PRINT
            ("h3_request_header_parse faile! REQUEST_LINE end without CRLF");
        return H3_ERR_REQUEST_LINE_PARSE_FAIL;
    }


    if (end(p) || (p >= (body + bodyLength)))   //Without other message
        return 0;

    header->Msg_Body = p;

    return 0;
}


#define NOT_FOUND_FIELD  0
#define FOUND_FIELD  1
#define MAX_FIELD_LEN  64
static int h3_check_filed_in_headlist(HeaderField * field, char *headerls)
{
    char filedname_str[MAX_FIELD_LEN];
    char *const delim = ",";
    char *token, *cur;
    char tmp_buf[HEADER_LIST_MAX_LEN + 1];
    if (field->FieldNameLen >= MAX_FIELD_LEN) {
        return NOT_FOUND_FIELD;
    }

    memset(filedname_str, '\0', MAX_FIELD_LEN);
    memcpy(filedname_str, field->FieldName, field->FieldNameLen);

    strncpy(tmp_buf, headerls, sizeof(tmp_buf));
    cur = tmp_buf;

    if (filedname_str[0] == '\0') {
        return NOT_FOUND_FIELD;
    }
    while ((token = strsep(&cur, delim)) != NULL) {
//        DEBUG_PRINT("%s\n", strstrip(token));  
        if (strcmp(filedname_str, strstrip(token)) == 0) {
            return FOUND_FIELD;
        }
    }
//    DBG_DYN_PRINT("Warning! fieldname:[%s] is not headerlist!!\n", filedname_str);
    return NOT_FOUND_FIELD;
}

#define TF_OSGI_DATA_MAX 2048
const char REQUEST_LINE[] = "RequestLine: ";
int h3_required_header_parse_by_headlist(void *task_p, char *headerls,
                                         int rule_dir)
{
    char *osgidata_p = NULL;
    HeaderField field;
    tf_process_task *tf_task = (tf_process_task *) task_p;
    const char *p;
    char *skb_data_end = NULL;
    int line_len = 0;
    char tmp[128];              //for Status Code


    if (tf_task->direction == IP_CT_DIR_ORIGINAL) {
        p = tf_task->request_h->Msg_Body;       //Clent-> Server
    } else {
        p = tf_task->response_h->Msg_Body;      // Server -> Client
    }
    //   DEBUG_PRINT("Msg_Body=%p , headls_do=0x%x, tf_task->data=%p\n", p, headls_do, tf_task->data);

    if (p == NULL) {
        return TF_PROCESS_SKIP;
    }
	
    osgidata_p = kmalloc(TF_OSGI_DATA_MAX, GFP_ATOMIC); // If all packest's is HTTP HEADER

    if (osgidata_p == NULL) {
        if (printk_ratelimit())
            printk("TF_PROCESS allocate mem failed!\n");
        return TF_PROCESS_SKIP;
    }
    memset(osgidata_p, '\0', TF_OSGI_DATA_MAX);

	if(tf_task->osgi_data !=NULL){
		if (printk_ratelimit())
			printk("[%s.%d]BUG: tf_task->osgi_data should is NULL, but it is not't!", __func__, __LINE__);
		kfree(tf_task->osgi_data);
		tf_task->osgi_data = NULL;
	}
	
    tf_task->osgi_data = osgidata_p;

    /******************************************************
	 * HTTP HEADER START-LINE 
       Response: Status-Line
       Request : Request-Line      
     ******************************************************/
    //   if(headls_do & (HTTP_REQUEST_HEADER | HTTP_RESPONSE_HEADER)){
    /* Request header start-line
     * p ---> the data after request-line/ Status-Line
     */
    if (tf_task->direction == IP_CT_DIR_ORIGINAL) {

        memcpy(osgidata_p, REQUEST_LINE, strlen(REQUEST_LINE));
        osgidata_p += strlen(REQUEST_LINE);
        tf_task->osgi_dlen = strlen(REQUEST_LINE);

        /* Request-Line   = "Method SP Request-URI SP HTTP-Version CRLF"  */
        line_len = p - tf_task->data;   //request-line Length
        if ((line_len > (TF_OSGI_DATA_MAX - 1)) || (line_len <= 0)) {
            printk("REQUEST_LINE ERROR!\n");
            return TF_PROCESS_SKIP;
        }
        memcpy(osgidata_p, tf_task->data, line_len);
        //debug
        //tf_dump_mem(osgidata_p, (line_len));

        osgidata_p += line_len;
        tf_task->osgi_dlen += line_len;
#if 0
        //debug: Force not to parse request fileds.
        //return TF_HTTP_PARSER_OK;
#endif

    } else {
        /*     response         */
        /* server ----> Client  */
        sprintf(tmp, "StatusCode: %s\r\n", tf_task->r_status);
        tf_task->osgi_dlen = strlen(tmp);
        memcpy(osgidata_p, tmp, tf_task->osgi_dlen);
        osgidata_p += tf_task->osgi_dlen;
    }


    if ((tf_task->direction != rule_dir) && (rule_dir != 2)) {
        /* Not capture other fields */
        return TF_HTTP_PARSER_OK;
    }
    skb_data_end = tf_task->data + tf_task->data_len;
    // Parse Others Header Fields Here
    if (p >= (skb_data_end - 1)) {
        return TF_HTTP_PARSER_OK;
    }

    do {
        field.FieldName = p;    // start of a header field name

        while (notend(p) && *p != ':') {
            p++;
            if (p >= (skb_data_end - 1)) {
                return TF_HTTP_PARSER_OK;
            }
        }
        field.FieldNameLen = p - field.FieldName;
        p++;                    // skip ':'

        // CRLF is not allowed here
        if (end(p) || iscrlf(p)) {
            return TF_HTTP_PARSER_FAIL; //format error!
        }
        while (notend(p) && isspace(*p)) {
            p++;                // skip space
            if (p >= (skb_data_end - 1)) {
                return TF_HTTP_PARSER_OK;
            }
        }
        // CRLF is not allowed here
        if (end(p) || iscrlf(p))
            return TF_HTTP_PARSER_FAIL; //format error!

        field.Value = p;
        while (notend(p) && notcrlf(p)) {
            p++;
            if (p >= (skb_data_end - 1)) {
                return TF_HTTP_PARSER_OK;
            }
        }
        field.ValueLen = p - field.Value;

        field.Fieldlen = p - field.FieldName;
#if DEBUG > 1
		tf_dump_mem(field.FieldName, field.Fieldlen );
#endif
        /**	 Check the data is desired or not. 	 **/
        if (h3_check_filed_in_headlist(&field, headerls)) {
            if ((tf_task->osgi_dlen + field.Fieldlen + 2) <
                (TF_OSGI_DATA_MAX - 1)) {
                memcpy(osgidata_p, field.FieldName, field.Fieldlen);
                osgidata_p += field.Fieldlen;
                addcrlf(osgidata_p);
                tf_task->osgi_dlen = osgidata_p - tf_task->osgi_data;
            } else {
                printk("[%s]BUFF[TF_OSGI_DATA_MAX] is full!\n", __func__);
            }
        }

        if ((p + 4) >= skb_data_end) {
            return TF_HTTP_PARSER_OK;
        }

        if (iscrlf(p));
        p += 2;

        // end of header: like "\r\n\r\n"
        if (iscrlf(p))
            return TF_HTTP_PARSER_OK;

    } while (notend(p) && p < skb_data_end);

    return TF_HTTP_PARSER_OK;
}

/************************************************
 *  RESPONSE 
 * 
 ***********************************************/

static int h3_check_status_code(const char *statuscode, int len)
{
    int i;


    if (len != STATUS_CODE_LEN) {
        return 0;
    }
    for (i = 0; i < STATUS_CODE_LEN; i++) {
        if ((*(statuscode + i) < 0x30) || (*(statuscode + i) > 0x39)) {
            DBG_DYN_PRINT("[%s.%d] 0x%x\n", __func__, __LINE__,
                          *(statuscode + i));
            return 0;
        }
    }
    return 1;
}


/**
 * This function returns a char pointer, which is the end of the request line.
 *
 * Return NULL if parse failed.
 */
const char *h3_response_line_parse(ResponseHeader * header,
                                   const char *body, int bodyLength)
{
    // Parse the request-line
    // http://tools.ietf.org/html/rfc2616#section-5.1
    // Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
    const char *p = body;
    const char *bend = body + bodyLength;
    header->ResponseLineStart = body;


    while (notend(p) && !isspace(*p)) {
        p++;
        if (p >= (bend - 1)) {
            return NULL;
        }
    }
    if (end(p) || iscrlf(p)) {
        // set error
        return NULL;
    }

    header->HTTPVersion = body;
    header->HTTPVersionLen = p - body;
    /*
       //HTTP-Version   = "HTTP" "/" 1*DIGIT "." 1*DIGIT
     */
    if (header->HTTPVersionLen != HTTP_VER_LEN) {
        return NULL;
    }
    // Skip space
    while (isspace(*p) && notcrlf(p) && notend(p)) {
        p++;
        if (p >= (bend - 1)) {
            return NULL;
        }
    }
    header->ResponseStatus = p;
    while (!isspace(*p) && notcrlf(p) && notend(p)) {
        p++;
        if (p >= (bend - 1)) {
            return NULL;
        }
    }
    header->ResponseStatusLen = p - header->ResponseStatus;
    // The Status-Code element is a 3-digit 
    if (h3_check_status_code
        (header->ResponseStatus, header->ResponseStatusLen) == 0) {
        return NULL;
    }


    if (iscrlf(p) || end(p)) {
        header->ResponseReason = NULL;
    } else {
        while (isspace(*p) && notcrlf(p)) {
            p++;
            if (p >= (bend - 1)) {
                return NULL;
            }
        }
        header->ResponseReason = p;
        while (notcrlf(p)) {
            p++;
            if (p >= (bend - 2)) {
                return NULL;
            }
        }
        header->ResponseReasonLen = p - header->ResponseReason;
    }
    DBG_DYN_PRINT("[%s.%d]h3_response_line_parse ok\n", __func__,
                  __LINE__);
    return p;
}

/**
 * Parse header body
 */
int h3_response_header_parse(ResponseHeader * header, const char *body,
                             int bodyLength)
{
    const char *p = h3_response_line_parse(header, body, bodyLength);

    if (p == NULL) {
        DBG_DYN_PRINT("[%s.%d]\n", __func__, __LINE__);
        return H3_ERR_RESPONSE_LINE_PARSE_FAIL;
    }
    // should be ended with CR-LF
    if (end(p)) {
        DBG_DYN_PRINT("[%s.%d]\n", __func__, __LINE__);
        return H3_ERR_RESPONSE_LINE_PARSE_FAIL;
    }

    if (p + 2 >= body + bodyLength) {
        DBG_DYN_PRINT("[%s.%d]\n", __func__, __LINE__);
        return H3_ERR_RESPONSE_LINE_PARSE_FAIL;
    }
    // skip CR-LF
    if (iscrlf(p)) {
        p += 2;
    } else {
        DBG_DYN_PRINT("[%s.%d]\n", __func__, __LINE__);
        return H3_ERR_RESPONSE_LINE_PARSE_FAIL;
    }

    DBG_DYN_PRINT("[%s.%d] h3_response_header_parse OK \n", __func__,
                  __LINE__);

    if (end(p))
        return 0;

    header->Msg_Body = p;

    return 0;
}
