/*
 * sitedef.h
 * 
 * This file contains site specific definitions.
 * The first three must be set. The last one might need to be changed.
 */

/* Primary Domain Controller, Backup Domain Controller, and NT domain */

#define PRIMARY_DC "my_pdc"
#define BACKUP_DC  "my_bdc"
#define NTDOMAIN   "my_domain"

/* Denied user file */

#define DENYUSERS  "/usr/local/squid/etc/denyusers"
