#include "mem.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>

struct fb{
	size_t size;
	struct fb *next;
};

struct fb *tete_fb;
mem_fit_function_t *mem_fit_function;

/*initialise la liste des zones libres avec une seule zone corerespondant à l'ensemble de la mémoire située en mem de taille taille*/
void mem_init(char* mem, size_t taille){
	tete_fb = (struct fb*)mem;
	tete_fb->size = taille;
	tete_fb->next = NULL;

	mem_fit(mem_fit_first);
}

void* mem_alloc(size_t taille){
	struct fb *fb_dispo, *fb_dispo_prec, *nouv_fb;
	size_t *resultat;
	char *nouv_fb_deplace;
	fb_dispo = mem_fit_function(tete_fb, taille+sizeof(size_t));  //le bloc libre qui peut etre alloué
	if(fb_dispo != NULL){

		size_t nouv_fb_taille = (fb_dispo->size)-taille-sizeof(size_t);
		if( nouv_fb_taille > sizeof(struct fb*)){ //la possibilité d'allouer encore un bloc dans la mémoire
			nouv_fb_deplace = (char *)fb_dispo;
			nouv_fb_deplace = nouv_fb_deplace+taille+sizeof(size_t);
			nouv_fb = (struct fb*)nouv_fb_deplace;

			//mettre à jour la liste de fb
			if(fb_dispo == tete_fb){
				tete_fb = nouv_fb;
				tete_fb->size = nouv_fb_taille;
				tete_fb->next = fb_dispo->next;
			}else{
				fb_dispo_prec = tete_fb;
				while(fb_dispo_prec->next != fb_dispo){
					fb_dispo_prec = fb_dispo_prec->next;
				}
				fb_dispo_prec->next = (struct fb*)nouv_fb;
				nouv_fb->size = nouv_fb_taille;
				nouv_fb->next = fb_dispo->next;
			}
		}else{
			taille = taille + nouv_fb_taille; //pas de nouveau free block
			if(fb_dispo == tete_fb){
				tete_fb = tete_fb->next;
			}else{
				fb_dispo_prec = tete_fb;
				while(fb_dispo_prec->next != fb_dispo){
					fb_dispo_prec = fb_dispo_prec->next;
				}
				fb_dispo_prec->next = fb_dispo->next;
			}
		}
		resultat = (size_t *)fb_dispo;
		*resultat = taille; //mettre la taille donnée à l'utilisateur
		resultat++;  		//decaler 8 octet et puis lui donner le debut de la mémoire allouée
		return resultat;
	}else
		return NULL; 
}

void mem_free(void* zone){
	struct fb *zone_libre, *zone_libre_next, *zone_libre_prec;
	size_t *taille;
	char *fin_zone_libre, *fin_zone_libre_prec;

	taille = (size_t *)zone-1;
	zone_libre = (struct fb*)taille;
	fin_zone_libre = (char *)taille + sizeof(size_t) +*taille;
	
	//trier la liste chainee
	if(zone_libre<tete_fb){ //zone_libre devient la tête de liste fb
		if(fin_zone_libre == (char *)tete_fb){
			zone_libre->size = *taille + sizeof(size_t) + tete_fb->size;
			zone_libre->next = tete_fb->next;
			tete_fb = zone_libre;
		}else{
			zone_libre->size = *taille+sizeof(size_t);
			zone_libre->next = tete_fb;
			tete_fb = zone_libre;
		}
	}else{ //trouver la bonne place dans la liste fb
		zone_libre_next = tete_fb;
		while(zone_libre_next != NULL && zone_libre_next<zone_libre){
			zone_libre_next = zone_libre_next->next;
		}
		zone_libre_prec = tete_fb;
		while(zone_libre_prec != NULL && zone_libre_prec->next != zone_libre_next){
			zone_libre_prec = zone_libre_prec->next;
		}

		//Tester si on peut faire une diffusion entre zone_libre et zone_libre->next
		if(fin_zone_libre == (char *)zone_libre_next){
			zone_libre->size = *taille + sizeof(size_t) + zone_libre_next->size;
			zone_libre->next = zone_libre_next->next;
		}else{
			zone_libre->size = *taille + sizeof(size_t);
			zone_libre->next = zone_libre_next;
		}

		//Tester si on peut faire une diffusion entre zone_libre et zone_libre->prec
		fin_zone_libre_prec = (char *)zone_libre_prec;
		fin_zone_libre_prec+=zone_libre_prec->size;
		if(fin_zone_libre_prec == (char *)zone_libre){
			zone_libre_prec->size = *taille + sizeof(size_t) + zone_libre_prec->size;
		}else{
			zone_libre_prec->next = zone_libre;
		}
	}

}

size_t mem_get_size(void * zone){
	size_t *taille = (size_t *)zone-1;
	return *taille;
}

/* Itérateur sur le contenu de l'allocateur */
void mem_show(void (*print)(void *, size_t, int free)){
	char  *offset = (char *)get_memory_adr();
	struct fb *zone_libre = tete_fb;
	size_t *zone_occupee;
	//parcourir l'ensemble de blocs
	while(offset < (char *)get_memory_adr()+get_memory_size()){
		if(offset != (char *)zone_libre){
			zone_occupee = (size_t *)offset+1;
			print(zone_occupee, mem_get_size(zone_occupee), 0);
			offset+=mem_get_size(zone_occupee)+sizeof(size_t);
		}else{
			print(zone_libre, zone_libre->size, 1);
			offset += zone_libre->size;
			zone_libre = zone_libre->next;
		}
	}
}

void mem_fit(mem_fit_function_t *ptr_mem_fit_function){
	mem_fit_function = ptr_mem_fit_function;
}

struct fb* mem_fit_first(struct fb* list, size_t size){
	struct fb *resultat = list;
	while(resultat != NULL && resultat->size < size){
		resultat = resultat->next;
	}
	return resultat;
}