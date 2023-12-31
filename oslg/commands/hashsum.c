/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/dl.h>
#include <grub/extcmd.h>
#include <grub/file.h>
#include <grub/disk.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/crypto.h>
#include <grub/lib.h>

static const struct grub_arg_option options[] = {
  {"hash", 'h', 0, "Specify hash to use.", "HASH", ARG_TYPE_STRING},
  {"check", 'c', 0, "Check hash list file.", "FILE", ARG_TYPE_STRING},
  {"prefix", 'p', 0, "Base directory for hash list.", "DIRECTORY",
   ARG_TYPE_STRING},
  {"keep-going", 'k', 0, "Don't stop after first error.", 0, 0},
  {0, 0, 0, 0, 0, 0}
};

struct { const char *name; const char *hashname; } aliases[] =
  {
    {"sha256sum", "sha256"},
    {"sha512sum", "sha512"},
    {"md5sum", "md5"},
  };

static inline int
hextoval (char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}

static grub_err_t
hash_file (grub_file_t file, const gcry_md_spec_t *hash, void *result)
{
  grub_uint8_t context[hash->contextsize];
  grub_uint8_t readbuf[4096];

  grub_memset (context, 0, sizeof (context));
  hash->init (context);
  while (1)
    {
      grub_ssize_t r;
      r = grub_file_read (file, readbuf, sizeof (readbuf));
      if (r < 0)
	return grub_errno;
      if (r == 0)
	break;
      hash->write (context, readbuf, r);
    }
  hash->final (context);
  grub_memcpy (result, hash->read (context), hash->mdlen);

  return GRUB_ERR_NONE;
}

static grub_err_t
check_list (const gcry_md_spec_t *hash, const char *hashfilename,
	    const char *prefix, int keep)
{
  grub_file_t hashlist, file;
  char *buf = NULL;
  grub_uint8_t expected[hash->mdlen];
  grub_uint8_t actual[hash->mdlen];
  grub_err_t err;
  unsigned i;
  unsigned unread = 0, mismatch = 0;

  hashlist = grub_file_open (hashfilename);
  if (!hashlist)
    return grub_errno;

  while (grub_free (buf), (buf = grub_getline (hashlist)))
    {
      const char *p = buf;
      for (i = 0; i < hash->mdlen; i++)
	{
	  int high, low;
	  high = hextoval (*p++);
	  low = hextoval (*p++);
	  if (high < 0 || low < 0)
	    return grub_error (GRUB_ERR_BAD_FILE_TYPE, "invalid hash list");
	  expected[i] = (high << 4) | low;
	}
      if (*p++ != ' ' || *p++ != ' ')
	return grub_error (GRUB_ERR_BAD_FILE_TYPE, "invalid hash list");
      if (prefix)
	{
	  char *filename;

	  filename = grub_xasprintf ("%s/%s", prefix, p);
	  if (!filename)
	    return grub_errno;
	  file = grub_file_open (filename);
	  grub_free (filename);
	}
      else
	file = grub_file_open (p);
      if (!file)
	{
	  grub_file_close (hashlist);
	  grub_free (buf);
	  return grub_errno;
	}
      err = hash_file (file, hash, actual);
      grub_file_close (file);
      if (err)
	{
	  grub_printf ("%s: READ ERROR\n", p);
	  if (!keep)
	    {
	      grub_file_close (hashlist);
	      grub_free (buf);
	      return err;
	    }
	  grub_print_error ();
	  grub_errno = GRUB_ERR_NONE;
	  unread++;
	  continue;
	}
      if (grub_crypto_memcmp (expected, actual, hash->mdlen) != 0)
	{
	  grub_printf ("%s: HASH MISMATCH\n", p);
	  if (!keep)
	    {
	      grub_file_close (hashlist);
	      grub_free (buf);
	      return grub_error (GRUB_ERR_TEST_FAILURE,
				 "hash of '%s' mismatches", p);
	    }
	  mismatch++;
	  continue;
	}
      grub_printf ("%s: OK\n", p);
    }
  if (mismatch || unread)
    return grub_error (GRUB_ERR_TEST_FAILURE,
		       "%d files couldn't be read and hash "
		       "of %d files mismatches", unread, mismatch);
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_cmd_hashsum (struct grub_extcmd *cmd,
		  int argc, char **args)
{
  struct grub_arg_list *state = cmd->state;
  const char *hashname = NULL;
  const char *prefix = NULL;
  const gcry_md_spec_t *hash;
  unsigned i;
  int keep = state[3].set;
  unsigned unread = 0;

  for (i = 0; i < ARRAY_SIZE (aliases); i++)
    if (grub_strcmp (cmd->cmd->name, aliases[i].name) == 0)
      hashname = aliases[i].hashname;
  if (state[0].set)
    hashname = state[0].arg;

  if (!hashname)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "no hash specified");

  hash = grub_crypto_lookup_md_by_name (hashname);
  if (!hash)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "unknown hash");

  if (state[2].set)
    prefix = state[2].arg;

  if (state[1].set)
    {
      if (argc != 0)
	return grub_error (GRUB_ERR_BAD_ARGUMENT,
			   "--check is incompatible with file list");
      return check_list (hash, state[1].arg, prefix, keep);
    }

  for (i = 0; i < (unsigned) argc; i++)
    {
      grub_uint8_t result[hash->mdlen];
      grub_file_t file;
      grub_err_t err;
      unsigned j;
      file = grub_file_open (args[i]);
      if (!file)
	{
	  if (!keep)
	    return grub_errno;
	  grub_print_error ();
	  grub_errno = GRUB_ERR_NONE;
	  unread++;
	  continue;
	}
      err = hash_file (file, hash, result);
      grub_file_close (file);
      if (err)
	{
	  if (!keep)
	    return err;
	  grub_print_error ();
	  grub_errno = GRUB_ERR_NONE;
	  unread++;
	  continue;
	}
      for (j = 0; j < hash->mdlen; j++)
	grub_printf ("%02x", result[j]);
      grub_printf ("  %s\n", args[i]);
    }

  if (unread)
    return grub_error (GRUB_ERR_TEST_FAILURE, "%d files couldn't be read.",
		       unread);
  return GRUB_ERR_NONE;
}

static grub_extcmd_t cmd, cmd_md5, cmd_sha256, cmd_sha512;

GRUB_MOD_INIT(hashsum)
{
  cmd = grub_register_extcmd ("hashsum", grub_cmd_hashsum,
			      GRUB_COMMAND_FLAG_BOTH,
			      "hashsum -h HASH [-c FILE [-p PREFIX]] "
			      "[FILE1 [FILE2 ...]]",
			      "Compute or check hash checksum.",
			      options);
  cmd_md5 = grub_register_extcmd ("md5sum", grub_cmd_hashsum,
				  GRUB_COMMAND_FLAG_BOTH,
				  "md5sum [-c FILE [-p PREFIX]] "
				  "[FILE1 [FILE2 ...]]",
				  "Compute or check hash checksum.",
				  options);
  cmd_sha256 = grub_register_extcmd ("sha256sum", grub_cmd_hashsum,
				     GRUB_COMMAND_FLAG_BOTH,
				     "sha256sum [-c FILE [-p PREFIX]] "
				     "[FILE1 [FILE2 ...]]",
				     "Compute or check hash checksum.",
				     options);
  cmd_sha512 = grub_register_extcmd ("sha512sum", grub_cmd_hashsum,
				     GRUB_COMMAND_FLAG_BOTH,
				     "sha512sum [-c FILE [-p PREFIX]] "
				     "[FILE1 [FILE2 ...]]",
				     "Compute or check hash checksum.",
				     options);
}

GRUB_MOD_FINI(hashsum)
{
  grub_unregister_extcmd (cmd);
  grub_unregister_extcmd (cmd_md5);
  grub_unregister_extcmd (cmd_sha256);
  grub_unregister_extcmd (cmd_sha512);
}
