/******************************************************************************
 * Laboratoire 5
 * GIF-3004 Systèmes embarqués temps réel
 * Hiver 2024
 * Marc-André Gardner
 * 
 * Fichier implémentant les fonctions de l'emulateur de clavier
 ******************************************************************************/

#include <stdbool.h>
#include "emulateurClavier.h"
#define TAILLE_PAQUET_USB 8
u_int8_t MODIFICATEUR_AUCUN =  0x00;
u_int8_t MODIFICATEUR_SHIFT = 0x02;

FILE* initClavier(){
    printf("emulateurClavier:initClavier : entered \n");
    // Deja implementee pour vous
    FILE* f = fopen(FICHIER_CLAVIER_VIRTUEL, "wb");
    setbuf(f, NULL);        // On desactive le buffering pour eviter tout delai
    return f;
}

int asciiToHid(char c) {
    //printf("emulateurClavier:asciiToHid : entered \n");
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

struct charToSendHID{int HidCode; u_int8_t toucheModif;};

int ecrireCaracteres(FILE* periphClavier, const char* caracteres, size_t len, unsigned int tempsTraitementParPaquetMicroSecondes){
    //printf("emulateurClavier:ecrireCaracteres : entered, len = %zu \n", len);
    // TODO ecrivez votre code ici. Voyez les explications dans l'enonce et dans
    // emulateurClavier.h
    
    struct charToSendHID arrayCharToSend[len];

    for(size_t i = 0; i < len; i++)
    {
        
        
        arrayCharToSend[i].HidCode = asciiToHid(caracteres[i]);

        if(isupper(caracteres[i]))
        {
            arrayCharToSend[i].toucheModif = MODIFICATEUR_SHIFT;
        }
        else
        {
            arrayCharToSend[i].toucheModif = MODIFICATEUR_AUCUN;
        }
    }

    u_int8_t currentShiftState = arrayCharToSend[0].toucheModif;
    bool isThereAPacket = false;
    u_int8_t paquet[TAILLE_PAQUET_USB] = {0};
    int packetIndex = 0;
    int counter = 0;
    //on fait en sorte d'envoyer de maniere continue les packets lorsqu'ils ont le meme modificateur au debut.

    for(size_t i = 0; i < len; i++)
    {
        
        //printf("char : %c | HID : %i | shift : %u \n", caracteres[i], arrayCharToSend[i].HidCode, arrayCharToSend[i].toucheModif) ;


        
        if(isThereAPacket == false)
        {

            for (size_t j = 3; j < TAILLE_PAQUET_USB; j++) {
                paquet[j] = 0; //reinit a 0 de 3 a 7
            }
            
            currentShiftState = (u_int8_t)arrayCharToSend[i].toucheModif; //sans cast, le type change bizarrement
            paquet[0] = currentShiftState;
            paquet[1] = 0;
            paquet[2] = arrayCharToSend[i].HidCode;
            packetIndex = 2;
            counter = 1;
            isThereAPacket = true;
            //printf("New packet : arrayCharToSend.toucheModif is %u currenShiftState is : %u\n", (u_int8_t)arrayCharToSend[i+1].toucheModif, (u_int8_t)currentShiftState);
        }

        else if(isThereAPacket == true)
        {
            packetIndex++;
            paquet[packetIndex] = arrayCharToSend[i].HidCode;
            counter++;
        }

        //on envoie le packet si le prochain a un shiftState différent ou si on est au dernier index
        //printf("Checking packet state arrayCharToSend.toucheModif is %u currenShiftState is : %u\n", (u_int8_t)arrayCharToSend[i+1].toucheModif, (u_int8_t)currentShiftState);
        if(i == (len-1) || (counter >= 6))
            {
                sendPacket(periphClavier, paquet, tempsTraitementParPaquetMicroSecondes);
                isThereAPacket = false;
                counter = 0;
                //printf("Packet sent ! reason : len or counter \n");

            }

        else if ((u_int8_t)arrayCharToSend[i+1].toucheModif != (u_int8_t)currentShiftState)
            {
                sendPacket(periphClavier, paquet, tempsTraitementParPaquetMicroSecondes);
                isThereAPacket = false;
                counter = 0;
                //printf("Packet sent ! reason : arrayCharToSend.toucheModif was %u currenShiftState was : %u\n", (u_int8_t)arrayCharToSend[i+1].toucheModif, (u_int8_t)currentShiftState);
            }


    }
    

    return len;
}

void sendPacket(FILE* peripherique, u_int8_t paquet[8], unsigned int tempsTraitementParPaquetMicroSecondes)
{

    for (size_t i = 0; i < 8; i++) {
        printf("%u ", paquet[i]);
    }
    printf("\n");


    fwrite(paquet, sizeof(u_int8_t), TAILLE_PAQUET_USB, peripherique);
    //on envoie une trame avec des 0 pour relâcher la touche
    static u_int8_t paquet_zeros[TAILLE_PAQUET_USB] = {0};
    fwrite(paquet_zeros, sizeof(u_int8_t), TAILLE_PAQUET_USB, peripherique);
    usleep(tempsTraitementParPaquetMicroSecondes);
    
}



