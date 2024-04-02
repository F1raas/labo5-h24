/******************************************************************************
 * Laboratoire 5
 * GIF-3004 Systèmes embarqués temps réel
 * Hiver 2024
 * Marc-André Gardner
 * 
 * Fichier principal
 ******************************************************************************/

#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "utils.h"
#include "emulateurClavier.h"
#include "tamponCirculaire.h"

#define ASCII_EOT 0x04
#define BUFFER_TAILLE 256



static void* threadFonctionClavier(void* args){
    printf("main:threadFonctionClavier : entered \n");
    // Implementez ici votre fonction de thread pour l'ecriture sur le bus USB
    // La premiere des choses est de recuperer les arguments (deja fait pour vous)
    struct infoThreadClavier *infos = (struct infoThreadClavier *)args;

    // Vous devez ensuite attendre sur la barriere passee dans les arguments
    // pour etre certain de commencer au meme moment que le thread lecteur

    // TODO
    
    // on cree notre requete tampon
    struct requete req;
    req.taille = 0;
    printf("main:threadFonctionClavier : malloc \n");
    req.data = (char*) malloc(sizeof(char)* 256);
    printf("main:threadFonctionClavier : entering barrier \n");
    pthread_barrier_wait(infos->barriere);

    // Finalement, ecrivez dans cette boucle la logique du thread, qui doit:
    // 1) Tenter d'obtenir une requete depuis le tampon circulaire avec consommerDonnee()
    // 2) S'il n'y en a pas, attendre un cours laps de temps (par exemple usleep(500))
    // 3) S'il y en a une, appeler ecrireCaracteres avec les informations requises
    // 4) Liberer la memoire du champ data de la requete avec la fonction free(), puisque
    //      la requete est maintenant terminee
    printf("main:threadFonctionClavier : entering while \n");
    while(1){
       // TODO
       if(consommerDonnee(&req) == 0)
       {
            usleep(500);
       }
       else
       {
            ecrireCaracteres(infos->pointeurClavier, req.data, req.taille, infos->tempsTraitementParCaractereMicroSecondes);
            printf("main:threadFonctionClavier : Freeing \n");
            free(req.data); //entraîne des segfaults????
       }
    }
    return NULL;
}

static void* threadFonctionLecture(void *args){

    // Implementez ici votre fonction de thread pour la lecture sur le named pipe
    // La premiere des choses est de recuperer les arguments (deja fait pour vous)
    struct infoThreadLecture *infos = (struct infoThreadLecture *)args;
    
    // Ces champs vous seront utiles pour l'appel a select()
    fd_set setFd;
    
    int nfds = infos->pipeFd + 1;

    // Vous devez ensuite attendre sur la barriere passee dans les arguments
    // pour etre certain de commencer au meme moment que le thread lecteur
    pthread_barrier_wait(infos->barriere);
    // TODO

    // Finalement, ecrivez dans cette boucle la logique du thread, qui doit:
    // 1) Remplir setFd en utilisant FD_ZERO et FD_SET correctement, pour faire en sorte
    //      d'attendre sur infos->pipeFd
    // 2) Appeler select(), sans timeout, avec setFd comme argument de lecture (on veut bien
    //      lire sur le pipe)
    // 3) Lire les valeurs sur le named pipe
    // 4) Si une de ses valeurs est le caracteres ASCII EOT (0x4), alors c'est la fin d'un
    //      message. Vous creez alors une nouvelle requete et utilisez insererDonnee() pour
    //      l'inserer dans le tampon circulaire. Notez que le caractere EOT ne doit PAS se
    //      retrouver dans le champ data de la requete! N'oubliez pas egalement de donner
    //      la bonne valeur aux champs taille et tempsReception.
    struct requete req;
    req.data = malloc(sizeof(char) * BUFFER_TAILLE);
    if (!req.data) {
        perror("Erreur d'allocation mémoire");
        return NULL;
    }
     while(1) {
        FD_ZERO(&setFd);
        FD_SET(infos->pipeFd, &setFd);

        if (select(nfds, &setFd, NULL, NULL, NULL) == -1) {
            perror("Erreur lors de l'appel à select");
            free(req.data);
            return NULL;
        }

        if (FD_ISSET(infos->pipeFd, &setFd)) {
            ssize_t bytesRead;
            char buf;
            req.taille = 0;

            while((bytesRead = read(infos->pipeFd, &buf, 1)) > 0) {
                if (buf == ASCII_EOT) {
                    req.tempsReception = get_time();

                    insererDonnee(&req);
                    req.taille = 0; // Réinitialiser pour la prochaine requête
                    memset(req.data, 0, BUFFER_TAILLE); 
                } else {
                    if (req.taille < BUFFER_TAILLE - 1) { // Laisser un espace pour '\0'
                        req.data[req.taille++] = buf;
                    }
                }
            }

            if (bytesRead == -1) {
                perror("Erreur de lecture du named pipe");
                free(req.data);
                return NULL;
            } else if (bytesRead == 0) {
                break;
            }
        }
    }

    free(req.data);
    return NULL;
}

int main(int argc, char* argv[]){
    printf("main:main : entered \n");
    if(argc < 4){
        printf("Pas assez d'arguments! Attendu : ./emulateurClavier cheminPipe tempsAttenteParPaquet tailleTamponCirculaire\n");
    }
    // A ce stade, vous pouvez considérer que:
    // argv[1] contient un chemin valide vers un named pipe
    // argv[2] contient un entier valide (que vous pouvez convertir avec atoi()) representant le nombre de microsecondes a
    //      attendre a chaque envoi de paquet
    // argv[3] contient un entier valide (que vous pouvez convertir avec atoi()) contenant la taille voulue pour le tampon
    //      circulaire

    // Vous avez plusieurs taches d'initialisation a faire :
    //
    // 1) Ouvrir le named pipe
    // TODO
    char namedPipe[100];
    strcpy(namedPipe, argv[1]);
    uint waitingTime = atoi(argv[2]);
    uint sizeTampon = atoi(argv[3]);
    int pipeFd = open(namedPipe, O_RDWR);
    if(pipeFd < 0)
    {
        perror("main : erreur pipe open \n");
        return EXIT_FAILURE;
    }

    FILE* peripheriqueClavier = initClavier();
    if(peripheriqueClavier == NULL)
    {
        printf("main : erreur initClavier \n");
        exit(EXIT_FAILURE);
    }
    
    

    

    // 2) Declarer et initialiser la barriere
    
    // TODO

    pthread_barrier_t barriere;
    pthread_barrier_init(&barriere, NULL, 2);

    // 3) Initialiser le tampon circulaire avec la bonne taille

    // TODO

    initTamponCirculaire(sizeTampon);

    // 4) Creer et lancer les threads clavier et lecteur, en leur passant les bons arguments dans leur struct de configuration respective
    
    // TODO
    struct infoThreadLecture args_ecr = {peripheriqueClavier, waitingTime, &barriere};
    struct infoThreadClavier args_lect = {pipeFd, &barriere};
    
    pthread_t lect;
    pthread_t ecr;
    pthread_create(&lect, NULL, threadFonctionLecture, (void*) &args_lect);
    pthread_create(&ecr, NULL, threadFonctionClavier, (void*)&args_ecr);
    
    

    // La boucle de traitement est deja implementee pour vous. Toutefois, si vous voulez eviter l'affichage des statistiques
    // (qui efface le terminal a chaque fois), vous pouvez commenter la ligne afficherStats().
    struct statistiques stats;
    double tempsDebut = get_time();
    while(1){
        // Affichage des statistiques toutes les 2 secondes
        calculeStats(&stats);
        //afficherStats((unsigned int)(round(get_time() - tempsDebut)), &stats);
        resetStats();
        usleep(2e6);
    }
    return 0;
}