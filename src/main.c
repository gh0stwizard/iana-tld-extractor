#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>
#include <locale.h>
#ifdef HAVE_CURL
    #include <curl/curl.h>
    #include <time.h>
#endif
#include <myhtml/api.h>
#ifdef HAVE_IDN2
    #include <idn2.h>
#else
    #include <idn/api.h>
#endif
#include "utf8_decode.h"


/* to print app version */
#define _STR(x) #x
#define STR(x) _STR(x)


#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif


static myhtml_tree_t *tree;
static myhtml_t *myhtml;
#ifndef HAVE_IDN2
static idn_resconf_t ctx;
#endif


struct res_html {
    char  *html;
    size_t size;
};


static void
init_myhtml (void)
{
    mystatus_t r;

    myhtml = myhtml_create ();
    assert (myhtml);

    r = myhtml_init (myhtml, MyHTML_OPTIONS_DEFAULT, 1, 0);
    assert (r == MyHTML_STATUS_OK);

    tree = myhtml_tree_create ();
    assert (tree);

    r = myhtml_tree_init (tree, myhtml);
    assert (r == MyHTML_STATUS_OK);
}


#ifndef HAVE_IDN2
static void
init_idn (idn_resconf_t *ctx)
{
    idn_result_t r;


    r = idn_resconf_initialize ();
    assert (r == idn_success);

    r = idn_resconf_create (ctx);
    assert (r == idn_success);

    idn_resconf_setlocalencoding (*ctx, NULL);
    idn_resconf_setlocalcheckfile (*ctx, NULL);
}
#endif


static void
free_myhtml (void)
{
    if (tree)
        myhtml_tree_destroy(tree);

#ifndef __NetBSD__ /* FIXME */
    if (myhtml)
        myhtml_destroy(myhtml);
#endif
}


static const char *
sanitize_text (const char *text, size_t length, int skipdot)
{
#define TEXT_SIZE 2048

    int c1 = 0, c2 = 0; /* characters */
    int p1 = 0, p2 = 0; /* byte position of characters */
    int pos = 0;        /* position in sanitized array */
    static char sanitized[TEXT_SIZE];


/* html data contain some unneccessary characters:
 * 1) such characters as '&lrm;' and '&rlm;' broke encoding to punycode;
 * 2) we don't want any '\r', '\n' characters in the output CSV file.
 */
#define SKIP(c, p, l) do { \
    if ((c) < 0x007f && \
        (((char)c) == '\r' || ((char)c) == '\n' || ((char)c) == '"' )) { \
        /* nop */ \
    } \
    else if (skipdot && ((c) < 0x007f) && ((char)c) == '.') { \
        /* nop */ \
    } \
    else if ((c) == 0x200e || (c) == 0x200f) { \
        /* nop */ \
    } \
    else { \
        assert (pos < TEXT_SIZE); \
        memcpy (sanitized + pos, text + p, l); \
        pos += l; \
    } \
} while (0)


    utf8_decode_init ((char *) text, length);
    /* look forward for characters and their lengths.
     * Such way (may be ugly) helps us avoid creation of utf8_encode() func.
     */
    for (;;) {
        c1 = utf8_decode_next ();
        p1 = utf8_decode_at_byte ();

        if (c1 < 0) {
            if (c2 > 0) { /* it is possible that we miss something */
                /* at p2, length: len - p2 */
                SKIP(c2, p2, length - p2);
            }
            break;
        }

        if (p2 > 0) { /* previous character */
            /* at p2, length: p1 - p2 */
            SKIP(c2, p2, p1 - p2);
        }

        /* look forward */
        c2 = utf8_decode_next ();
        p2 = utf8_decode_at_byte ();

        if (c2 > 0) {
            /* at p1, length: p2 - p1 */
            SKIP(c1, p1, p2 - p1);
        }
        else {
            /* it possible that we read everything; does not work always. */
            /* at p1, length: len - p1 */
            SKIP(c1, p1, length - p1);
        }
    }

    assert (c1 == UTF8_END);
    sanitized[pos] = '\0';

    return sanitized;
#undef SKIP
#undef TEXT_SIZE
}


#ifdef HAVE_IDN2
static const char *
encode_domain (const char *domain, size_t length)
{
    int rc;
    char *result;

    rc = idn2_lookup_ul (domain, &result, 0);
    if (rc != IDN2_OK)
    {
        fprintf (stderr, "%s (%s, %d)\n",
                 idn2_strerror (rc),
                 idn2_strerror_name (rc),
                 rc);
        return NULL;
    }

    return result;
}
#else
static const char *
encode_domain (const char *domain, size_t length)
{
    idn_action_t actions = IDN_ENCODE_REGIST;
    idn_result_t r;
#define DOMAIN_SIZE 1024
    static char result[DOMAIN_SIZE]; /* XXX no critic */


    r = idn_res_encodename (ctx, actions, domain, result, DOMAIN_SIZE - 1);

    if (r != idn_success) {
        fprintf (stderr, "%s: %s\n", domain, idn_result_tostring(r));
        return domain;
    }

    return result;
#undef DOMAIN_SIZE
}
#endif

static void
parse_tld (myhtml_tree_node_t *parent, int raw)
{
    myhtml_tree_node_t *node;
    myhtml_collection_t *td;
    myhtml_tree_t *t;
#define get_tagid(n) myhtml_token_node_tag_id (myhtml_node_token ((n)))

    node = myhtml_node_child (parent);
    /* skip thead */
    if (get_tagid (myhtml_node_next (node)) == MyHTML_TAG_TH)
        return;

    t = myhtml_node_tree (node);
    td = myhtml_get_nodes_by_tag_id_in_scope (t, NULL, parent, MyHTML_TAG_TD, NULL);
    assert (td != NULL && td->list != NULL && td->length > 0);


    /* skip invalid collections */
    if (td->length != 3)
        goto done;

    for (size_t i = 0; i < td->length; i++) {
        myhtml_collection_t *txt = myhtml_get_nodes_by_tag_id_in_scope
            (t, NULL, td->list[i], MyHTML_TAG__TEXT, NULL);

        for (size_t j = 0; j < txt->length; j++) {
            size_t len = 0;
            const char *text = myhtml_node_text (txt->list[j], &len);
            /* skip empty lines */
            if (len > 0 && text[0] == '\n')
                continue;
            if (i == 0) {
                /* domain */
                const char *d = sanitize_text (text, len, 1);

                if (raw == 1) {
                    printf ("\"%s\",", d);
                }
                else {
                    ssize_t dlen = strlen (d);
#ifndef HAVE_IDN2
                    printf ("\"%s\",", encode_domain (d, dlen));
#else
                    const char *puny = encode_domain (d, dlen);
                    if (puny != NULL) {
                        printf ("\"%s\",", puny);
                        free ((void *)puny);
                    }
                    else {
                        /* ascii domain as is */
                        printf ("\"%s\",", d);
                    }
#endif
                }
            }
            else {
                /* type * sponsor */
                const char *copy = sanitize_text (text, len, 0);
                printf ("\"%s\"%s", copy, (i == td->length - 1) ? "" : ",");
            }
        }
        myhtml_collection_destroy (txt);
    }

    printf ("\n");

done:
    myhtml_collection_destroy (td);
}


static void
parse_html (myhtml_tree_t *tree, int raw)
{
    myhtml_collection_t *table, *tbody, *th, *tr;
    myhtml_tree_node_t *node;
    myhtml_tree_t *tbody_tree;


    table = myhtml_get_nodes_by_attribute_value
        (tree, NULL, NULL, true, "id", 2, "tld-table", 9, NULL);
    assert (table != NULL && table->list != NULL && table->length > 0);

    node = myhtml_node_next (myhtml_node_child (table->list[0]));
    assert (node);

    th = myhtml_get_nodes_by_tag_id_in_scope
        (tree, NULL, node, MyHTML_TAG_TH, NULL);
    assert (th != NULL && th->list != NULL && th->length > 0);

    for (size_t i = 0; i < th->length; i++) {
        node = myhtml_node_child (th->list[i]);
        const char *text = myhtml_node_text (node, NULL);
        printf ("\"%s\"%s", text, (i == th->length - 1) ? "" : ",");
    }
    printf ("\n");
    myhtml_collection_destroy (th);

    tbody = myhtml_get_nodes_by_tag_id (tree, table, MyHTML_TAG_BODY, NULL);
    assert (tbody != NULL && tbody->list != NULL && tbody->length > 0);
    tbody_tree = myhtml_node_tree (tbody->list[0]);
    tr = myhtml_get_nodes_by_tag_id (tbody_tree, NULL, MyHTML_TAG_TR, NULL);
    assert (tr != NULL && tr->list != NULL && tr->length > 0);

    for (size_t i = 0; i < tr->length; i++)
        parse_tld (tr->list[i], raw);

    /*XXX segfault in mycore_free (still exists in 4.0.5) */
//    myhtml_collection_destroy (tbody);
//    myhtml_collection_destroy (tr);

    myhtml_collection_destroy (table);
}


static struct res_html
load_html_file (const char* filename)
{
    FILE *fh = fopen(filename, "rb");
    if(fh == NULL) {
        fprintf(stderr, "Can't open html file: %s\n", filename);
        exit(EXIT_FAILURE);
    }

    if(fseek(fh, 0L, SEEK_END) != 0) {
        fprintf(stderr, "Can't set position (fseek) in file: %s\n", filename);
        exit(EXIT_FAILURE);
    }

    long size = ftell(fh);

    if(fseek(fh, 0L, SEEK_SET) != 0) {
        fprintf(stderr, "Can't set position (fseek) in file: %s\n", filename);
        exit(EXIT_FAILURE);
    }

    if(size <= 0) {
        fprintf(stderr, "Can't get file size or file is empty: %s\n", filename);
        exit(EXIT_FAILURE);
    }

    char *html = (char*)malloc(size + 1);
    if(html == NULL) {
        fprintf(stderr, "Can't allocate mem for html file: %s\n", filename);
        exit(EXIT_FAILURE);
    }

    size_t nread = fread(html, 1, size, fh);
    if (nread != size) {
        fprintf(stderr, "could not read %ld bytes (%zu bytes done)\n", size, nread);
        exit(EXIT_FAILURE);
    }

    fclose(fh);

    struct res_html res = { html, (size_t) size };
    return res;
}


#ifdef HAVE_CURL
#define CURL_CHECK(x) do { assert ((x) == CURLE_OK); } while (0)

#ifndef CURL_MAX_RETRIES
#define CURL_MAX_RETRIES 3
#endif

#ifndef CURL_RETRY_SLEEP_SEC
#define CURL_RETRY_SLEEP_SEC 1
#endif

#ifndef ROOT_DB_URL
#define ROOT_DB_URL "https://www.iana.org/domains/root/db"
#endif


static void
init_curl (void)
{
    CURL_CHECK(curl_global_init (CURL_GLOBAL_ALL));
}


static void
free_curl (void)
{
    curl_global_cleanup ();
}


static size_t
curl_write_cb (char *data, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    FILE *fh = (FILE *) userp;


    if (fwrite (data, size, nmemb, fh) == 0) {
        fprintf (stderr, "fwrite: %s\n", strerror(errno));
        return 0;
    }

    return realsize;
}


static int
download (const char *url, const char *outfile)
{
    CURLcode code;
    CURL *curl;
    int retry = 0;
    int max_retries = CURL_MAX_RETRIES;
    struct timespec retry_ts = { CURL_RETRY_SLEEP_SEC, 0 };
    FILE *fh;


    fh = fopen (outfile, "w");
    assert (fh != NULL);

    curl = curl_easy_init ();
    assert (curl != NULL);

    curl_easy_setopt (curl, CURLOPT_URL, url);
    curl_easy_setopt (curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt (curl, CURLOPT_WRITEDATA, (void *) fh);
    curl_easy_setopt (curl, CURLOPT_USERAGENT, "IANA TLD Extractor/1.0");

    do {
        code = curl_easy_perform (curl);

        if (code == CURLE_OK)
            break;

        if (code == CURLE_COULDNT_RESOLVE_HOST ||
            code == CURLE_OPERATION_TIMEDOUT)
        {
            if (retry++ >= max_retries)
                break;

            assert (fseek (fh, 0L, SEEK_SET) != 0);
            (void) nanosleep (&retry_ts, NULL);
        }
        else
        {
            /* unknown error */
            break;
        }
    } while (code != CURLE_OK);

    if (code != CURLE_OK)
        fprintf (stderr, "curl: %s\n", curl_easy_strerror (code));

    curl_easy_cleanup (curl);
    fclose (fh);

    return (code == CURLE_OK);
}
#endif


typedef struct app_options_s {
    int download;
    int printraw;
    const char *html_file;
} app_options_t;


static void
print_usage()
{
    printf ("Usage: iana-tld-extractor [OPTIONS] HTML_FILE\n"
            "Options:\n");
#define p(o, d) printf ("  %-28s %s\n", (o), (d))
    p("--help, -h",         "print this help");
#ifdef HAVE_CURL
    p("--download, -d",     "download from IANA site");
#endif
    p("--raw-domains, -r",  "print raw domains instead of punycode");
    p("--version, -v",      "print version");
#undef p
}


static void
print_version()
{
    const char futures[] = {
#ifdef HAVE_CURL
    " +curl"
#endif
#ifdef HAVE_IDN2
    " +idn2"
#else
    " +idnkit"
#endif
    };
    printf("v%s%s\n", STR(APP_VERSION), futures);
}


static int
parse_args (int argc, char *argv[], app_options_t *result)
{
    int r;
    static struct option opts[] = {
        { "help",        no_argument, 0, 'h' },
#ifdef HAVE_CURL
        { "download",    no_argument, 0, 'd' },
#endif
        { "raw-domains", no_argument, 0, 'r' },
        { "version",     no_argument, 0, 'v' },
        { 0, 0, 0, 0 }
    };

    while (1) {
        int index = 0;
#ifdef HAVE_CURL
        r = getopt_long (argc, argv, "hdrv", opts, &index);
#else
        r = getopt_long (argc, argv, "hrv", opts, &index);
#endif

        if (r == -1)
            break;

        switch (r) {
        case 0:     break;
#ifdef HAVE_CURL
        case 'd':   result->download = 1; break;
#endif
        case 'r':   result->printraw = 1; break;
        case 'h':   print_usage(); return 0;
        case 'v':   print_version(); return 0;
        default:    print_usage(); return 1;
        }
    }

    if (optind >= argc) {
        print_usage();
        return 1;
    }

    result->html_file = argv[optind];

    return -1;
}


extern int
main (int argc, char *argv[])
{
    struct res_html data;
    app_options_t app_opts = { 0, 0, NULL };

    setlocale (LC_ALL, "en_US.UTF-8");

    int parse_args_rv = parse_args(argc, argv, &app_opts);
    if (parse_args_rv != -1)
        return parse_args_rv;

#ifdef HAVE_CURL
    init_curl ();
#endif

    init_myhtml ();

#ifndef HAVE_IDN2
    init_idn (&ctx);
#endif

#ifdef HAVE_CURL
    if (app_opts.download == 1)
        if (! download (ROOT_DB_URL, argv[argc - 1]))
            return 2;
#endif

    data = load_html_file (app_opts.html_file);
    myhtml_parse (tree, MyENCODING_UTF_8, data.html, data.size);
    parse_html (tree, app_opts.printraw);
    free (data.html);

#ifndef HAVE_IDN2
    idn_resconf_destroy (ctx);
#endif

    free_myhtml ();

#ifdef HAVE_CURL
    free_curl ();
#endif

    return 0;
}
