/*
 * Main coding by Elliot Lee <sopwith@redhat.com>, Red Hat Software.
 * Copyright (C) 1996.
 * Copyright (c) Jan RÍkorajski, 1999.
 * Copyright (c) Red Hat, Inc., 2007.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, and the entire permission notice in its entirety,
 *    including the disclaimer of warranties.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * ALTERNATIVELY, this product may be distributed under the terms of
 * the GNU Public License, in which case the provisions of the GPL are
 * required INSTEAD OF the above restrictions.  (This clause is
 * necessary due to a potential bad interaction between the GPL and
 * the restrictions contained in a BSD-style copyright.)
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <pwd.h>
#include <syslog.h>
#include <shadow.h>
#include <time.h>		/* for time() */
#include <fcntl.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#ifdef WITH_SELINUX
static int selinux_enabled=-1;
#include <selinux/selinux.h>
static security_context_t prev_context=NULL;
#define SELINUX_ENABLED (selinux_enabled!=-1 ? selinux_enabled : (selinux_enabled=is_selinux_enabled()>0))
#endif

#ifdef USE_CRACKLIB
#include <crack.h>
#endif

#include <security/_pam_macros.h>

/* indicate the following groups are defined */

#define PAM_SM_PASSWORD

#include <security/pam_modules.h>
#include <security/pam_ext.h>
#include <security/pam_modutil.h>

#include "yppasswd.h"
#include "md5.h"
#include "support.h"
#include "bigcrypt.h"

#if !((__GLIBC__ == 2) && (__GLIBC_MINOR__ >= 1))
extern int getrpcport(const char *host, unsigned long prognum,
		      unsigned long versnum, unsigned int proto);
#endif				/* GNU libc 2.1 */

/*
   How it works:
   Gets in username (has to be done) from the calling program
   Does authentication of user (only if we are not running as root)
   Gets new password/checks for sanity
   Sets it.
 */

/* data tokens */

#define _UNIX_OLD_AUTHTOK	"-UN*X-OLD-PASS"
#define _UNIX_NEW_AUTHTOK	"-UN*X-NEW-PASS"

#define MAX_PASSWD_TRIES	3
#ifndef CRACKLIB_DICTS
#define CRACKLIB_DICTS		NULL
#endif

static char *getNISserver(pam_handle_t *pamh)
{
	char *master;
	char *domainname;
	int port, err;

	if ((err = yp_get_default_domain(&domainname)) != 0) {
		pam_syslog(pamh, LOG_WARNING, "can't get local yp domain: %s",
			 yperr_string(err));
		return NULL;
	}
	if ((err = yp_master(domainname, "passwd.byname", &master)) != 0) {
		pam_syslog(pamh, LOG_WARNING, "can't find the master ypserver: %s",
			 yperr_string(err));
		return NULL;
	}
	port = getrpcport(master, YPPASSWDPROG, YPPASSWDPROC_UPDATE, IPPROTO_UDP);
	if (port == 0) {
		pam_syslog(pamh, LOG_WARNING,
		         "yppasswdd not running on NIS master host");
		return NULL;
	}
	if (port >= IPPORT_RESERVED) {
		pam_syslog(pamh, LOG_WARNING,
		         "yppasswd daemon running on illegal port");
		return NULL;
	}
	return master;
}

#ifdef WITH_SELINUX

static int _unix_run_update_binary(pam_handle_t *pamh, unsigned int ctrl, const char *user,
    const char *fromwhat, const char *towhat, int remember)
{
    int retval, child, fds[2];
    void (*sighandler)(int) = NULL;

    D(("called."));
    /* create a pipe for the password */
    if (pipe(fds) != 0) {
	D(("could not make pipe"));
	return PAM_AUTH_ERR;
    }

    if (off(UNIX_NOREAP, ctrl)) {
	/*
	 * This code arranges that the demise of the child does not cause
	 * the application to receive a signal it is not expecting - which
	 * may kill the application or worse.
	 *
	 * The "noreap" module argument is provided so that the admin can
	 * override this behavior.
	 */
	sighandler = signal(SIGCHLD, SIG_DFL);
    }

    /* fork */
    child = fork();
    if (child == 0) {
        size_t i=0;
        struct rlimit rlim;
	static char *envp[] = { NULL };
	char *args[] = { NULL, NULL, NULL, NULL, NULL, NULL };
        char buffer[16];

	/* XXX - should really tidy up PAM here too */

	close(0); close(1);
	/* reopen stdin as pipe */
	close(fds[1]);
	dup2(fds[0], STDIN_FILENO);

	if (getrlimit(RLIMIT_NOFILE,&rlim)==0) {
	  for (i=2; i < rlim.rlim_max; i++) {
	    if ((unsigned int)fds[0] != i)
	  	   close(i);
	  }
	}

	/* exec binary helper */
	args[0] = x_strdup(UPDATE_HELPER);
	args[1] = x_strdup(user);
	args[2] = x_strdup("update");
	if (on(UNIX_SHADOW, ctrl))
		args[3] = x_strdup("1");
	else
		args[3] = x_strdup("0");

        snprintf(buffer, sizeof(buffer), "%d", remember);
        args[4] = strdup(buffer);
	
	execve(UPDATE_HELPER, args, envp);

	/* should not get here: exit with error */
	D(("helper binary is not available"));
	exit(PAM_AUTHINFO_UNAVAIL);
    } else if (child > 0) {
	/* wait for child */
	/* if the stored password is NULL */
        int rc=0;
	if (fromwhat)
	  pam_modutil_write(fds[1], fromwhat, strlen(fromwhat)+1);
	else
	  pam_modutil_write(fds[1], "", 1);
	if (towhat) {
	  pam_modutil_write(fds[1], towhat, strlen(towhat)+1);
	}
	else
	  pam_modutil_write(fds[1], "", 1);

	close(fds[0]);       /* close here to avoid possible SIGPIPE above */
	close(fds[1]);
	rc=waitpid(child, &retval, 0);  /* wait for helper to complete */
	if (rc<0) {
	  pam_syslog(pamh, LOG_ERR, "unix_update waitpid failed: %m");
	  retval = PAM_AUTH_ERR;
	} else {
	  retval = WEXITSTATUS(retval);
	}
    } else {
	D(("fork failed"));
	close(fds[0]);
 	close(fds[1]);
	retval = PAM_AUTH_ERR;
    }

    if (sighandler != SIG_ERR) {
        (void) signal(SIGCHLD, sighandler);   /* restore old signal handler */
    }

    return retval;
}

static int selinux_confined(void)
{
    static int confined = -1;
    int fd;
    char tempfile[]="/etc/.pwdXXXXXX";

    if (confined != -1)
    	return confined;

    /* cannot be confined without SELinux enabled */
    if (!SELINUX_ENABLED){
       	confined = 0;
       	return confined;
    }
    
    /* let's try opening shadow read only */
    if ((fd=open("/etc/shadow", O_RDONLY)) != -1) {
        close(fd);
        confined = 0;
        return confined;
    }

    if (errno == EACCES) {
	confined = 1;
	return confined;
    }
    
    /* shadow opening failed because of other reasons let's try 
       creating a file in /etc */
    if ((fd=mkstemp(tempfile)) != -1) {
        unlink(tempfile);
        close(fd);
        confined = 0;
        return confined;
    }
    
    confined = 1;
    return confined;
}

#else
static int selinux_confined(void)
{
    return 0;
}
#endif

#include "passupdate.c"

static int check_old_password(const char *forwho, const char *newpass)
{
	static char buf[16384];
	char *s_luser, *s_uid, *s_npas, *s_pas;
	int retval = PAM_SUCCESS;
	FILE *opwfile;

	opwfile = fopen(OLD_PASSWORDS_FILE, "r");
	if (opwfile == NULL)
		return PAM_ABORT;

	while (fgets(buf, 16380, opwfile)) {
		if (!strncmp(buf, forwho, strlen(forwho))) {
			buf[strlen(buf) - 1] = '\0';
			s_luser = strtok(buf, ":,");
			s_uid = strtok(NULL, ":,");
			s_npas = strtok(NULL, ":,");
			s_pas = strtok(NULL, ":,");
			while (s_pas != NULL) {
				char *md5pass = Goodcrypt_md5(newpass, s_pas);
				if (!strcmp(md5pass, s_pas)) {
					_pam_delete(md5pass);
					retval = PAM_AUTHTOK_ERR;
					break;
				}
				s_pas = strtok(NULL, ":,");
				_pam_delete(md5pass);
			}
			break;
		}
	}
	fclose(opwfile);

	return retval;
}

static int _do_setpass(pam_handle_t* pamh, const char *forwho,
		       const char *fromwhat,
		       char *towhat, unsigned int ctrl, int remember)
{
	struct passwd *pwd = NULL;
	int retval = 0;
	int unlocked = 0;
	char *master = NULL;

	D(("called"));

	pwd = getpwnam(forwho);

	if (pwd == NULL) {
		retval = PAM_AUTHTOK_ERR;
		goto done;
	}

	if (on(UNIX_NIS, ctrl) && _unix_comesfromsource(pamh, forwho, 0, 1)) {
	    if ((master=getNISserver(pamh)) != NULL) {
		struct timeval timeout;
		struct yppasswd yppwd;
		CLIENT *clnt;
		int status;
		enum clnt_stat err;

		/* Unlock passwd file to avoid deadlock */
#ifdef USE_LCKPWDF
		unlock_pwdf();
#endif
		unlocked = 1;

		/* Initialize password information */
		yppwd.newpw.pw_passwd = pwd->pw_passwd;
		yppwd.newpw.pw_name = pwd->pw_name;
		yppwd.newpw.pw_uid = pwd->pw_uid;
		yppwd.newpw.pw_gid = pwd->pw_gid;
		yppwd.newpw.pw_gecos = pwd->pw_gecos;
		yppwd.newpw.pw_dir = pwd->pw_dir;
		yppwd.newpw.pw_shell = pwd->pw_shell;
		yppwd.oldpass = fromwhat ? strdup (fromwhat) : strdup ("");
		yppwd.newpw.pw_passwd = towhat;

		D(("Set password %s for %s", yppwd.newpw.pw_passwd, forwho));

		/* The yppasswd.x file said `unix authentication required',
		 * so I added it. This is the only reason it is in here.
		 * My yppasswdd doesn't use it, but maybe some others out there
		 * do.                                        --okir
		 */
		clnt = clnt_create(master, YPPASSWDPROG, YPPASSWDVERS, "udp");
		clnt->cl_auth = authunix_create_default();
		memset((char *) &status, '\0', sizeof(status));
		timeout.tv_sec = 25;
		timeout.tv_usec = 0;
		err = clnt_call(clnt, YPPASSWDPROC_UPDATE,
				(xdrproc_t) xdr_yppasswd, (char *) &yppwd,
				(xdrproc_t) xdr_int, (char *) &status,
				timeout);

		free (yppwd.oldpass);

		if (err) {
			_make_remark(pamh, ctrl, PAM_TEXT_INFO,
				clnt_sperrno(err));
		} else if (status) {
			D(("Error while changing NIS password.\n"));
		}
		D(("The password has%s been changed on %s.",
		   (err || status) ? " not" : "", master));
		pam_syslog(pamh, LOG_NOTICE, "password%s changed for %s on %s",
			 (err || status) ? " not" : "", pwd->pw_name, master);

		auth_destroy(clnt->cl_auth);
		clnt_destroy(clnt);
		if (err || status) {
			_make_remark(pamh, ctrl, PAM_TEXT_INFO,
				_("NIS password could not be changed."));
			retval = PAM_TRY_AGAIN;
		}
#ifdef DEBUG
		sleep(5);
#endif
	    } else {
		    retval = PAM_TRY_AGAIN;
	    }
	}

	if (_unix_comesfromsource(pamh, forwho, 1, 0)) {
#ifdef USE_LCKPWDF
		if(unlocked) {
			if (lock_pwdf() != PAM_SUCCESS) {
				return PAM_AUTHTOK_LOCK_BUSY;
			}
		}
#endif
#ifdef WITH_SELINUX
	        if (selinux_confined())
			  return _unix_run_update_binary(pamh, ctrl, forwho, fromwhat, towhat, remember);
#endif
		/* first, save old password */
		if (save_old_password(forwho, fromwhat, remember)) {
			retval = PAM_AUTHTOK_ERR;
			goto done;
		}
		if (on(UNIX_SHADOW, ctrl) || _unix_shadowed(pwd)) {
			retval = _update_shadow(pamh, forwho, towhat);
			if (retval == PAM_SUCCESS)
				if (!_unix_shadowed(pwd))
					retval = _update_passwd(pamh, forwho, "x");
		} else {
			retval = _update_passwd(pamh, forwho, towhat);
		}
	}


done:
#ifdef USE_LCKPWDF
	unlock_pwdf();
#endif

	return retval;
}

static int _unix_verify_shadow(pam_handle_t *pamh, const char *user, unsigned int ctrl)
{
	struct passwd *pwd = NULL;	/* Password and shadow password */
	struct spwd *spwdent = NULL;	/* file entries for the user */
	time_t curdays;
	int retval = PAM_SUCCESS;

	/* UNIX passwords area */
	pwd = getpwnam(user);	/* Get password file entry... */
	if (pwd == NULL)
		return PAM_AUTHINFO_UNAVAIL;	/* We don't need to do the rest... */

	if (_unix_shadowed(pwd)) {
		/* ...and shadow password file entry for this user, if shadowing
		   is enabled */
#ifdef WITH_SELINUX
		if (selinux_confined())
			spwdent = _unix_run_verify_binary(pamh, ctrl, user);
		else
		{
#endif
			setspent();
			spwdent = getspnam(user);
			endspent();
#ifdef WITH_SELINUX
		}
#endif
		if (spwdent == NULL)
			return PAM_AUTHINFO_UNAVAIL;
	} else {
		if (strcmp(pwd->pw_passwd,"*NP*") == 0) { /* NIS+ */
			uid_t save_uid;

			save_uid = geteuid();
			seteuid (pwd->pw_uid);
			spwdent = getspnam( user );
			seteuid (save_uid);

			if (spwdent == NULL)
				return PAM_AUTHINFO_UNAVAIL;
		} else
			spwdent = NULL;
	}

	if (spwdent != NULL) {
		/* We have the user's information, now let's check if their account
		   has expired (60 * 60 * 24 = number of seconds in a day) */

		if (off(UNIX__IAMROOT, ctrl)) {
			/* Get the current number of days since 1970 */
			curdays = time(NULL) / (60 * 60 * 24);
			if (curdays < spwdent->sp_lstchg) {
				pam_syslog(pamh, LOG_DEBUG,
					"account %s has password changed in future",
					user);
				curdays = spwdent->sp_lstchg;
			}
			if ((curdays - spwdent->sp_lstchg < spwdent->sp_min)
				 && (spwdent->sp_min != -1))
				/*
				 * The last password change was too recent.
				 */
				retval = PAM_AUTHTOK_ERR;
			else if ((curdays - spwdent->sp_lstchg > spwdent->sp_max)
				 && (curdays - spwdent->sp_lstchg > spwdent->sp_inact)
				 && (curdays - spwdent->sp_lstchg >
				     spwdent->sp_max + spwdent->sp_inact)
				 && (spwdent->sp_max != -1) && (spwdent->sp_inact != -1)
				 && (spwdent->sp_lstchg != 0))
				/*
				 * Their password change has been put off too long,
				 */
				retval = PAM_ACCT_EXPIRED;
			else if ((curdays > spwdent->sp_expire) && (spwdent->sp_expire != -1)
				 && (spwdent->sp_lstchg != 0))
				/*
				 * OR their account has just plain expired
				 */
				retval = PAM_ACCT_EXPIRED;
		}
	}
	return retval;
}

static int _pam_unix_approve_pass(pam_handle_t * pamh
				  ,unsigned int ctrl
				  ,const char *pass_old
				  ,const char *pass_new)
{
	const void *user;
	const char *remark = NULL;
	int retval = PAM_SUCCESS;

	D(("&new=%p, &old=%p", pass_old, pass_new));
	D(("new=[%s]", pass_new));
	D(("old=[%s]", pass_old));

	if (pass_new == NULL || (pass_old && !strcmp(pass_old, pass_new))) {
		if (on(UNIX_DEBUG, ctrl)) {
			pam_syslog(pamh, LOG_DEBUG, "bad authentication token");
		}
		_make_remark(pamh, ctrl, PAM_ERROR_MSG, pass_new == NULL ?
			_("No password supplied") : _("Password unchanged"));
		return PAM_AUTHTOK_ERR;
	}
	/*
	 * if one wanted to hardwire authentication token strength
	 * checking this would be the place - AGM
	 */

	retval = pam_get_item(pamh, PAM_USER, &user);
	if (retval != PAM_SUCCESS) {
		if (on(UNIX_DEBUG, ctrl)) {
			pam_syslog(pamh, LOG_ERR, "Can not get username");
			return PAM_AUTHTOK_ERR;
		}
	}
	if (off(UNIX__IAMROOT, ctrl)) {
#if 0
		remark = FascistCheck (pass_new, CRACKLIB_DICTS);
		D(("called cracklib [%s]", remark));
#else
		if (strlen(pass_new) < 6)
		  remark = _("You must choose a longer password");
		D(("length check [%s]", remark));
#endif
		if (on(UNIX_REMEMBER_PASSWD, ctrl)) {
			if ((retval = check_old_password(user, pass_new)) == PAM_AUTHTOK_ERR)
			  remark = _("Password has been already used. Choose another.");
			if (retval == PAM_ABORT) {
				pam_syslog(pamh, LOG_ERR, "can't open %s file to check old passwords",
					OLD_PASSWORDS_FILE);
				return retval;
			}
		}
	}
	if (remark) {
		_make_remark(pamh, ctrl, PAM_ERROR_MSG, remark);
		retval = PAM_AUTHTOK_ERR;
	}
	return retval;
}


PAM_EXTERN int pam_sm_chauthtok(pam_handle_t * pamh, int flags,
				int argc, const char **argv)
{
	unsigned int ctrl, lctrl;
	int retval;
	int remember = -1;
	int rounds = -1;

	/* <DO NOT free() THESE> */
	const char *user;
	const void *pass_old, *pass_new;
	/* </DO NOT free() THESE> */

	D(("called."));

	ctrl = _set_ctrl(pamh, flags, &remember, &rounds, argc, argv);

	/*
	 * First get the name of a user
	 */
	retval = pam_get_user(pamh, &user, NULL);
	if (retval == PAM_SUCCESS) {
		/*
		 * Various libraries at various times have had bugs related to
		 * '+' or '-' as the first character of a user name. Don't take
		 * any chances here. Require that the username starts with an
		 * alphanumeric character.
		 */
		if (user == NULL || (!isalnum(*user) &&
		    *user != '_' && *user != '.')) {
			pam_syslog(pamh, LOG_ERR, "bad username [%s]", user);
			return PAM_USER_UNKNOWN;
		}
		if (retval == PAM_SUCCESS && on(UNIX_DEBUG, ctrl))
			pam_syslog(pamh, LOG_DEBUG, "username [%s] obtained",
			         user);
	} else {
		if (on(UNIX_DEBUG, ctrl))
			pam_syslog(pamh, LOG_DEBUG,
			         "password - could not identify user");
		return retval;
	}

	D(("Got username of %s", user));

	/*
	 * Before we do anything else, check to make sure that the user's
	 * info is in one of the databases we can modify from this module,
	 * which currently is 'files' and 'nis'.  We have to do this because
	 * getpwnam() doesn't tell you *where* the information it gives you
	 * came from, nor should it.  That's our job.
	 */
	if (_unix_comesfromsource(pamh, user, 1, on(UNIX_NIS, ctrl)) == 0) {
		pam_syslog(pamh, LOG_DEBUG,
			 "user \"%s\" does not exist in /etc/passwd%s",
			 user, on(UNIX_NIS, ctrl) ? " or NIS" : "");
		return PAM_USER_UNKNOWN;
	} else {
		struct passwd *pwd;
		_unix_getpwnam(pamh, user, 1, 1, &pwd);
		if (pwd == NULL) {
			pam_syslog(pamh, LOG_DEBUG,
				"user \"%s\" has corrupted passwd entry",
				user);
			return PAM_USER_UNKNOWN;
		}
		if (!_unix_shadowed(pwd) &&
		    (strchr(pwd->pw_passwd, '*') != NULL)) {
			pam_syslog(pamh, LOG_DEBUG,
				"user \"%s\" does not have modifiable password",
				user);
			return PAM_USER_UNKNOWN;
		}
	}

	/*
	 * This is not an AUTH module!
	 */
	if (on(UNIX__NONULL, ctrl))
		set(UNIX__NULLOK, ctrl);

	if (on(UNIX__PRELIM, ctrl)) {
		/*
		 * obtain and verify the current password (OLDAUTHTOK) for
		 * the user.
		 */
		char *Announce;

		D(("prelim check"));

		if (_unix_blankpasswd(pamh, ctrl, user)) {
			return PAM_SUCCESS;
		} else if (off(UNIX__IAMROOT, ctrl)) {

			/* instruct user what is happening */
#define greeting "Changing password for "
			Announce = (char *) malloc(sizeof(greeting) + strlen(user));
			if (Announce == NULL) {
				pam_syslog(pamh, LOG_CRIT,
				         "password - out of memory");
				return PAM_BUF_ERR;
			}
			(void) strcpy(Announce, greeting);
			(void) strcpy(Announce + sizeof(greeting) - 1, user);
#undef greeting

			lctrl = ctrl;
			set(UNIX__OLD_PASSWD, lctrl);
			retval = _unix_read_password(pamh, lctrl
						     ,Announce
					     ,_("(current) UNIX password: ")
						     ,NULL
						     ,_UNIX_OLD_AUTHTOK
					     ,&pass_old);
			free(Announce);

			if (retval != PAM_SUCCESS) {
				pam_syslog(pamh, LOG_NOTICE,
				    "password - (old) token not obtained");
				return retval;
			}
			/* verify that this is the password for this user */

			retval = _unix_verify_password(pamh, user, pass_old, ctrl);
		} else {
			D(("process run by root so do nothing this time around"));
			pass_old = NULL;
			retval = PAM_SUCCESS;	/* root doesn't have too */
		}

		if (retval != PAM_SUCCESS) {
			D(("Authentication failed"));
			pass_old = NULL;
			return retval;
		}
		retval = pam_set_item(pamh, PAM_OLDAUTHTOK, (const void *) pass_old);
		pass_old = NULL;
		if (retval != PAM_SUCCESS) {
			pam_syslog(pamh, LOG_CRIT,
			         "failed to set PAM_OLDAUTHTOK");
		}
		retval = _unix_verify_shadow(pamh,user, ctrl);
		if (retval == PAM_AUTHTOK_ERR) {
			if (off(UNIX__IAMROOT, ctrl))
				_make_remark(pamh, ctrl, PAM_ERROR_MSG,
					     _("You must wait longer to change your password"));
			else
				retval = PAM_SUCCESS;
		}
	} else if (on(UNIX__UPDATE, ctrl)) {
		/*
		 * tpass is used below to store the _pam_md() return; it
		 * should be _pam_delete()'d.
		 */

		char *tpass = NULL;
		int retry = 0;

		/*
		 * obtain the proposed password
		 */

		D(("do update"));

		/*
		 * get the old token back. NULL was ok only if root [at this
		 * point we assume that this has already been enforced on a
		 * previous call to this function].
		 */

		if (off(UNIX_NOT_SET_PASS, ctrl)) {
			retval = pam_get_item(pamh, PAM_OLDAUTHTOK
					      ,&pass_old);
		} else {
			retval = pam_get_data(pamh, _UNIX_OLD_AUTHTOK
					      ,&pass_old);
			if (retval == PAM_NO_MODULE_DATA) {
				retval = PAM_SUCCESS;
				pass_old = NULL;
			}
		}
		D(("pass_old [%s]", pass_old));

		if (retval != PAM_SUCCESS) {
			pam_syslog(pamh, LOG_NOTICE, "user not authenticated");
			return retval;
		}

		D(("get new password now"));

		lctrl = ctrl;

		if (on(UNIX_USE_AUTHTOK, lctrl)) {
			set(UNIX_USE_FIRST_PASS, lctrl);
		}
		retry = 0;
		retval = PAM_AUTHTOK_ERR;
		while ((retval != PAM_SUCCESS) && (retry++ < MAX_PASSWD_TRIES)) {
			/*
			 * use_authtok is to force the use of a previously entered
			 * password -- needed for pluggable password strength checking
			 */

			retval = _unix_read_password(pamh, lctrl
						     ,NULL
					     ,_("Enter new UNIX password: ")
					    ,_("Retype new UNIX password: ")
						     ,_UNIX_NEW_AUTHTOK
					     ,&pass_new);

			if (retval != PAM_SUCCESS) {
				if (on(UNIX_DEBUG, ctrl)) {
					pam_syslog(pamh, LOG_ALERT,
						 "password - new password not obtained");
				}
				pass_old = NULL;	/* tidy up */
				return retval;
			}
			D(("returned to _unix_chauthtok"));

			/*
			 * At this point we know who the user is and what they
			 * propose as their new password. Verify that the new
			 * password is acceptable.
			 */

			if (*(const char *)pass_new == '\0') {	/* "\0" password = NULL */
				pass_new = NULL;
			}
			retval = _pam_unix_approve_pass(pamh, ctrl, pass_old, pass_new);
			
			if (retval != PAM_SUCCESS && off(UNIX_NOT_SET_PASS, ctrl)) {
				pam_set_item(pamh, PAM_AUTHTOK, NULL);
			}
		}

		if (retval != PAM_SUCCESS) {
			pam_syslog(pamh, LOG_NOTICE,
			         "new password not acceptable");
			pass_new = pass_old = NULL;	/* tidy up */
			return retval;
		}
#ifdef USE_LCKPWDF
		if (lock_pwdf() != PAM_SUCCESS) {
			return PAM_AUTHTOK_LOCK_BUSY;
		}
#endif

		if (!selinux_confined() && pass_old) {
			retval = _unix_verify_password(pamh, user, pass_old, ctrl);
			if (retval != PAM_SUCCESS) {
				pam_syslog(pamh, LOG_NOTICE, "user password changed by another process");
#ifdef USE_LCKPWDF
				unlock_pwdf();
#endif
				return retval;
			}
		}

		
		if (!selinux_confined() && 
		    (retval=_unix_verify_shadow(pamh, user, ctrl)) != PAM_SUCCESS) {
			pam_syslog(pamh, LOG_NOTICE, "user not authenticated 2");
#ifdef USE_LCKPWDF
			unlock_pwdf();
#endif
			return retval;
		}


		if (!selinux_confined() &&
		    (retval=_pam_unix_approve_pass(pamh, ctrl, pass_old, pass_new)) != PAM_SUCCESS) {
			pam_syslog(pamh, LOG_NOTICE,
			         "new password not acceptable 2");
			pass_new = pass_old = NULL;	/* tidy up */
#ifdef USE_LCKPWDF
			unlock_pwdf();
#endif
			return retval;
		}

		/*
		 * By reaching here we have approved the passwords and must now
		 * rebuild the password database file.
		 */

		/*
		 * First we encrypt the new password.
		 */

		tpass = create_password_hash(pass_new, ctrl, rounds);
		if (tpass == NULL) {
			pam_syslog(pamh, LOG_CRIT,
				"out of memory for password");
			pass_new = pass_old = NULL;	/* tidy up */
			unlock_pwdf();
			return PAM_BUF_ERR;
		}

		D(("password processed"));

		/* update the password database(s) -- race conditions..? */

		retval = _do_setpass(pamh, user, pass_old, tpass, ctrl,
		                     remember);
	        /* _do_setpass has called unlock_pwdf for us */

		_pam_delete(tpass);
		pass_old = pass_new = NULL;
	} else {		/* something has broken with the module */
		pam_syslog(pamh, LOG_ALERT,
		         "password received unknown request");
		retval = PAM_ABORT;
	}

	D(("retval was %d", retval));

	return retval;
}


/* static module data */
#ifdef PAM_STATIC
struct pam_module _pam_unix_passwd_modstruct = {
    "pam_unix_passwd",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    pam_sm_chauthtok,
};
#endif
