/******************************************************************************
 * Laboratoire 5
 * GIF-3004 Systèmes embarqués temps réel
 * Hiver 2024
 * Marc-André Gardner
 * 
 * Fichier implémentant les fonctions de l'emulateur de clavier
 ******************************************************************************/

#include "emulateurClavier.h"
#define TAILLE_PAQUET_USB 8
#define MODIFICATEUR_AUCUN 0x00
#define MODIFICATEUR_SHIFT 0x02
FILE* initClavier(){
    printf("emulateurClavier:initClavier : entered \n");
    // Deja implementee pour vous
    FILE* f = fopen(FICHIER_CLAVIER_VIRTUEL, "wb");
    setbuf(f, NULL);        // On desactive le buffering pour eviter tout delai
    return f;
}

int asciiToHid(char c) {
    printf("emulateurClavier:asciiToHid : entered \n");
    if (c >= 'a' && c <= 'z') return 4 + (c - 'a');
    if (c >= 'A' && c <= 'Z') return 4 + (c - 'A');
    if (c >= '1' && c <= '9') return 30 + (c - '1');
    if (c == '0') return 39;
    if (c == '.') return 55;
    if (c == ',') return 54;
    if (c == ' ') return 44;
    if (c == '\n') return 40;
    return 0; 
}

int ecrireCaracteres(FILE* periphClavier, const char* caracteres, size_t len, unsigned int tempsTraitementParPaquetMicroSecondes){
    printf("emulateurClavier:ecrireCaracteres : entered \n");
    // TODO ecrivez votre code ici. Voyez les explications dans l'enonce et dans
    // emulateurClavier.h
    unsigned char paquet[TAILLE_PAQUET_USB] = {MODIFICATEUR_AUCUN, 0}; 
    size_t i = 0;
    int nbCaracteresDansPaquet = 0;

    // Boucle sur chaque caractère de la chaîne.
    for (i = 0; i < len; i++) {
        
        char c = caracteres[i];
        int codeHID = asciiToHid(c);
        printf("emulateurClavier: Char lu : %c , Code HID : %i \n", c, codeHID);

        if (isupper(c)) {
            paquet[0] = MODIFICATEUR_SHIFT; 
        }

        if (codeHID != 0) {
            paquet[2 + nbCaracteresDansPaquet++] = codeHID;
        }

        if (nbCaracteresDansPaquet == 6 || i == len - 1) {
            fwrite(paquet, sizeof(unsigned char), TAILLE_PAQUET_USB, periphClavier);
            usleep(tempsTraitementParPaquetMicroSecondes);
            memset(paquet, 0, TAILLE_PAQUET_USB);
            fwrite(paquet, sizeof(unsigned char), TAILLE_PAQUET_USB, periphClavier);
            usleep(tempsTraitementParPaquetMicroSecondes);
            memset(paquet, 0, TAILLE_PAQUET_USB);
            paquet[0] = isupper(caracteres[i + 1]) ? MODIFICATEUR_SHIFT : MODIFICATEUR_AUCUN;
            nbCaracteresDansPaquet = 0;
        }
    }

    return len; 


}