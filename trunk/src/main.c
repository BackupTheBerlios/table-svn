/*
 * Copyright (c) 2008-2009 Christian Oien at gmail dot com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
extern char *optarg;

/* for some reason not been defined */
char *strdup(const char *s);
int getopt(int argc, char * const argv[], const char *optstring);

#include "table.h"

struct tb_fmtspec tb_spec_csv = {
	delim: ';',
	helmetchar: 0,
	hookedchar: '\015'
}; 

int tb_concatenate = 0;
int tb_keep_headers = 0;
int tb_add_headers = 0;
int tb_long_listing = 0;
int tb_sophisticated = 0;
int tb_quiet_mode = 0;
int tb_verbose = 0;
int tb_output_format = 0;
struct tb_fmtspec *tb_output_spec
         = &tb_spec_csv; /* reflects tb_output_format */

/* not parm for now */
int tb_peek_lines_n = 7;

struct tb_infile **tb_infiles = NULL;

struct tb_infile *tb_infile_new(const char *path, int grab_n)
{
	struct tb_infile *ptr = calloc(1, sizeof *ptr);
	if ( ! ptr) return NULL;
	ptr->path = path?strdup(path):NULL;
	ptr->parms.grab_n = grab_n;
	return ptr;
}

char *tb_infile_name(struct tb_infile *infile)
{
	return infile->path?infile->path:"(stdin)";
}

void tb_args(int argc, char **argv)
{
	int n_infiles=0, n_alloced_ptrs, c, grab_n=0;
	struct tb_infile **ptr;
	tb_infiles = malloc((n_alloced_ptrs=2) * sizeof *tb_infiles);
	while (0<(c=getopt(argc, argv, "-cg:hlo:vq"))) {
		switch(c) {
		case 1:
			tb_infiles[n_infiles] = tb_infile_new(optarg, grab_n);
			ptr = realloc(tb_infiles,
				(++n_alloced_ptrs) * sizeof *tb_infiles);
			if (!ptr || !tb_infiles[n_infiles]) {
				fprintf(stderr, "realloc failed\n");
				exit(2);
			}
			++n_infiles;
			tb_infiles = ptr;
			grab_n = 0;
		break;
		case 'c':
			++tb_concatenate;
		break;
		case 'g':
			grab_n = atoi(optarg);
		break;
		case 'h':
			++tb_keep_headers;
		break;
		case 'H':
			++tb_add_headers;
		break;
		case 'l':
			++tb_long_listing;
		break;
		case 'o':
			tb_output_format = optarg[0];
		break;
		case 'v':
			++tb_verbose;
		break;
		case 'q':
			++tb_quiet_mode;
		break;
		case 's':
			++tb_sophisticated;
		break;
		default:
			fprintf(stderr, "unknown option '%c'\n", c);
		case '?':
			exit(0);
		}
	}
	if (n_infiles == 0 || grab_n > 0) {
		tb_infiles[n_infiles++] = tb_infile_new(NULL, grab_n);
	}
	tb_infiles[n_infiles] = NULL;
	return;
}

void tb_dumpparms(void)
{
	struct tb_infile **ptr;
	fprintf(stderr, "concatenate:\t%s\n", tb_concatenate?"yes":"no");
	fprintf(stderr, "keep-headers:\t%s\n", tb_keep_headers?"yes":"no");
	fprintf(stderr, "add-headers:\t%s\n", tb_add_headers?"yes":"no");
	fprintf(stderr, "output-format:\t%c\n",
		tb_output_format?tb_output_format:'-');
	fprintf(stderr, "long-listing:\t%s\n", tb_long_listing?"yes":"no");
	fprintf(stderr, "sophisticated:\t%s\n", tb_sophisticated?"yes":"no");
	fprintf(stderr, "quiet-mode:\t%s\n", tb_quiet_mode?"yes":"no");
	fprintf(stderr, "verbose:\t%d\n", tb_verbose);
	for (ptr=tb_infiles; *ptr; ptr++) {
		fprintf(stderr,
			"%s\t%d\n", tb_infile_name(*ptr),
			(*ptr)->parms.grab_n);
	}
}

void tb_files(void)
{
	struct tb_infile **ptr;
	for (ptr=tb_infiles; *ptr; ptr++) {
		if ((*ptr)->path && 0==strcmp("-", (*ptr)->path)) {
			free((*ptr)->path);
			(*ptr)->path = NULL;
		}
		if (NULL==(*ptr)->path) {
			(*ptr)->f = stdin;
		} else if (!((*ptr)->f = fopen((*ptr)->path, "rb"))) {
			fprintf(stderr, "could not open '%s'\n",
				(*ptr)->path);
			exit(1);
		}
	}
}

static int tb_parse_fields(struct tb_fmtspec *spec, char *line, char ***fields)
{
#define TB_PARSE_PTRBUF_LEN 32
	static char *ptrbuf[TB_PARSE_PTRBUF_LEN];
	int i=0;
	char *ptr = line;
	if (tb_verbose>2) fprintf(stderr, "parsing %s:", line);
	if (line[0] == spec->helmetchar) ++ptr;
	(*fields) = ptrbuf;
	ptrbuf[i++] = ptr;
	do {
		if (tb_verbose>2) fprintf(stderr, "%c.", ptr[0]);
		ptr = strchr(ptr, spec->delim);
		if ( ! ptr) break;
		*ptr = '\0';
		ptrbuf[i++] = ++ptr;
	} while (i < TB_PARSE_PTRBUF_LEN);
	if (tb_verbose>2)
		fprintf(stderr, " %d/%d fields\n", i, spec->n_fields);
	if (ptr || i!=spec->n_fields) return 1;
	ptr = strchr(ptrbuf[i-1], '\n');
	if (ptr) {
		*ptr = '\0';
		if (ptr[-1] == spec->hookedchar) ptr[-1] = '\0';
	}
	return 0;
}

int tb_check_fields()
{
	/* todo: for each field check if its common
	 *       otherwise augment corresponding counter.
	 *       return non-zero if some fields uncommon. */
	return 0;
}

void tb_common_field_formats(struct tb_infile *infile)
{
	infile->fields = calloc(infile->spec.n_fields,
			sizeof infile->fields[0]);
	/* todo: use the first lines to decide what format
	 *       is most common to each field.  use and populate
	 *       the field_info structure */
}

void tb_analyze_first_lines(struct tb_infile *infile)
{
	int no_helmetchar=0, no_hookedchar=0;
	int i,j,k,n;
	int delim_hits[TB_PREF_DELIM_N];
	int prev_delim_hits[TB_PREF_DELIM_N];
	int hits_changes[TB_PREF_DELIM_N];
	char *ptr;
	memset(hits_changes, 0, TB_PREF_DELIM_N * sizeof hits_changes[0]);
	for (i=0; i<tb_peek_lines_n; i++) {
		if (infile->first_lines[i] == NULL) {
			if (i>1) break;
			fprintf(stderr, "analyze of %s did not succeed\n",
					tb_infile_name(infile));
			return;
		}
		if (tb_verbose>1) fprintf(stderr, "line %d:", i+1);
		if ( ! no_helmetchar) {
			if ( ! infile->spec.helmetchar) {
				if (infile->first_lines[i][0] == '\015')
					infile->spec.helmetchar = '\015';
			} else if (infile->first_lines[i][0]
					!= infile->spec.helmetchar) {
				if (i != 0) {
					infile->spec.helmetchar = 0;
					no_helmetchar = 1;
				}
			}
		}
		ptr = infile->first_lines[i];
		memset(delim_hits, 0, sizeof delim_hits);
		while (*ptr) {
			n = strcspn(ptr, TB_PREF_DELIM);
			ptr += n;
			switch (*ptr) {
			case TB_PREF_DELIM_a: ++delim_hits[0]; ++ptr; break;
			case TB_PREF_DELIM_b: ++delim_hits[1]; ++ptr; break;
			case TB_PREF_DELIM_c: ++delim_hits[2]; ++ptr; break;
			case TB_PREF_DELIM_d: ++delim_hits[3]; ++ptr; break;
			case TB_PREF_DELIM_e: ++delim_hits[4]; ++ptr; break;
			case TB_PREF_DELIM_f: ++delim_hits[5]; ++ptr; break;
			case TB_PREF_DELIM_g: ++delim_hits[6]; ++ptr; break;
			case '\0': continue;
			default: assert(0);
			}
			if (tb_verbose>1)
				fprintf(stderr, " hit '%c'.", ptr[-1]);
		}
		if (i>0) {
			for (k=0; k<TB_PREF_DELIM_N; k++) {
				if (delim_hits[k] != prev_delim_hits[k]) {
					if (tb_verbose>1)
						fprintf(stderr,
						" '%c' %d-->%d\n", ptr[-1],
						prev_delim_hits[k],
						delim_hits[k]);
					++hits_changes[k];
				}
			}
		}
		memcpy(prev_delim_hits, delim_hits, sizeof delim_hits);
		/* todo: search for eventual hookedchar */
		if (tb_verbose>1) putc('\n', stderr);
	}
	for (k=0; k<TB_PREF_DELIM_N; k++) {
		if (delim_hits[k]>0 && 0==hits_changes[k]) {
			infile->spec.delim = TB_PREF_DELIM[k];
			infile->spec.n_fields = delim_hits[k]+1;
			break;
		}
	}
	if (infile->spec.delim) {
		if (tb_concatenate && tb_verbose>1
				|| tb_verbose>2)
			fprintf(stderr,
			"delimiter '%c' for '%s'\n",
			infile->spec.delim=='\t'?'t':infile->spec.delim,
			tb_infile_name(infile));
	} else if (tb_verbose>1) {
		fprintf(stderr, "delimiter not found for '%s'\n",
			tb_infile_name(infile));
	}
	tb_common_field_formats(infile);
}

void tb_analyze()
{
	char lbuf[512];
	struct tb_infile **ptr;
	int i;
	for (ptr=tb_infiles; *ptr; ptr++) {
		assert((*ptr)->f != NULL);
		(*ptr)->first_lines = calloc(tb_peek_lines_n+1,
				sizeof *(*ptr)->first_lines);
		i=0;
		while(fgets(lbuf, sizeof lbuf, (*ptr)->f)
				&& i<tb_peek_lines_n) {
			(*ptr)->first_lines[i++] = strdup(lbuf);
		}
		tb_analyze_first_lines(*ptr);
	}
}

void tb_infiles_reset()
{
	struct tb_infile **ptr;
	for (ptr=tb_infiles; *ptr; ptr++) {
		assert((*ptr)->f != NULL);
		if ((*ptr)->path) { /* not stdin */
			clearerr((*ptr)->f);
			fseek((*ptr)->f, 0L, SEEK_SET);
		}
	}
}

void tb_totalscan()
{
	int i;
	char lbuf[512];
	struct tb_infile **ptr;
	int linenum = 0;
	char **fields;
	tb_infiles_reset();
	for (ptr=tb_infiles; *ptr; ptr++) {
		if ( ! (*ptr)->path) { /* stdin */
		for (i=0; (*ptr)->first_lines[i]; i++) {
			if (tb_parse_fields(&(*ptr)->spec,
					(*ptr)->first_lines[i], &fields)) {
				if (tb_verbose>2) fprintf(stderr,
					"%s:%d fields parse failed",
						tb_infile_name(*ptr));
				continue;
			}
			if (tb_check_fields(&(*ptr), fields)) {
				++(*ptr)->n_uncommon_rows;
			}
		}
		continue;
		} /* non-stdin .. */
		while (fgets(lbuf, sizeof lbuf, (*ptr)->f)) {
			if (tb_parse_fields(&(*ptr)->spec,
					lbuf, &fields)) {
				if (tb_verbose>2) fprintf(stderr,
					"%s:%d fields parse failed",
						tb_infile_name(*ptr));
				continue;
			}
			if (tb_check_fields(&(*ptr), fields)) {
				++(*ptr)->n_uncommon_rows;
			}
		}
		++(*ptr)->n_lines;
	}
}

void tb_list()
{
	int i;
	struct tb_infile **ptr;
	for (ptr=tb_infiles; *ptr; ptr++) {
		fprintf(stderr, "%s: `%d`%c`%d",
			tb_infile_name(*ptr),
			(*ptr)->spec.helmetchar,
			(*ptr)->spec.delim=='\t'?'t':(*ptr)->spec.delim,
			(*ptr)->spec.hookedchar);
		if (tb_long_listing) {
			printf(" %d ", (*ptr)->n_uncommon_rows);
			for (i=0; i<(*ptr)->spec.n_fields; i++) {
				printf("%d ",
				(*ptr)->fields[i].uncommon_count);
			}
		}
		putc('\n', stdout);
	}
}

static void tb_empty_fields(struct tb_fmtspec *spec, int n_fields)
{
	int i;
	for (i=1; i<n_fields; i++) {
		putc(spec->delim, stdout);
	}
}

int tb_paste()
{
	char lbuf[512];
	struct tb_infile **ptr;
	int i, n_not_eof = 1, linenum = 1,
		got_stdin_from_mem, first_lines_done = 0;
	char **fields;
	tb_infiles_reset();
	while (n_not_eof>0) {
		n_not_eof = 0;
		got_stdin_from_mem = 0;
		if (linenum != 1 && tb_output_spec->helmetchar)
			putc(tb_output_spec->helmetchar, stdout);
		for (ptr=tb_infiles; *ptr; ptr++) {
			if (ptr!=tb_infiles) {
				putc(tb_output_spec->delim, stdout);
			}
			if (!(*ptr)->path && !first_lines_done) {
				if ((*ptr)->first_lines[linenum-1]) {
					strncpy(lbuf,
						(*ptr)->first_lines[linenum-1],
						sizeof lbuf);
					++n_not_eof;
					got_stdin_from_mem = 1;
				} else first_lines_done = 1;
			}
			if (!got_stdin_from_mem
			&& (feof((*ptr)->f)
				|| !fgets(lbuf, sizeof lbuf, (*ptr)->f))) {
				if (tb_verbose>2)
					fprintf(stderr, "%s at eof\n",
						tb_infile_name(*ptr));
				tb_empty_fields(tb_output_spec,
					(*ptr)->spec.n_fields);
				continue;
			}
			++n_not_eof;
			if (tb_parse_fields(&(*ptr)->spec,
					lbuf, &fields)) {
				if (tb_verbose>2) fprintf(stderr,
					"%s:%d fields parse failed",
						tb_infile_name(*ptr));
				tb_empty_fields(tb_output_spec,
					(*ptr)->spec.n_fields);
				continue;
			}
			if (strchr(fields[0], ';')&&'"'!=fields[0][0])
				printf("\"%s\"", fields[0]);
			else printf("%s", fields[0]);
			/* todo: instead of (in desperation of protecting the
			 * ';') using "%s" as format, i should use
			 * it only if that is the format of
			 * the corresponding input field.  also, '"'
			 * characters in the string must be written as '""'
			 * according to rfc 4180 (on that output format).
			 * should have a field-reformating function that
			 * takes FROM and TO format, doing interpretation
			 * and generation of escapes and so. */
			for (i=1; i<(*ptr)->spec.n_fields; i++) {
				if (strchr(fields[i], ';')&&'"'!=fields[i][0])
					printf("%c\"%s\"",
						tb_output_spec->delim,
						fields[i]);
				else printf("%c%s", tb_output_spec->delim,
					fields[i]);
			}
		}
		if (tb_output_spec->hookedchar)
			putc(tb_output_spec->hookedchar, stdout);
		putc('\n', stdout);
		++linenum;
	}
	return 0;
}

void tb_report_by_exit()
{
	/* todo: give count of non-consistent inputfiles */
	exit(1);
}

int main(int argc, char **argv)
{
	tb_args(argc, argv);
	if (tb_verbose) tb_dumpparms();
	tb_files();
	tb_analyze();
	if (tb_long_listing)
		tb_totalscan();
	if (tb_concatenate) {
		tb_paste();
		return 0;
	}
	if (tb_quiet_mode) tb_report_by_exit();
	tb_list();
	return 0;
}

