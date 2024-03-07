/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010 Free Software Foundation, Inc.
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

#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/extcmd.h>
#include <grub/test.h>
#include <stddef.h>

static int run_test(grub_test_t test)
{
  grub_test_run(test);
  return 0;
}

static grub_err_t grub_functional_test(struct grub_extcmd *cmd __attribute__ ((unused)),
                                       int argc __attribute__ ((unused)),
                                       char **args __attribute__ ((unused)))
{
  struct grub_list *test_list = GRUB_AS_LIST(grub_test_list);

  struct grub_list *iter;
  for (iter = test_list->next; iter != test_list; iter = iter->next)
  {
    // Manually extracting the structure pointer
    struct grub_test *test = (struct grub_test *)((char *)iter - offsetof(struct grub_test, list));

    int result = run_test(test);

    if (result != 0)
    {
      grub_printf("Test '%s' failed", test->name);
      return GRUB_ERR_IO;
      // Handle the error as needed
    }
  }

  return GRUB_ERR_NONE;
}

static grub_extcmd_t cmd;

GRUB_MOD_INIT (functional_test)
{
  cmd = grub_register_extcmd ("functional_test", grub_functional_test,
			      GRUB_COMMAND_FLAG_CMDLINE, 0,
			      "Run all functional tests.", 0);
}

GRUB_MOD_FINI (functional_test)
{
  grub_unregister_extcmd (cmd);
}
