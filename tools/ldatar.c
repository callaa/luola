/*
 * LDAT - Luola Datafile format archiver
 * Copyright (C) 2002-2005 Calle Laakkonen
 *
 * File        : ldatar.c
 * Description : A program to manipulate LDAT archives
 * Author(s)   : Calle Laakkonen
 *
 * LDAT is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Luola is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <stdlib.h>
#include <getopt.h>
#include <stdio.h>

#include "ldat.h"
#include "archive.h"

typedef enum {MODE_UNSET,LIST,PACK,EXTRACT} Mode;
static Mode ldat_mode = MODE_UNSET;
static int verbose = 0, pack_index = 1;

/* Print out list of files in LDAT archive */
static int list_ldat(const char *filename) {
    LDAT *ldat = ldat_open_file(filename);
    if(!ldat) return 1;

    print_ldat_catalog(ldat, verbose);

    ldat_free(ldat);
    return 0;
}

/* Make a new LDAT archive */
static int pack_ldat(const char *filename,int filec, char *filev[]) {
    const char *outputfile;
    LDAT *ldat = ldat_create();

    if(filec==0) {
        /* No list of files specified, assume filename is a .pack file */
        outputfile = pack_ldat_index(ldat,filename, pack_index,verbose);
        if(outputfile==NULL)
            return 1;
    } else {
        /* Pack listed files */
        struct dllist *files=NULL;
        int r;
        for(r=0;r<filec;r++) {
            struct Filename *newfile = make_file(filev[r],filev[r],0);
            if(newfile == NULL)
                return 1;
            if (files)
                dllist_append(files, newfile);
            else
                files = dllist_append(NULL, newfile);
        }
        pack_ldat_files(ldat,files,verbose);
        outputfile = filename;
    }
    if(verbose)
        printf("Saving \"%s\"...\n",outputfile);
    if(ldat_save_file(ldat, outputfile)) {
        fputs("Error occured while saving",stderr);
        ldat_free(ldat);
        return 1;
    }
    if(verbose)
        printf("\tOk.\n");
    ldat_free(ldat);
    return 0;
}

/* Extract files from an LDAT archive */
static int extract_ldat(const char *filename,int filec, char *filev[]) {
    int rval=0;
    if(filec==0) {
        /* No list of files specified, assume filename is a .pack file */
        if(unpack_ldat_index(filename, verbose))
            rval=1;
    } else {
        /* Unpack listed files */
        LDAT *ldat = ldat_open_file(filename);
        if(ldat==NULL) {
            rval=1;
        } else {
            struct dllist *files=NULL;
            int r;
            for(r=0;r<filec;r++) {
                struct Filename file;
                strcpy(file.filename,filev[r]);
                strcpy(file.id,filev[r]);
                file.index = 0;
                if(r+1<filec) {
                    char *endptr=NULL;
                    int tmpi = strtol(filev[r+1],&endptr,10);
                    if(endptr==NULL) {
                        file.index=tmpi;
                        r++;
                    }
                }
                if(unpack_ldat_file(ldat,&file,verbose)) {
                    rval=1;
                    break;
                }
            }
            ldat_free(ldat);
        }
    }

    return rval;
}

static void set_mode(Mode mode) {
    if(ldat_mode!=MODE_UNSET) {
        fputs("Use only one of -l, -p or -x\n",stderr);
        exit(1);
    }
    ldat_mode = mode;
}

int main(int argc, char *argv[]) {
    const char *ldatfile;
    int c=0,rval;
    if(argc==1) {
        puts("Luola Datafile tool");
        puts("Usage: ldat <options> [files...]\nOptions:");
        puts("\t-h, --help                Show this help");
        puts("\t-l, --list <ldat>         List the contents of an archive");
        puts("\t-p, --pack <ldat/pack>    Pack files into an archive");
        puts("\t-x, --extract <ldat/pack> Extract files from an archive");
        puts("\t-I, --noindex             Do not automatically insert index file");
        puts("\t-v, --verbose             Print extra information");
        exit(0);
    }

    while(c!=-1) {
        static struct option options[] = {
            {"list",    required_argument,  0, 'l'},
            {"pack",    required_argument,  0, 'p'},
            {"extract", required_argument,  0, 'x'},
            {"index",   no_argument,        0, 'i'},
            {"noindex",no_argument,        0, 'I'},
            {0,0,0,0}};
        int option_index = 0;

        c = getopt_long(argc,argv,"l:p:x:Iv", options, &option_index);
        switch(c) {
            case 'l': set_mode(LIST); ldatfile=optarg; break;
            case 'p': set_mode(PACK); ldatfile=optarg; break;
            case 'x': set_mode(EXTRACT); ldatfile=optarg; break;
            case 'I': pack_index = 0; break;
            case 'v': verbose = 1; break;
            case '?': exit(1);
        }
    }

    switch(ldat_mode) {
        case MODE_UNSET:
            fputs("No action selected\n",stderr);
            rval=1;
            break;
        case LIST:
            rval=list_ldat(ldatfile);
            break;
        case PACK:
            rval=pack_ldat(ldatfile,argc-optind,argv+optind);
            break;
        case EXTRACT:
            rval=extract_ldat(ldatfile,argc-optind,argv+optind);
            break;
    }

    return rval;
}

