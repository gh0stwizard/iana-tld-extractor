#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <myhtml/api.h>
#include <idn/api.h>


#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif


static myhtml_tree_t *tree;
static myhtml_t *myhtml;

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


static void
init_idn (idn_resconf_t *ctx)
{
    idn_result_t r;
    char *idnconf[] = { "map tr46-processing-deviation", NULL };


    r = idn_resconf_initialize ();
    assert (r == idn_success);

    r = idn_resconf_create (ctx);
    assert (r == idn_success);

    if (ARRAY_SIZE(idnconf) <= 1)
        return;

    r = idn_resconf_loadstrings (*ctx, idnconf);
    assert (r == idn_success);
}


static void
free_myhtml (void)
{
    if (tree)
        myhtml_tree_destroy(tree);
    if (myhtml)
        myhtml_destroy(myhtml);
}


inline static void
sanitize_text (char *t, size_t length)
{
    for (size_t i = 0; i < length; i++, t++)
        if (*t == '\r' || *t == '\n')
            *t = ' ';
}


static void
parse_tld (myhtml_tree_node_t *parent)
{
    myhtml_tree_node_t *node;
    myhtml_collection_t *td;
    myhtml_tree_t *t;
    char *copy = NULL;
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
            if (len > 0 && text[0] == '\n')
                continue;
            copy = strdup (text);
            sanitize_text (copy, len);
            printf ("\"%.*s\"%s",
                len, copy, (i == td->length - 1) ? "" : ",");
            free (copy);
        }

        myhtml_collection_destroy (txt);
    }

    printf ("\n");

done:
    myhtml_collection_destroy (td);
}


static void
parse_html (myhtml_tree_t *tree)
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
        parse_tld (tr->list[i]);

    /*XXX segfault in mycore_free */
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


extern int
main (int argc, char *argv[])
{
    idn_resconf_t ctx;
    struct res_html res;


    init_myhtml ();
    init_idn (&ctx);

    while (argc-- >= 2) {
        res = load_html_file(argv[argc]);
        myhtml_parse(tree, MyENCODING_UTF_8, res.html, res.size);
        parse_html (tree);
        free (res.html);
    }

    idn_resconf_destroy (ctx);
    free_myhtml ();
    return 0;
}
