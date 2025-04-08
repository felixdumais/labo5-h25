/******************************************************************************
 * Laboratoire 5
 * GIF-3004 Systèmes embarqués temps réel
 * Hiver 2025
 * Marc-André Gardner
 * 
 * Fichier implémentant les fonctions de l'emulateur de clavier
 ******************************************************************************/

#include "emulateurClavier.h"
#include <stdbool.h> 


FILE* initClavier(){
    // Deja implementee pour vous
    FILE* f = fopen(FICHIER_CLAVIER_VIRTUEL, "wb");
    setbuf(f, NULL);        // On desactive le buffering pour eviter tout delai
    return f;
}


int ecrireCaracteres(FILE* periphClavier, const char* caracteres, size_t len, unsigned int tempsTraitementParPaquetMicroSecondes){
    // TODO ecrivez votre code ici. Voyez les explications dans l'enonce et dans
    // emulateurClavier.h
    if (periphClavier == NULL || caracteres == NULL || len == 0) {
        return -1; // Erreur si les arguments sont invalides
    }

    unsigned char paquet[LONGUEUR_USB_PAQUET] = {0};
    unsigned char bufferZeros[LONGUEUR_USB_PAQUET] = {0}; // Buffer rempli de zéros

    size_t caracteresEcrits = 0;
    size_t i = 0;
    bool is_maj_pressed;


    while (i < len) {
        // Remplir le paquet avec les codes clavier
        memset(paquet, 0, LONGUEUR_USB_PAQUET);
        is_maj_pressed = false;

        // Remplir le paquet avec les codes clavier
        for (size_t j = 2; j < LONGUEUR_USB_PAQUET && i < len; j++, i++) {
            char c = caracteres[i];
            unsigned char keycode = 0;
            bool need_shift = false;

            if (c >= 'a' && c <= 'z') {
                keycode = c - 'a' + 4;
            } else if (c >= 'A' && c <= 'Z') {
                keycode = c - 'A' + 4;
                need_shift = true;
            } else if (c >= '1' && c <= '9') {
                keycode = c - '1' + 30;
            } else {
                switch (c) {
                    case '.': keycode = 55; break;
                    case ',': keycode = 54; break;
                    case '\n': keycode = 40; break;
                    case ' ': keycode = 44; break;
                    case '0': keycode = 39; break;
                    default: keycode = 0; break;
                }
            }

            // Gestion du verrouillage majuscule
            if (need_shift) {
                is_maj_pressed = true;
                paquet[0] = 0x02; // Indicateur "Shift"
            } else if (is_maj_pressed) {
                // i--;
                break;
            }

            paquet[j] = keycode;
        }

        // // Envoyer le paquet
        // if (fwrite(paquet, LONGUEUR_USB_PAQUET, 1, periphClavier) != 1) {
        //     return -1; // Erreur lors de l'écriture
        // }

        // // Envoyer le buffer de zéros pour relâcher les touches
        // if (fwrite(bufferZeros, LONGUEUR_USB_PAQUET, 1, periphClavier) != 1) {
        //     return -1; // Erreur lors de l'écriture
        // }

        memset(paquet, 0, sizeof(char));

        // Attendre le temps de traitement
        usleep(tempsTraitementParPaquetMicroSecondes);

        caracteresEcrits += (i - caracteresEcrits < LONGUEUR_USB_PAQUET) ? (i - caracteresEcrits) : LONGUEUR_USB_PAQUET;
    }

    return caracteresEcrits;


}
