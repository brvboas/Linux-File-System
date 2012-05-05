/*
 * RSFS - Really Simple File System
 *
 * Copyright © 2010 Gustavo Maciel Dias Vieira
 * Copyright © 2010 Rodrigo Rocco Barbieri
 *
 * This file is par t of RSFS.
 *
 * RSFS is free software: you can redistribute it and/or modify
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define FS_R 0
#define FS_W 1
/* Funcoes de gerencia de arquivos */
int fs_init();
int fs_format();
int fs_free();
int fs_list(char *buffer, int size);
int fs_create(char *file_name);
int fs_remove(char *file_name);

/* Funcoes auxiliares */
int fs_writeFat();
int fs_writeDir();
int fs_ffree_cluster();
int fs_isFormated();
int fs_fileE(char *file_name);
int fs_write_sector(int cluster, char *buffer);
int fs_read_sector(int cluster, char *buffer);

/* Funcoes de escrite e leitura de arquivos */
int fs_open(char *file_name, int mode);
int fs_close(int file);
int fs_write(char *buffer, int size, int file);
int fs_read(char *buffer, int size, int file);
