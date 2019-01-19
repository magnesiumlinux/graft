/*
 * graft_passwd.c
 * examine a new fragment in /etc/passwd format
 * ensure no entries match an existing entry (id or name) in orig
 * and no superusers or supergroups are created
 * and that: name~[a-z][0-9], passwd == "x", home starts with "/home/"
 * 
 * on success, write the combined files to stdout
 * on error, exit nonzero and write nothing to stdout
 * do not write partial files
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define _GNU_SOURCE /* for fgetpwent */
#include <pwd.h>

#define UID_MIN (uid_t)1
#define GID_MIN (gid_t)1
#define NAMESZ  8
#define HOME    "/home/"
#define EXE     "graft_passwd"

#define USAGE() (fprintf(stderr, "Usage: " EXE " original_file new_file\n"), exit(1))
#define DIE(fmt, ...) (fprintf(stderr, fmt "\n", ## __VA_ARGS__), exit(2))
#define DIES(msg) (fprintf(stderr, "error %s: %s\n", msg, strerror(errno)), exit(2))

void cat(FILE *fp) {
  /* copy contents of fp to stdout */
  unsigned char buf[512];
  unsigned len;
  while ( (len=fread(buf, 1, sizeof(buf), fp)) >0 ) {
    if ( fwrite(buf, 1, len, stdout) < len ) DIES("writing stdout");
  }
  if ( ferror(fp) )  DIES("reading new");
}

struct passwd *fgetpwuid(FILE *fp, uid_t uid){
  struct passwd *cur;
  fseek(fp, 0, SEEK_SET);
  while ( (cur=fgetpwent(fp)) != NULL ) {
    if ( cur->pw_uid == uid)  return cur;
  } 
  return NULL;
}

struct passwd *fgetpwnam(FILE *fp, const char *name) {
  struct passwd *cur;
  fseek(fp, 0, SEEK_SET);
  while ( (cur=fgetpwent(fp)) != NULL ) {
    if ( ! strcmp(cur->pw_name, name) ) return cur;
  } 
  return NULL;
}

void graft_passwd(FILE *orig, FILE *new) {
  struct passwd *pwent;
  uid_t uid_copy;
  char name_copy[NAMESZ];
  char *cp;

  while ( (pwent=fgetpwent(new)) != NULL ) {
    if ( pwent->pw_uid < UID_MIN ) DIE("invalid uid %d for user '%s'", pwent->pw_uid, pwent->pw_name); 
    if ( pwent->pw_gid < GID_MIN ) DIE("invalid gid %d for user '%s'", pwent->pw_gid, pwent->pw_name); 
    if ( strcmp(pwent->pw_passwd, "x") ) DIE("password must be 'x'");
    if ( strstr(pwent->pw_dir, HOME) != pwent->pw_dir) DIE("home must be in " HOME);
    for (cp=pwent->pw_name; *cp!='\0'; cp++) {
      if ( ! isdigit(*cp) && ! islower(*cp) ) DIE("invalid username: %s", pwent->pw_name);
    }
    if ( strlen(pwent->pw_name) > NAMESZ ) DIE("name too long: %s", pwent->pw_name);

    /* check duplicates last because fgetpw* clobbers our pwent */
    uid_copy = pwent->pw_uid;
    strcpy(name_copy, pwent->pw_name);
    if ( fgetpwuid(orig, uid_copy) ) DIE("duplicate uid: %d", uid_copy);
    if ( fgetpwnam(orig, name_copy) ) DIE("duplicate name: %s", name_copy);    
  }  
  if ( ferror(new) ) DIES("reading new");
  
  /* everything's great! now write both files out to stdout! */
  if ( fseek(new, 0, SEEK_SET) ) DIES("rewinding new");
  if ( fseek(orig, 0, SEEK_SET) ) DIES("rewinding orig");
  cat(orig);
  cat(new);
}


int main(int argc, char **argv) {
  FILE *orig, *new;

  if ( argc < 3 ) USAGE();

  if ( (orig=fopen(argv[1], "r")) == NULL ) DIES("opening orig");

  if ( (new=fopen(argv[2], "r")) == NULL ) DIES("opening new");  

  graft_passwd(orig, new);
}
