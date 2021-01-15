//
// Created by blade on 13/01/2021.
//

#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include <math.h>

#define minima -2147483647                  //le int minimum sur ma machine

typedef enum { false, true } bool;         //false sera utilisé pour l'addition et true pour le max

struct tablo {
    int * tab;
    int size;
};

//pour valgrind : j'initie le tableau ce qui me permet de manipuler le tableau sans case à "NULL"
void prepatablo(struct tablo * tabEntree, struct tablo * reception, bool operateur){
    reception->size =tabEntree->size;
    reception->tab=malloc(reception->size*sizeof(int));         //operateur sert à mettre la valeur neutre pour l'opération à venir : 0 pour l'addition et l'int minimum pour "max"
    if (operateur == false) {                                       //n'est utile que pour le tableau b dans le scan préfixe
        for (int i = 0; i < reception->size; i++) {
            reception->tab[i] = 0;                                //on initialise toutes les cases des tableaux qu'on utilise
        }
    }else{
        for (int i = 0; i < reception->size; i++) {
            reception->tab[i] = minima;
        }
    }
}

int max(int x, int y){
    if (x>=y){
        return x;
    }else{
        return y;
    }
}

void printArray(struct tablo * tmp) {
    printf("---- Array of size %i ---- \n", tmp->size);
    int size = tmp->size;
    int i;
    for (i = 0; i < size; ++i) {
        printf("%i ", tmp->tab[i]);
    }
    printf("\n");
}

void montee(struct tablo * tabEntier, struct tablo * arbre1, bool operateur) {
    int mark = (int)(arbre1->size/2);

    for(int i =0;i<tabEntier->size;i++){
        arbre1->tab[mark]=tabEntier->tab[i];
        mark++;
    }

    for (int l = log2(tabEntier->size)-1;l>=0;l--){
        if (operateur==false){
            #pragma omp parallel for
            for (int j = pow(2,l); j<(int)pow(2,l+1);j++){
                arbre1->tab[j]=arbre1->tab[2*j]+arbre1->tab[2*j+1];
            }
        }else{
            #pragma omp parallel for
            for (int j = pow(2,l); j<(int)pow(2,l+1);j++){
                arbre1->tab[j]=max(arbre1->tab[2*j],arbre1->tab[2*j+1]);
            }
        }
    }
}


void descente(struct tablo * arbre1, struct tablo * arbre2, bool operateur) {
    for(int l =1;l<=(int)log2(arbre1->size/2);l++){
        if (operateur==false){                                              //placer le else ici me force à multiplier les lignes de code mais évite de répéter la condition à chaque boucle
            #pragma omp parallel for
            for (int j =pow(2,l);j<(int)pow(2,l+1);j++) {
                if (j % 2 == 0) {
                    arbre2->tab[j] = arbre2->tab[(int) j / 2];
                } else {
                    arbre2->tab[j] = arbre2->tab[(int) (j - 1) / 2] + arbre1->tab[j - 1];
                }
            }
        }else{
            #pragma omp parallel for
            for (int j =pow(2,l);j<(int)pow(2,l+1);j++) {
                if (j % 2 == 0) {
                    arbre2->tab[j] = arbre2->tab[(int) j / 2];
                } else {
                    arbre2->tab[j] = max(arbre2->tab[(int) (j - 1) / 2] , arbre1->tab[j - 1]);
                }
            }
        }
    }
}

void final(struct tablo * arbre1, struct tablo *arbre2, bool operateur) {
    int m= (int)log2(arbre1->size/2);
    if (operateur==false) {
        #pragma omp parallel for
        for (int j = pow(2, m); j < (int)pow(2,m+1); j++) {
            arbre2->tab[j] = arbre2->tab[j] + arbre1->tab[j];
        }
    }else{
        #pragma omp parallel for
        for (int j = pow(2, m); j < (int)pow(2,m+1); j++) {
            arbre2->tab[j] = max(arbre2->tab[j] , arbre1->tab[j]);
        }
    }
}

// méthode pour exécuter le scan préfixe (parralélisé en montée, descente et dans l'étape final)
//prend 2 tableau : le tableau d'entier entrant à scanner et le tableau recption qui contiendra le résultat
void scan_prefixe(struct tablo *tabEntree, struct tablo *reception, bool operateur){
    struct tablo * arbre1 = malloc(sizeof(struct tablo));
    tabEntree->size=tabEntree->size*2;
    prepatablo(tabEntree,arbre1,false);
    tabEntree->size=tabEntree->size/2;
    montee(tabEntree, arbre1, operateur);

    struct tablo * arbre2 = malloc(sizeof(struct tablo));
    tabEntree->size=tabEntree->size*2;
    prepatablo(tabEntree,arbre2,operateur);
    tabEntree->size=tabEntree->size/2;
    descente(arbre1, arbre2, operateur);

    final(arbre1,arbre2, operateur);

    int markdown = tabEntree->size;
    #pragma omp parallel for
    for (int i=0; i<reception->size;i++){
        reception->tab[i]=arbre2->tab[i+markdown];
    }

    free(arbre1->tab);
    free(arbre2->tab);
    free(arbre1);
    free(arbre2);
}

void reverse(struct tablo *tabEntree, struct tablo *reception){
    #pragma omp parallel for
    for (int i=0; i<tabEntree->size;i++){
        reception->tab[(tabEntree->size-1)-i]=tabEntree->tab[i];
    }
}

void make_ssum(struct tablo *tabEntree, struct tablo *reception, bool operateur){
    struct tablo * tempTabEntree = malloc(sizeof(struct tablo));
    prepatablo(tabEntree,tempTabEntree,false);
    struct tablo * temprecep = malloc(sizeof(struct tablo));
    prepatablo(tabEntree,temprecep,false);

    reverse(tabEntree,tempTabEntree);
    scan_prefixe(tempTabEntree,temprecep,operateur);
    reverse(temprecep,reception);
    free(tempTabEntree->tab);
    free(temprecep->tab);
    free(tempTabEntree);
    free(temprecep);
}

void step5(struct tablo *qtab, struct tablo *psumtab,struct tablo *ssumtab, struct tablo *smaxtab,struct tablo *pmaxtab,struct tablo *mtab){
    struct tablo * ms = malloc(sizeof(struct tablo));
    prepatablo(qtab,ms,false);
    struct tablo * mp = malloc(sizeof(struct tablo));
    prepatablo(qtab,mp,false);

    #pragma omp parallel for
    for (int i=0;i<qtab->size;i++){
        ms->tab[i]=pmaxtab->tab[i]-ssumtab->tab[i];
        mp->tab[i]=smaxtab->tab[i]-psumtab->tab[i]+qtab->tab[i];            //dans l'équation final q[i] n'est ajouté qu'une fois au final
        mtab->tab[i]=ms->tab[i]+mp->tab[i];
    }
}

void max_subArray(struct tablo *final,struct tablo *q, struct tablo *m){
    int indexMax=0;                             //indice qui me permet d'avoir le début de la séquence de max dans m
    final->tab[0]=m->tab[0];
    for (int i =1; i< m->size;i++){                     //la parallélisation de la boucle for amène parfois à un mauvais résultat
       // int thr = omp_get_thread_num();
     //   printf("thread numero %d \n", thr);
        if (m->tab[i]>final->tab[0]){
            final->tab[0]=m->tab[i];
            indexMax=i;
        //    printf("%d\n",indexMax);
        }else {
            if (m->tab[i] == final->tab[0] && i < indexMax) {              //à max égale on cherche aussi l'indice le plus faible
                indexMax = i;                                              //malgrès les threads qui donnent le résultat dans le désordre
            }
        }
    }
    int fin_indexMax=indexMax;                          //indice qui me permet d'avoir la fin de la séquence de max dans m
    int index_final=1;                                  //indice qui me permet de me positionner dans final
    for (int i=indexMax;i<m->size;i++){
        if(m->tab[i]==final->tab[0]){
            final->tab[index_final]=q->tab[i];
            index_final++;
            fin_indexMax=i;
        }else{break;}
    }
    final->size=fin_indexMax-indexMax+2;
}

// on considère que le nom d'un fichier sera toujours le seul paramètre passé au programme
int main(int argc, char **argv) {

    struct tablo q;
    int number;
    FILE *fileTest = fopen(argv[1], "r");
    if (fileTest != NULL) {
        fseek(fileTest, 0L, SEEK_END);
        q.size = ftell(fileTest);
        fseek(fileTest, 0L, SEEK_SET);
        int i = 0;
        q.tab = malloc(q.size * sizeof(int));
        while (fscanf(fileTest, "%d", &number) == 1) {
            q.tab[i] = number;
            i++;
        }
        q.size = i;
        fclose(fileTest);
    } else {
        exit(-1);
    }
    //printArray(&q);

    struct tablo *psum = malloc(sizeof(struct tablo));
    prepatablo(&q, psum, false);
    scan_prefixe(&q, psum, false);
    //printArray(psum);

    struct tablo *ssum = malloc(sizeof(struct tablo));
    prepatablo(&q, ssum, false);
    make_ssum(&q,ssum,false);
    //printArray(ssum);

    struct tablo * smax = malloc(sizeof(struct tablo));
    prepatablo(&q,smax,false);
    make_ssum(psum,smax,true);               //j'ai laissé make_ssum comme nom de méthode car le procédé est pareil
    //printArray(smax);

    struct tablo * pmax = malloc(sizeof(struct tablo));
    prepatablo(&q,pmax,false);
    scan_prefixe(ssum,pmax,true);
    //printArray(pmax);

    struct tablo * m = malloc(sizeof(struct tablo));
    prepatablo(&q,m,false);
    step5(&q,psum,ssum,smax,pmax,m);
    //printArray(m);

    struct tablo * final = malloc(sizeof(struct tablo));
    prepatablo(&q,final,false);
    max_subArray(final,&q,m);

    for (int i=0;i<final->size;i++){
        printf("%d ",final->tab[i]);
    }
}
