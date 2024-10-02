
#include "microbian.h"
#include "hardware.h"
#include "lib.h"

#define GROUP 16
#define IMAGE_SIZE 40
#define MAX_IMAGE 4

const image emoji_sourire =
    IMAGE(0, 1, 0, 1, 0,
          0, 0, 0, 0, 0,
          1, 0, 0, 0, 1,
          0, 1, 1, 1, 0,
          0, 0, 0, 0, 0);

const image grand_coeur =
    IMAGE(0, 1, 0, 1, 0,
          1, 1, 1, 1, 1,
          1, 1, 1, 1, 1,
          0, 1, 1, 1, 0,
          0, 0, 1, 0, 0);

const image emoji_triste =
    IMAGE(0, 1, 0, 1, 0,
          0, 0, 0, 0, 0,
          0, 1, 1, 1, 0,
          1, 0, 0, 0, 1,
          0, 0, 0, 0, 0);

const image petit_coeur =
    IMAGE(0, 1, 0, 1, 0,
          0, 1, 1, 1, 0,
          0, 1, 1, 1, 0,
          0, 0, 1, 0, 0,
          0, 0, 0, 0, 0);

static const image *list_image[MAX_IMAGE] = {&emoji_sourire, &grand_coeur, &emoji_triste, &petit_coeur};

void receiver_task(int dummy)
{
    byte buf[RADIO_PACKET];
    int n;
    printf("Hello\n");
    while (1) {
        n = radio_receive(buf);
        if(n == IMAGE_SIZE){
            printf("Reception de l'image \n");
            display_show((void*)buf);
        }
    }
}

void sender_task(int dummy)
{
    // Lecture des boutons A et B
    gpio_connect(BUTTON_A);
    gpio_connect(BUTTON_B);
    int state_buttton_A = 1;
    int state_buttton_B = 1;
    int image_choisi = 0;
    while (1)
    {
        state_buttton_A = gpio_in(BUTTON_A);
        state_buttton_B = gpio_in(BUTTON_B);

        // Verification de l'appui d'au moins un bouton
        if (state_buttton_A != 1 || state_buttton_B != 1)
        {
            /*
            Nouvelle verification de l'etat des boutons apres le delay
            Permet de gerer le debounce
            */
            timer_delay(100);
            if(gpio_in(BUTTON_A) != 1){
            state_buttton_A = gpio_in(BUTTON_A);
            }
            if(gpio_in(BUTTON_B) != 1){
                state_buttton_B = gpio_in(BUTTON_B);
            }
            // Bouton A et B appuye simultanement
            if (state_buttton_A == 0 && state_buttton_B == 0)
            {
                if (image_choisi < 0 || image_choisi > MAX_IMAGE - 1)
                {
                    printf("Erreur lors de l'envoi de l'image \n");
                }
                else
                {
                    printf("Envoi de l'image \n");
                    radio_send((void *)*list_image[image_choisi], sizeof(*list_image[image_choisi]));
                }
            }
            // Bouton A appuye
            else if (state_buttton_A == 0 && state_buttton_B == 1 && image_choisi > 0)
            {
                printf("Passage a l'emoji suivant \n");
                image_choisi--;
            }
            // Bouton B appuye
            else if (state_buttton_A == 1 && state_buttton_B == 0 && image_choisi < MAX_IMAGE - 1)
            {
                printf("Retour a l'emoji precedent \n");
                image_choisi++;
            }
            display_show(*list_image[image_choisi]);
            timer_delay(100);
        }
    }
}

void init(void)
{
    serial_init();
    radio_init();
    radio_group(GROUP);
    timer_init();
    display_init();
    start("Receiver", receiver_task, 0, STACK);
    start("Sender", sender_task, 0, STACK);
}