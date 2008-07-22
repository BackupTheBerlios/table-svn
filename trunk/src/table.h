struct tb_infparms {
    int grab_n;
};

struct tb_fmtspec {
    int delim;
    int pad; /* char (SPC only) or <0 (meaning none) */
    int helmetchar; /* first on line except on first line */
    int hookedchar; /* always last on line */
    int n_fields;
    int n_rows;
    int n_head;
    int n_tail;
};

struct tb_fldinfo {
    char **alts;
    int *alt_counts;
    int n_alts;
    int uncommon_count;
};

struct tb_infile {
    char *path; /* stdin represented by nil  */
    FILE *f; /* nil means not currently opened */
    struct tb_infparms parms;
    struct tb_fmtspec spec;
    struct tb_fldinfo *fields;
    char *header_line;
    char **first_lines;
    char *tail_line;
    /* following only populated when "long-listing" mode */
    int n_lines;
    int n_uncommon_rows;
};

struct tb_outfile {
    struct tb_fmtspec spec;
    char **formats; /* a format nil means dont format */
};

#define TB_PREF_DELIM "\t;|#, ="
#define TB_PREF_DELIM_a '\t'
#define TB_PREF_DELIM_b ';'
#define TB_PREF_DELIM_c '|'
#define TB_PREF_DELIM_d '#'
#define TB_PREF_DELIM_e ','
#define TB_PREF_DELIM_f ' '
#define TB_PREF_DELIM_g '='
#define TB_PREF_DELIM_N 7

