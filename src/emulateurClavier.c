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
    bool is_maj_packet;
    bool is_lower_packet;


    while (i < len) {
        // Remplir le paquet avec les codes clavier
        memset(paquet, 0, LONGUEUR_USB_PAQUET);
        is_maj_packet = false;
        is_lower_packet = false;

        // Remplir le paquet avec les codes clavier
        for (size_t j = 2; j < LONGUEUR_USB_PAQUET && i < len; i++) {
            char c = caracteres[i];
            unsigned char keycode = 0;

            if (c >= 'a' && c <= 'z') {
                keycode = c - 'a' + 4;
                if (is_maj_packet) break; // stop if we're switching cases
                is_lower_packet = true;
            } else if (c >= 'A' && c <= 'Z') {
                keycode = c - 'A' + 4;
                if (is_lower_packet) break; // stop if we're switching cases
                is_maj_packet = true;
                paquet[0] = 0x02; // Shift modifier
            } else if (c >= '1' && c <= '9') {
                keycode = c - '1' + 30;
                if (is_maj_packet) break; // stop if we're switching cases
                is_lower_packet = true;
            } else {
                if (is_maj_packet) break; // stop if we're switching cases
                is_lower_packet = true;
                switch (c) {
                    case '.': keycode = 55; break;
                    case ',': keycode = 54; break;
                    case '\n': keycode = 40; break;
                    case ' ': keycode = 44; break;
                    case '0': keycode = 39; break;
                    default: keycode = 0; break;
                }
            }

            if (keycode != 0 && j > 2)
            {
                bool is_keycode_in_paquet = false;
                for (size_t k = 2; k < j; k++)
                {
                    if (paquet[k] == keycode)
                    {
                        is_keycode_in_paquet = true;
                        break;
                    }

                }
                if (is_keycode_in_paquet)
                    break;
            }

            paquet[j++] = keycode;
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