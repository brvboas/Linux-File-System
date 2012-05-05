/*
 * RSFS - Really Simple File System
 *
 * Copyright © 2010 Gustavo Maciel Dias Vieira
 * Copyright © 2010 Rodrigo Rocco Barbieri
 *
 * This file is part of RSFS.
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "disk.h"
#include "fs.h"

#define CLUSTERSIZE 4096

unsigned short fat[65536];

typedef struct {
       char used;
       char name[25];
       unsigned short first_block;
       int size;
} dir_entry;

dir_entry dir[128];

typedef struct {
       char open;
       int byte_atual;
       int cluster_atual;
       int mode;
} arq_open;

arq_open arq[128];

/* Funcao que inicia o sistema de arquivos
 * retorno: 1 caso inicie com sucesso
 *          0 caso contrario
 
Inicia o sistema de arquivos e suas estruturas internas. Esta função  ́é auto-
maticamente chamada pelo interpretador de comandos no início do sistema. Esta função
deve carregar dados do disco para restaurar um sistema já em uso é um bom momento
para verificar se o disco está ́formatado.
                            
 */
int fs_init() {
	int i=0;
	char *aux;
	aux = (char *) fat;

	for(i=0;i<128;i++)
	   arq[i].open = 0;

   /* Le a FAT do disco para a memoria */
  	for (i=0; i < 32; i++){
  	 	if(!fs_read_sector(i,&aux[SECTORSIZE*i]))
  	   		return 0;
   	}
  	/* Le diretorio do disco para a memoria */	
  	if (!(fs_read_sector(32,(char *) dir)))
  	   return 0;

 if(!fs_isFormated()){      
            fs_format();
    }
    	return 1;
	
}

/* Funcao que formata o disco
 * retorno: 1 caso a formatacao seja completada com sucesso
 *          0 caso contrario

             Inicia o dispositivo de disco para uso, iniciando e escrevendo as estruturas
de dados necessárias.

 */
int fs_format() {
  int i;
  /* inicializando a FAT*/
  /* inicializando os 32 agrupamentos da FAT */
  for (i = 0; i < 32; i++){
    fat[i]=3;
   }
  /* inicializando o agrupamento do diretorio */  
  fat[32]=4;

  /* inicializando os agrupamentos de arquivo como agrupamentos livres */
  for (i = 33; i < bl_size()/8; i++) {
    fat[i]=1; 
  }
  if(!fs_writeFat()){
    printf("\nErro na formatacao do disco.\n");
    return 0;
  }
  /* inicializando o diretorio */
  for (i = 0; i < 128; i++)
    dir[i].used = 0;     
  if(!fs_writeDir()){
    printf("\nErro na formatacao do disco.\n");
    return 0;
  }
  printf("\nO seu disco foi formatado! \n");  
  return 1;
}


/* Funcao que verifica o espaço livre no disco
 * retorno: espaco livre no disco em bytes (agrupamentos * CLUSTERSIZE)
 */
int fs_free() {
int cont,i;
cont=0;
	for (i=0;i<bl_size()/8;i++){
		if (fat[i] == 1)
			cont++;
	}
	return cont*CLUSTERSIZE;
 
}


/* Funcao que lista os arquivos do diretorio
 * retorno: 

Lista os arquivos do diretório, colocando a saída      
formatada em buffer. O formato é simples, um arquivo por linha, seguido de seu tamanho
e separado por dois tabs. Observe que a sua função não deve escrever na tela.

 */
int fs_list(char *buffer, int size) {
  int i;
  strcpy(buffer,"");
  for (i=0;i<128;i++){
     if(dir[i].used==1){
     	buffer += sprintf(buffer,"%s\t\t%d\n",dir[i].name,dir[i].size);
     }
  }
  return 1;
}


/* Funcao que cria um arquivo no disco
 * retorno: 1 se completar a operacao
 *          0 caso contrário

Cria um novo arquivo com nome file name e tamanho 0.
Um erro deve ser gerado se o arquivo já existe.

 */
int fs_create(char* file_name) {
	int i, p_dir=-1, p_fat=-1;
	/* verifica se o file_name recebido excede 24 caracteres */
	if(strlen(file_name)>=25){
		printf("O tamanho do nome excedeu o limite de 24 caracteres.\n");
		return 0;
	}	
	for (i=0; i<128; i++){
	    /* verifica se o nome de arquivo passado e um nome de arquivo ja existente */
		if((dir[i].used)&&(!strcmp(dir[i].name,file_name))){
			printf("Nome de arquivo ja existente.\n");
			return 0;
		}
		/* seta a variavel auxiliar com a primeira posicao livre no diretorio */
		if ((p_dir == -1) && (dir[i].used==0))
			p_dir = i;
	}
	/* verifica se o diretório não está cheio */
	if (p_dir == -1){
           printf("Nao e possivel criar o arquivo, o diretorio esta cheio.");
           return 0;
    	}
	dir[p_dir].used = 1;
	strcpy(dir[p_dir].name, file_name);
        p_fat = fs_ffree_cluster();
	dir[p_dir].first_block = p_fat;
	dir[p_dir].size = 0;
	if (!fs_writeDir()){
		printf("Erro ao escrever o diretorio");
		return 0;
	}	
	fat[p_fat]=2;
	if (!fs_writeFat()){
		printf("Erro ao escrever a Fat");
		return 0;
	}	
  	printf("Criado o arquivo: %s \n", file_name);
	return 1;
}


/* Funcao que remove um arquivo do disco
 * retorno: 1 se completar a operação
 *          0 caso contrario

Remove o arquivo com nome file name. Um erro deve
ser gerado se o arquivo não existe.

 */
int fs_remove(char *file_name) {
  int i, p_dir=-1, p_fat, p_prox;
  for(i=0;i<128;i++){
    if ((dir[i].used==1)&&(!strcmp(dir[i].name,file_name)))
    	p_dir=i;
  }
  if (p_dir==-1){
  	printf("Arquivo não existe!\n");
  	return 0;
  }
  p_fat = dir[p_dir].first_block;
  dir[p_dir].used=0;
  while (fat[p_fat]!=2){
     p_prox=fat[p_fat];
     fat[p_fat] = 3;
     p_fat = p_prox;
  }
  fat[p_fat] = 3;
	if (!fs_writeDir()){
		printf("Erro ao escrever o diretorio");
		return 0;
	}	
	if (!fs_writeFat()){
		printf("Erro ao escrever a Fat");
		return 0;
	}	
  printf("Arquivo apagado com sucesso!\n");
  return 1;
}


/* Funcao que escreve a fat no disco
 * retorno: 1 se completar a operacao
 *          0 caso contrario
 */
int fs_writeFat(){
	int i;
	char *aux;
        aux = (char*) fat;
	for (i=0; i<32; i++){
		if (!fs_write_sector(i, &aux[CLUSTERSIZE*i]))
			return 0;
	}
	return 1;		
}


/* Funcao que escreve o diretorio no disco
 * retorno: 1 se completar a operacao
 *          0 caso contrario
 */
int fs_writeDir(){
    if (!fs_write_sector(32, (char *)dir))
		return 0;
	return 1;
}

/* encontra o primeiro agrupamento livre na fat 
 * retorno: posição do primeiro agrupamento livre
 *          -1 se todos agrupamento estiverem ocupados 
 */
int fs_ffree_cluster(){ 
    int i = 33; /* primeira posicao que pode conter um arquivo */
    if (!fs_free())
        return -1;
    while (fat[i] != 1 && i < bl_size()/8)
        i++;
    return i;
}


/* função que verifica se o disco esta formatado 
 * retorno: 1 se estiver formatado
 *          0 caso contrario
 */
int fs_isFormated(){
    int i;
    /* verifica se os agrupamentos da FAT estao marcados como tal */
    for (i=0; i< 32; i++)
        if (fat[i] != 3)
            return 0;
    /* verifica se o agrupamento do diretorio esta marcado como tal */            
    if (fat[32] != 4)
        return 0;
    return 1;
}

/* função que escreve o agrupamento passado nos setores do disco
 * entrada: cluster: inteiro equivalente ao agrupamento a ser escrito
 			buffer: cadeia de caracteres a ser escrita no disco
 * retorno: a posicao do arquivo no diretorio se existir
 *          -1 caso contrario
 */
int fs_write_sector(int cluster, char *buffer){
	int i;
	for(i=0;i<8;i++){
		if(!bl_write((cluster*8)+i,&buffer[SECTORSIZE*i]))
			return 0;
	}
	return 1;		
}


/* função que lê os setores do disco 
 * entrada: cluster: inteiro equivalente ao agrupamento a ser lido
 			buffer: cadeia de caracteres a ser escrita no disco
 * retorno: a posicao do arquivo no diretorio se existir
 *          -1 caso contrario
 */
int fs_read_sector(int cluster, char *buffer){
	int i;
	for (i = 0; i < 8; i++){
		if (!bl_read((cluster*8)+i, &buffer[SECTORSIZE*i]))
			return 0;
	}
	return 1;		
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
Abre o arquivo file name para leitura (se mode
for FS_R = 0) ou escrita (se mode for FS_W = 1). Ao abrir um arquivo para leitura, um erro deve
ser gerado se o arquivo não existe. Ao abrir um arquivo para escrita, o arquivo deve ser
criado ou um arquivo pré-existente deve ser apagado e criado novamente com tamanho 0.
Retorna o identificador do arquivo aberto (um inteiro) ou -1 em caso de erro.

*/
int fs_open(char *file_name, int mode) {
  int i, p_dir=-1;
  for(i=0;i<128;i++){
    if ((dir[i].used==1)&&(!strcmp(dir[i].name,file_name)))
    	p_dir=i;
  }
  if(mode==0){
	  if (p_dir==-1){
	  	printf("Arquivo não existe!\n");
	  	return -1;
	  }
	  arq[p_dir].open = 1;
	  arq[p_dir].byte_atual = 0;
          arq[p_dir].cluster_atual = dir[p_dir].first_block;	
          arq[p_dir].mode = mode;	  
	  return p_dir;
  }
  if(mode==1){
  	if(p_dir!=-1)
  	  fs_remove(file_name);
  	if(!fs_create(file_name))
  	  return -1;
  }
  p_dir = -1;
  for(i=0;i<128;i++){
    if ((dir[i].used==1)&&(!strcmp(dir[i].name,file_name)))
    	p_dir=i;
  }
  if(p_dir != -1){
    arq[p_dir].open = 1;
    arq[p_dir].byte_atual = 0;
    arq[p_dir].cluster_atual = dir[p_dir].first_block;	
    arq[p_dir].mode = mode;	  	  
  }
  return p_dir;
}

/*
Fecha o arquivo dado pelo identificador de arquivo file. Um erro
deve ser gerado se não existe arquivo aberto com este identificador.
*/

int fs_close(int file)  {
  if (file==-1){
  	printf("Arquivo não existe!\n");
  	return 0;
  }
  fat[arq[file].cluster_atual]=2;	
  arq[file].open = 0;
  printf("Arquivo fechado!\n");
  return 1;
}

/*
Escreve size bytes do buffer no arquivo aberto com identificador file. Retorna quantidade de bytes escritos (0 se não
escreveu nada, -1 em caso de erro). Um erro deve ser gerado se não existe arquivo aberto
com este identificador ou caso o arquivo tenha sido aberto para leitura.

*/
int fs_write(char *buffer, int size, int file) {
  char aux[CLUSTERSIZE], *aux2;
  int k=0, p_fat, j=0, i=0;
  int byte_inicial, cluster_inicial, bytes_escrever=size, bytes_escritos=0;
  if ((arq[file].mode == 1) && (arq[file].open == 1)){
	byte_inicial = arq[file].byte_atual;
	cluster_inicial = arq[file].cluster_atual;
 	while (bytes_escrever != bytes_escritos){ //enquanto não tiver escrito size bytes
		if (((k=arq[file].byte_atual%CLUSTERSIZE)==0)&&(dir[file].size!=0)){ // se o cluster atual está cheio procura um novo cluster 
			if((p_fat=fs_ffree_cluster())==-1){
				printf("Não há mais espaço na fat!\n");
				dir[file].size +=bytes_escritos;	
				if (!fs_writeDir()){
					printf("Erro ao escrever o diretorio");
					return 0;
				}	
				if (!fs_writeFat()){
					printf("Erro ao escrever a Fat");
					return 0;
				}	
				return bytes_escritos;
			}
			fat[arq[file].cluster_atual]=p_fat;
			fat[p_fat]=2;
	} 
		// se ja tem dados no cluster atual
		if ((k!=0)&& (byte_inicial!=0)){
			// le o cluster atual inteiro para aux
			fs_read_sector(arq[file].cluster_atual, aux);
			//concatena aux com buffer em aux2
			// nao funciona
	     	aux2 += sprintf(aux, buffer);
			bytes_escrever = strlen(aux2) - k;
			j+= bytes_escrever;

		}
		else {
			if (dir[file].size == 0){	
				strcpy(aux2, buffer);
				bytes_escrever = strlen(aux2) - k;

			}
		}		
		fs_write_sector(arq[file].cluster_atual, &aux2[bytes_escritos]);
		fs_read_sector(arq[file].cluster_atual, aux);
		i++;
		bytes_escritos += strlen(aux)-bytes_escritos-k;
	}
	dir[file].size +=bytes_escritos;
	arq[file].byte_atual+=bytes_escritos;
	if (!fs_writeDir()){
		printf("Erro ao escrever o diretorio");
		return 0;
	}	
	if (!fs_writeFat()){
		printf("Erro ao escrever a Fat");
		return 0;
	}	
	return bytes_escritos; 	
  }
  if (arq[file].open == 0)
  	printf("Arquivo não está aberto!\n");
  else
  	printf("Erro: Arquivo aberto apenas para leitura!\n");
  return -1;}

/* 
     Lê no máximo size bytes no buffer do
     arquivo aberto com identificador file. Retorna quantidade de bytes efetivamente lidos (0
     se não leu nada, o que indica que o arquivo terminou, -1 em caso de erro). Um erro deve
     ser gerado se não existe arquivo aberto com este identificador ou caso o arquivo tenha
     sido aberto para escrita.

*/
int fs_read(char *buffer, int size, int file) {
  int  bytes_ler=size, bytes_lidos=0;
  if ((arq[file].mode == 0) && (arq[file].open == 1)){
	  if(size <= arq[file].byte_atual - dir[file].size)// le ate size bytes
	   	bytes_ler = strlen(buffer);
	  else // le ate o fim do arquivo
	  	 bytes_ler = dir[file].size;
	  while (bytes_lidos <  bytes_ler){
		if (buffer == NULL)
			break;
	  	fs_read_sector(arq[file].cluster_atual, &buffer[bytes_lidos]);
		bytes_lidos = strlen(buffer);
	  }
		if (!fs_writeDir()){
			printf("Erro ao escrever o diretorio");
			return 0;
		}	

		if (!fs_writeFat()){
			printf("Erro ao escrever a Fat");
			return 0;
		}	
	  return bytes_lidos;
  }	
  if (arq[file].open == 0)
  	printf("Arquivo não está aberto!\n");
  else
  	printf("Erro: Arquivo aberto apenas para escrita!\n");
  return -1;
}




