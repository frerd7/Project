/* gpt.c - Read GUID Partition Tables (GPT).  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2005,2006,2007,2008  Free Software Foundation, Inc.
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

#include <grub/disk.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/partition.h>
#include <grub/dl.h>
#include <grub/msdos_partition.h>
#include <grub/gpt_partition.h>

static grub_uint8_t grub_gpt_magic[8] =
  {
    0x45, 0x46, 0x49, 0x20, 0x50, 0x41, 0x52, 0x54
  };

static const grub_gpt_part_type_t grub_gpt_partition_type_empty = GRUB_GPT_PARTITION_TYPE_EMPTY;

static struct grub_partition_map grub_gpt_partition_map;



static grub_err_t
gpt_partition_map_iterate (grub_disk_t disk,
			   int (*hook) (grub_disk_t disk,
					const grub_partition_t partition,
					void *closure),
			   void *closure)
{
  struct grub_partition part;
  struct grub_gpt_header gpt;
  struct grub_gpt_partentry entry;
  struct grub_disk raw;
  struct grub_msdos_partition_mbr mbr;
  grub_uint64_t entries;
  unsigned int i;
  int last_offset = 0;

  /* Enforce raw disk access.  */
  raw = *disk;
  raw.partition = 0;

  /* Read the protective MBR.  */
  if (grub_disk_read (&raw, 0, 0, sizeof (mbr), &mbr))
    return grub_errno;

  /* Check if it is valid.  */
  if (mbr.signature != grub_cpu_to_le16 (GRUB_PC_PARTITION_SIGNATURE))
    return grub_error (GRUB_ERR_BAD_PART_TABLE, "no signature");

  /* Make sure the MBR is a protective MBR and not a normal MBR.  */
  if (mbr.entries[0].type != GRUB_PC_PARTITION_TYPE_GPT_DISK)
    return grub_error (GRUB_ERR_BAD_PART_TABLE, "no GPT partition map found");

  /* Read the GPT header.  */
  if (grub_disk_read (&raw, 1, 0, sizeof (gpt), &gpt))
    return grub_errno;

  if (grub_memcmp (gpt.magic, grub_gpt_magic, sizeof (grub_gpt_magic)))
    return grub_error (GRUB_ERR_BAD_PART_TABLE, "no valid GPT header");

  grub_dprintf ("gpt", "Read a valid GPT header\n");

  entries = grub_le_to_cpu64 (gpt.partitions);
  for (i = 0; i < grub_le_to_cpu32 (gpt.maxpart); i++)
    {
      if (grub_disk_read (&raw, entries, last_offset,
			  sizeof (entry), &entry))
	return grub_errno;

      if (grub_memcmp (&grub_gpt_partition_type_empty, &entry.type,
		       sizeof (grub_gpt_partition_type_empty)))
	{
	  /* Calculate the first block and the size of the partition.  */
	  part.start = grub_le_to_cpu64 (entry.start);
	  part.len = (grub_le_to_cpu64 (entry.end)
		      - grub_le_to_cpu64 (entry.start) + 1);
	  part.offset = entries;
	  part.index = i;
	  part.partmap = &grub_gpt_partition_map;
	  part.data = &entry;

	  grub_dprintf ("gpt", "GPT entry %d: start=%lld, length=%lld\n", i,
			(unsigned long long) part.start,
			(unsigned long long) part.len);

	  if (hook (disk, &part, closure))
	    return 1;
	}

      last_offset += grub_le_to_cpu32 (gpt.partentry_size);
      if (last_offset == GRUB_DISK_SECTOR_SIZE)
	{
	  last_offset = 0;
	  entries++;
	}
    }

  return 0;
}

struct gpt_partition_map_probe_closure
{
  grub_partition_t p;
  int partnum;
};

static int
find_func (grub_disk_t d __attribute__ ((unused)),
	   const grub_partition_t partition, void *closure)
{
  struct gpt_partition_map_probe_closure *c = closure;

  if (c->partnum == partition->index)
    {
      c->p = (grub_partition_t) grub_malloc (sizeof (*c->p));
      if (! c->p)
	return 1;

      grub_memcpy (c->p, partition, sizeof (*c->p));
      return 1;
    }

  return 0;
}

static grub_partition_t
gpt_partition_map_probe (grub_disk_t disk, const char *str)
{
  char *s = (char *) str;
  struct gpt_partition_map_probe_closure c;

  c.p = 0;
  /* Get the partition number.  */
  c.partnum = grub_strtoul (s, 0, 10) - 1;
  if (grub_errno)
    {
      grub_error (GRUB_ERR_BAD_FILENAME, "invalid partition");
      return 0;
    }

  gpt_partition_map_iterate (disk, find_func, &c);
  if (grub_errno)
    goto fail;

  return c.p;

 fail:
  grub_free (c.p);
  return 0;
}


static char *
gpt_partition_map_get_name (const grub_partition_t p)
{
  return grub_xasprintf ("%d", p->index + 1);
}


/* Partition map type.  */
static struct grub_partition_map grub_gpt_partition_map =
  {
    .name = "part_gpt",
    .iterate = gpt_partition_map_iterate,
    .probe = gpt_partition_map_probe,
    .get_name = gpt_partition_map_get_name
  };

GRUB_MOD_INIT(gpt_partition_map)
{
  grub_partition_map_register (&grub_gpt_partition_map);
}

GRUB_MOD_FINI(gpt_partition_map)
{
  grub_partition_map_unregister (&grub_gpt_partition_map);
}
