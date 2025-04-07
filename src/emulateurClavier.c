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

    unsigned char paquet[LONGUEUR_USB_PAQUET] = {0, 0, 4, 5, 6, 7, 8, 9};
    unsigned char bufferZeros[LONGUEUR_USB_PAQUET] = {0}; // Buffer rempli de zéros

    size_t caracteresEcrits = 0;
    size_t i = 0;
    bool is_maj_pressed = false;


    while (i < len) {
        // Remplir le paquet avec les codes clavier
        for (size_t j = 2; j < LONGUEUR_USB_PAQUET && i < len; j++) {
            char c = caracteres[i];
            if (c >= 'a' && c <= 'z') {
                if (is_maj_pressed) break;
                paquet[j] = c - 'a' + 4; // Codes clavier pour a-z (exemple)
            } else if (c >= 'A' && c <= 'Z') {
                is_maj_pressed = true;
                paquet[j] = c - 'A' + 4; // Codes clavier pour A-Z (exemple)
                paquet[0] = 2;
            } else if (c >= '0' && c <= '9') {
                if (is_maj_pressed) break;
                paquet[j] = c - '0' + 30; // Codes clavier pour 0-9 (exemple)
            } else if (c == '.') {
                if (is_maj_pressed) break;
                paquet[j] = 55; // Code clavier pour . (exemple)
            } else if (c == ',') {
                if (is_maj_pressed) break;
                paquet[j] = 54; // Code clavier pour , (exemple)
            } else if (c == '\n') {
                if (is_maj_pressed) break;
                paquet[j] = 40; // Code clavier pour Entrée (exemple)
            } else {
                if (is_maj_pressed) break;
                paquet[j] = 0; // Code par défaut pour les autres caractères
            }
            i++;
        }

        // Envoyer le paquet
        if (fwrite(paquet, LONGUEUR_USB_PAQUET, 1, periphClavier) != 1) {
            return -1; // Erreur lors de l'écriture
        }

        // Envoyer le buffer de zéros pour relâcher les touches
        if (fwrite(bufferZeros, LONGUEUR_USB_PAQUET, 1, periphClavier) != 1) {
            return -1; // Erreur lors de l'écriture
        }

        memset(paquet, 0, sizeof(char));

        // Attendre le temps de traitement
        usleep(tempsTraitementParPaquetMicroSecondes);

        caracteresEcrits += (i - caracteresEcrits < LONGUEUR_USB_PAQUET) ? (i - caracteresEcrits) : LONGUEUR_USB_PAQUET;
    }

    return caracteresEcrits;


}
