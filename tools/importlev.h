#ifndef IMPORTLEV_H
#define IMPORTLEV_H

/* Report from save_level */
struct Files {
    int failed;
    const char *artwork;
    const char *collmap;
    const char *cfgfile;
    const char *thumbnail;
};

/* Importer module registration function should return */
/* this structure. */
struct Importer {
	/* Name of this level format */
	const char *name;

    /* Level aspect ratio */
    float aspect;
    
	/* Check if the given file is in this format */
	int (*check_format)(FILE *fp);

	/* Load level to memory */
	int (*load_level)(FILE *fp);

	/* Free level data */
	void (*unload_level)(void);

	/* Save level in Luola format */
	struct Files (*save_level)(const char *basename, int lcmap);
};

#endif

