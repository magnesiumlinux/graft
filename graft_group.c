/*
 * graft_group
 *
 * examine a new fragment in /etc/group format
 * ensure all group names/ids are unique or match orig,
 * that there are no supergroups
 * and all group members exist in /etc/passwd
 *
 * write merged lines to stdout 
 */

#define _GNU_SOURCE /* for fgetgrent */           
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAMESZ 8
#define MEMB_MAX 16

#define GROUP_MIN 1

#define USAGE() (fprintf(stderr, "graft group original_file new_file\n"), exit(1))
#define DIE(fmt, ...) (fprintf(stderr, fmt "\n", ##__VA_ARGS__),exit(2))
#define DIES(msg) (fprintf(stderr, "error %s: %s\n", msg, strerror(errno)), exit(2))

/**
 ** Linked list of entries to write
 **/

struct grplist {
  char gr_name[NAMESZ];
  /*password is always empty*/
  gid_t gr_gid;
  char gr_mem[MEMB_MAX][NAMESZ];
  unsigned char nmemb;
  struct grplist *prev;
  struct grplist *next;
};

struct grplist *grpadd(struct grplist *all, struct group *grent) {
  struct grplist *this;

  this = malloc(sizeof(struct grplist));
  if ( ! this ) DIES("malloc");

  /* deep copy fields */
  strcpy(this->gr_name, grent->gr_name);
  this->gr_gid = grent->gr_gid;
  for (this->nmemb=0; grent->gr_mem[this->nmemb]; this->nmemb++) {
    if ( this->nmemb >= MEMB_MAX ) DIE("too many group members");
    strcpy(this->gr_mem[this->nmemb], grent->gr_mem[this->nmemb]);
  }

  /* append */
  if ( ! all ) {
    all = this;
    all->next = NULL;
    all->prev = NULL;
  } else {
    this->next = all;
    all->prev = this;
    all = this;
  }

  return all;
}

void grpwrite(FILE *fp, struct grplist *all) {
  int i;
  
  while ( all ) {
    fprintf(fp, "%s:x:%d:", all->gr_name, all->gr_gid);
    for (i=0; i<all->nmemb-1; i++) {
      fprintf(fp, "%s,", all->gr_mem[i]);
    }
    if ( all->nmemb ) {
      fprintf(fp, "%s\n", all->gr_mem[all->nmemb-1]);
    } else {
      fprintf(fp, "\n");
    }
  
    all = all->next;
  }
    
}


void group_merge(struct grplist *dst, struct group *src) {
  /* go through groups, adding missing members from src to dst */
  int i, j, done=0;
  char *needle;
  
  for (i=0; src->gr_mem[i]; i++) {
    needle = src->gr_mem[i];
    for (j=0; j<dst->nmemb; j++) {
      if ( ! strcmp(needle, dst->gr_mem[j]) ) {
        done = 1;
      }
    }

    if ( ! done ) {
      strcpy(dst->gr_mem[dst->nmemb], needle);
      dst->nmemb++;
    }
  }
}


void group_check(struct group *grent) {
  /* ensure that there are no supergroups
   * and all group members exist in /etc/passwd
   */
  int i;

  if ( grent->gr_gid < GROUP_MIN ) DIE("supergroup: %d", grent->gr_gid);
  if ( strcmp(grent->gr_passwd, "x") ) DIE("nonempty password: %s", grent->gr_passwd);
  for (i=0; grent->gr_mem[i] ; i++ ){
    if ( ! getpwnam(grent->gr_mem[i]) ) DIE("no user: %s", grent->gr_mem[i]); 
  }
}


void graft_group(FILE *orig, FILE *new) {
  struct group *grent;
  struct grplist *this, *all=NULL;
  char namecmp;
  int done;
  /* check and add all new groups to the list */
  while ( (grent=fgetgrent(new)) != NULL ) {
    group_check(grent);
    all = grpadd(all, grent);
  }

  /* look through existing groups and modify or add them */
  while ( (grent=fgetgrent(orig)) != NULL ) {
    this = all;
    done = 0;
    while ( this && !done ) {
      namecmp = strcmp(this->gr_name, grent->gr_name);
      
      /* ensure name,id either both match, or both mismatch */
      if ( ( (!namecmp) && this->gr_gid != grent->gr_gid ) 
           || ( namecmp && this->gr_gid == grent->gr_gid) ) DIE("group name/id mismatch");

      if ( ! namecmp ) {
        /* this refers to an existing group name and id: merge */
        group_merge(this, grent);
        done = 1;
      } else {
        this = this->next;
      }
    }

    /* otherwise, grent isn't in the list: add it */
    if ( ! this ) {
      all = grpadd(all, grent);
    }
  }

  grpwrite(stdout, all);
}


int main(int argc, char **argv) {
  FILE *orig, *new;

  if ( argc < 3 ) USAGE();

  if ( (orig=fopen(argv[1], "r")) == NULL ) DIES("opening orig");
  if ( (new=fopen(argv[2], "r")) == NULL ) DIES("opening new");
  
  graft_group(orig, new);
}
