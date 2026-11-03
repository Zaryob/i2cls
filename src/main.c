/*
 * i2c-tools-ng - Modern, smart I2C diagnostic and interrogation suite
 * Copyright (C) 2026 i2c-tools-ng contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include "i2c_types.h"
#include "i2c_db.h"

/* Subcommand handlers */
int cmd_ls_main(int argc, char *argv[]);
int cmd_tree_main(int argc, char *argv[]);
int cmd_probe_main(int argc, char *argv[]);
int cmd_bind_main(int argc, char *argv[]);
int cmd_unbind_main(int argc, char *argv[]);

static void print_general_help(const char *prog_name)
{
    printf("%si2c-tools-ng%s v%s - Next-Generation I2C Toolkit & Interrogation Suite\n",
           COLOR_BOLD, COLOR_RESET, I2CTOOL_VERSION);
    printf("Licensed under GNU GPLv3. Written from scratch for modern embedded Linux.\n\n");
    printf("%sUsage:%s %s <command> [arguments]\n\n", COLOR_BOLD, COLOR_RESET, prog_name);
    printf("%sAvailable Commands:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  %si2cls%s      (ls)       Smart I2C bus scanner & chip identification\n", COLOR_CYAN, COLOR_RESET);
    printf("  %si2ctree%s    (tree)     Visualizes bus hierarchy, MUXes & devices in an ASCII tree\n", COLOR_CYAN, COLOR_RESET);
    printf("  %si2cprobe%s   (probe)    Performs deep register diagnostics & fingerprint inspection\n", COLOR_CYAN, COLOR_RESET);
    printf("  %si2cbind%s    (bind)     Instantiates and binds a Linux kernel driver to an I2C device\n", COLOR_CYAN, COLOR_RESET);
    printf("  %si2cunbind%s  (unbind)   Unbinds/deletes a device from the Linux kernel\n\n", COLOR_CYAN, COLOR_RESET);
    printf("%sDirect Invocation:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  Commands can be called directly when symlinked (e.g., `i2cls 1` or `i2ctree`).\n\n");
    printf("Run '%s <command> --help' for command-specific options.\n", prog_name);
}

int main(int argc, char *argv[])
{
    g_enable_color = isatty(STDOUT_FILENO) && !getenv("NO_COLOR");
    i2c_db_init();

    /* Extract binary name to support symlinked multicall */
    char *base = basename(argv[0]);

    if (strcmp(base, "i2cls") == 0) {
        return cmd_ls_main(argc, argv);
    } else if (strcmp(base, "i2ctree") == 0) {
        return cmd_tree_main(argc, argv);
    } else if (strcmp(base, "i2cprobe") == 0) {
        return cmd_probe_main(argc, argv);
    } else if (strcmp(base, "i2cbind") == 0) {
        return cmd_bind_main(argc, argv);
    } else if (strcmp(base, "i2cunbind") == 0) {
        return cmd_unbind_main(argc, argv);
    }

    /* Dispatched via first argument */
    if (argc < 2) {
        print_general_help(argv[0]);
        return 0;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "ls") == 0 || strcmp(cmd, "i2cls") == 0) {
        return cmd_ls_main(argc - 1, &argv[1]);
    } else if (strcmp(cmd, "tree") == 0 || strcmp(cmd, "i2ctree") == 0) {
        return cmd_tree_main(argc - 1, &argv[1]);
    } else if (strcmp(cmd, "probe") == 0 || strcmp(cmd, "i2cprobe") == 0) {
        return cmd_probe_main(argc - 1, &argv[1]);
    } else if (strcmp(cmd, "bind") == 0 || strcmp(cmd, "i2cbind") == 0) {
        return cmd_bind_main(argc - 1, &argv[1]);
    } else if (strcmp(cmd, "unbind") == 0 || strcmp(cmd, "i2cunbind") == 0) {
        return cmd_unbind_main(argc - 1, &argv[1]);
    } else if (strcmp(cmd, "-h") == 0 || strcmp(cmd, "--help") == 0) {
        print_general_help(argv[0]);
        return 0;
    } else if (strcmp(cmd, "-v") == 0 || strcmp(cmd, "--version") == 0) {
        printf("i2c-tools-ng version %s (GPLv3)\n", I2CTOOL_VERSION);
        return 0;
    } else {
        fprintf(stderr, "%s[-] Unknown command '%s'.%s\n\n", COLOR_RED, cmd, COLOR_RESET);
        print_general_help(argv[0]);
        return 1;
    }
}
