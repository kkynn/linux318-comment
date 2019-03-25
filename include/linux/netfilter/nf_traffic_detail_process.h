#ifndef __TRAFFICE_DETAIL_PROCESS_H
#define __TRAFFICE_DETAIL_PROCESS_H
#define DEBUG 0

#define TRAFFCIE_PROCESS_RULE_MAX  32   //CONFIG_CMCC_TRAFFIC_PROCESS_RULE_NUM


/* Internal option */


/***** HTTP *******************/

// CRLF string for readibility
#define CRLF "\r\n"
#define MAX_HEADER_SIZE 20

#define iscolon(p) (*p == ':')

#define iscrlf(p) (*p == '\r' && *(p + 1) == '\n')
#define notcrlf(p) (*p != '\r' && *(p + 1) != '\n')

#define notend(p) (*p != '\0')
#define end(p) (*p == '\0')

#define addcrlf(p)           \
     do {                    \
            *p = '\r';       \
            p = p+1;         \
            *p = '\n';       \
            p = p+1;         \
     } while (0)

enum H3_ERROR {
    H3_ERR_HTTP_BODY_PARSE_FAIL = -100,
    H3_ERR_RESPONSE_LINE_PARSE_FAIL ,
    H3_ERR_REQUEST_LINE_PARSE_FAIL,
    H3_ERR_INCOMPLETE_HEADER,
    H3_ERR_UNEXPECTED_CHAR,
};


typedef struct _HeaderField HeaderField;
typedef struct _HeaderFieldList HeaderFieldList;

/*
    Host: github.com
    ^     ^
    |     |
    |     Value (ValueLen = 10)
    |
    | FieldName, FieldNameLen = 4
*/
struct _HeaderField {
    const char *FieldName;
    int         FieldNameLen;
    const char *Value;
    int  ValueLen;
    int Fieldlen;
};

typedef struct  {
    /**
     * Pointer to start of the request line.
     */
    const char * RequestLineStart;
    /**
     * Pointer to the end of the request line 
     */
    const char * RequestLineEnd;
    /**
     * Pointer to the start of the request method string
     */
    const char * RequestMethod;
    int    RequestMethodLen;
    const char * RequestURI;
    int RequestURILen;
    const char * HTTPVersion;
    int HTTPVersionLen;
    const char *Msg_Body;
#if 0
    unsigned int HeaderSize;
    HeaderField Fields[MAX_HEADER_SIZE];
#endif
} RequestHeader;



typedef struct  {
    const char * ResponseLineStart;
    const char * ResponseLineEnd;
    const char * HTTPVersion;
    int HTTPVersionLen;
    const char * ResponseStatus;
    int    ResponseStatusLen;
    const char * ResponseReason;
    int    ResponseReasonLen;
    const char *Msg_Body;
#if 0
    unsigned int HeaderSize;
    HeaderField Fields[MAX_HEADER_SIZE];
#endif
} ResponseHeader;


struct _HeaderFieldList {
    HeaderField ** Fields;
    int cap;
    int len;
};
/**
 * Predefined string and constants
 */
extern int h3_response_header_parse(ResponseHeader *header, const char *body, int bodyLength);
extern int h3_request_header_parse(RequestHeader *header, char *body, int bodyLength);
extern RequestHeader* h3_request_header_new(void);
extern ResponseHeader* h3_response_header_new(void);

void h3_request_header_free(RequestHeader *header);
void h3_response_header_free(ResponseHeader *header);

//int h3_request_header_parse_by_headlist(void *task_p, int headls_flag);
int h3_required_header_parse_by_headlist(void *task_p, char *headls, int rule_dir);


struct store_list_head_t {
	atomic_t n;
	struct list_head list;
};

extern struct store_list_head_t store_list_head;

#define HEADER_LIST_MAX_LEN   128

typedef struct TRAFFCIE_PROCESS_RULE {
  struct list_head list;
  __be32 remoteAddress;
  uint32_t remotePort;
  uint8_t direction;
  unsigned char   h_dest[ETH_ALEN];
  char methodList[64];
  char statusList[HEADER_LIST_MAX_LEN];
  char HTTP_headerlist[HEADER_LIST_MAX_LEN];
  char bundlename[64];
  uint32_t index;
  unsigned long time;
  struct rcu_head rcu;
} traffice_process_rule;


typedef struct TF_PROCESS_TASK {
    struct sk_buff *skb;
    struct iphdr *iph;
    struct tcphdr *tcph;    
    char *data;  //point to SKB Application Data
    int data_len;
    int direction;
    ResponseHeader *response_h;
    RequestHeader *request_h;
    char *osgi_data;
    int osgi_dlen;
    void *ct;
    char r_status[4];
} tf_process_task;

enum conn_dir {
   CONN_DIR_UP = 0,
   CONN_DIR_DOWN,
   CONN_DIR_ALL
};

#define TF_PROCESS_NEED 1
#define TF_PROCESS_SKIP 0
#define TF_HTTP_PARSER_WAIT      2
#define TF_HTTP_PARSER_OK      1
#define TF_HTTP_PARSER_FAIL   -1



#define		HTTP_REQUEST_HEADER		0x1
#define		HTTP_RESPONSE_HEADER		0x2
#define		HTTP_GENERAL_HEADER		0x4
#define		HTTP_ENTITY_HEADER		0x8


/************* NETLINK ***********************************/

typedef struct TRAFFCIE_PROCESS_MSG_H {
  char bundlename[64];
  int osgi_datalen;
} tf_msg_header_T, *tf_msg_header_Tp;



extern int tf_nl_init(void);
extern void tf_nl_exit(void);
extern int tf_nl_send_msg(char *msg, char *msg2);

#if DEBUG
extern int tf_debug_level;
#define DEBUG_PRINT(fmt, ...)   printk("[NF_TF_PROCESS_(%s,%d)]" fmt, __FUNCTION__ ,__LINE__,##__VA_ARGS__)

#define DBG_DYN_PRINT(fmt, ...)                                   \
({                                                              \
                                                                 \
         if (tf_debug_level>=2) {                                    \
                 printk(fmt, ##__VA_ARGS__);                     \
         }                                                       \
})

#define DBG_DYN_CPRINT(fmt, ...)                                   \
({                                                              \
                                                                 \
         if (tf_debug_level>=1) {                                    \
                 printk("\033[1;34m"fmt"\033[0m", ##__VA_ARGS__);      \
         }                                                       \
})

#else
#define DEBUG_PRINT(fmt, ...) 
#define DBG_DYN_PRINT(fmt, ...)
#define DBG_DYN_CPRINT(fmt, ...) 
#endif


#endif
