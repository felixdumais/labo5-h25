/******************************************************************************
 * Laboratoire 5
 * GIF-3004 Systèmes embarqués temps réel
 * Hiver 2025
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


static void* threadFonctionClavier(void* args){
    // Implementez ici votre fonction de thread pour l'ecriture sur le bus USB
    // La premiere des choses est de recuperer les arguments (deja fait pour vous)
    struct infoThreadClavier *infos = (struct infoThreadClavier *)args;

    // Vous devez ensuite attendre sur la barriere passee dans les arguments
    // pour etre certain de commencer au meme moment que le thread lecteur

    // TODO
    pthread_barrier_wait(infos->barriere);

    // Finalement, ecrivez dans cette boucle la logique du thread, qui doit:
    // 1) Tenter d'obtenir une requete depuis le tampon circulaire avec consommerDonnee()
    // 2) S'il n'y en a pas, attendre un cours laps de temps (par exemple usleep(500))
    // 3) S'il y en a une, appeler ecrireCaracteres avec les informations requises
    // 4) Liberer la memoire du champ data de la requete avec la fonction free(), puisque
    //      la requete est maintenant terminee

    struct requete req;
    memset(&req, 0, sizeof(struct requete));  // Met tous les champs à zéro

    while(1){
       // TODO
        if(!consommerDonnee(&req))
        {
            usleep(500);
            continue;
        }

        ecrireCaracteres(infos->pointeurClavier, req.data, req.taille, infos->tempsTraitementParCaractereMicroSecondes);
        free(req.data);
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

    // TODO
    pthread_barrier_wait(infos->barriere);

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
    char* buffer;

    while(1){
        // TODO
        FD_ZERO(&setFd);
        FD_SET(infos->pipeFd, &setFd);

        // Attendre qu'il y ait quelque chose à lire
        if (select(nfds, &setFd, NULL, NULL, NULL) > 0) {
            if (FD_ISSET(infos->pipeFd, &setFd)) {
                // Lire le contenu du named pipe
                int tailleContenu;
                int lectureTotale = 0;
                int lectureCourante;

                if (read(infos->pipeFd, tailleContenu, sizeof(size_t)) != sizeof(size_t)) {
                    perror("Erreur lors de la lecture de la taille du contenu depuis le pipe");
                    close(infos->pipeFd);
                    continue; 
                }

                req.data = malloc(tailleContenu);
                buffer = malloc(tailleContenu);
                if (req.data == NULL) {
                    perror("Erreur d'allocation de mémoire");
                    exit(1);
                }

                while (lectureTotale < tailleContenu) {
                    lectureCourante = read(infos->pipeFd, buffer + lectureTotale, tailleContenu - lectureTotale);
                    if (lectureCourante == -1) {
                        perror("Erreur lors de la lecture du contenu depuis le pipe");
                        free(buffer);
                        lectureCourante = 0;
                        break;
                    }
                    lectureTotale += lectureCourante;
                }

                int last_message_starting_position = 0;
                if (lectureTotale > 0) {
                    for (int i = 0; i < lectureTotale; i++) {
                        if (buffer[i] == 0x04) { // ASCII EOT, fin du message
                            req.taille = i - last_message_starting_position + 1;
                            req.data = malloc(req.taille);
                            if (req.data != NULL && i > 0) {
                                memcpy(req.data, buffer, i);  // Copier le message sans le EOT
                                req.tempsReception = time(NULL);
                                insererDonnee(&req); // Insérer dans le tampon circulaire
                            }
                            
                            // Réinitialiser la requête pour le prochain message
                            req.taille = 0;
                            req.tempsReception = time(NULL);
                            last_message_starting_position = i;
                        }
                        // } else if (req.taille < sizeof(req.data) - 1) {
                        //     req.data[req.taille++] = buffer[i]; // Ajouter au buffer de la requête
                        // }
                    }
                } else if (lectureTotale == 0) {
                    // Fermeture du pipe détectée (écriture fermée)
                    break;
                }
            }
        }
    }
    return NULL;
}

int main(int argc, char* argv[]){
    if(argc < 4){
        printf("Pas assez d'arguments! Attendu : ./emulateurClavier cheminPipe tempsAttenteParPaquet tailleTamponCirculaire\n");
    }

    // A ce stade, vous pouvez consider que:
    // argv[1] contient un chemin valide vers un named pipe
    // argv[2] contient un entier valide (que vous pouvez convertir avec atoi()) representant le nombre de microsecondes a
    //      attendre a chaque envoi de paquet
    // argv[3] contient un entier valide (que vous pouvez convertir avec atoi()) contenant la taille voulue pour le tampon
    //      circulaire

    // Vous avez plusieurs taches d'initialisation a faire :
    //
    // 1) Ouvrir le named pipe

    // TODO
    int fd = open(argv[1], O_RDWR);
    if (fd == -1) {
        perror("Erreur ouverture du named pipe");
        exit(EXIT_FAILURE);
    }

    // 2) Declarer et initialiser la barriere
    
    // TODO
    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, 2);  // 2 threads doivent attendre

    // 3) Initialiser le tampon circulaire avec la bonne taille

    // TODO

    initTamponCirculaire(atoi(argv[3]));

    // 4) Creer et lancer les threads clavier et lecteur, en leur passant les bons arguments dans leur struct de configuration respective
    
    // TODO

    struct infoThreadLecture infosLecture = {
        .pipeFd = fd, 
        .barriere = &barrier
    };

    struct infoThreadClavier infosClavier = {
        .pointeurClavier = fopen("/dev/bus/usb/001/001", "w"),  // Fichier de sortie USB
        .tempsTraitementParCaractereMicroSecondes = atoi(argv[2]),
        .barriere = &barrier
    };

    pthread_t tid_clavier, tid_lecteur;
    pthread_create(&tid_clavier, NULL, threadFonctionClavier, (void*)&infosClavier);
    pthread_create(&tid_lecteur, NULL, threadFonctionLecture, (void*)&infosLecture);


    // La boucle de traitement est deja implementee pour vous. Toutefois, si vous voulez eviter l'affichage des statistiques
    // (qui efface le terminal a chaque fois), vous pouvez commenter la ligne afficherStats().
    struct statistiques stats;
    double tempsDebut = get_time();
    while(1){
        // Affichage des statistiques toutes les 2 secondes
        calculeStats(&stats);
        afficherStats((unsigned int)(round(get_time() - tempsDebut)), &stats);
        resetStats();
        usleep(2e6);
    }

    pthread_join(threadFonctionClavier, NULL);
    pthread_join(threadFonctionClavier, NULL);

    pthread_barrier_destroy(&barrier);
    close(infosLecture.pipeFd);
    fclose(infosClavier.pointeurClavier);
    return 0;
}
