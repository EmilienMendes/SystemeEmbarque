#include "microbian.h"
#include "hardware.h"
#include "lib.h"

#define GROUP 16
#define IMAGE_SIZE 40
#define TRUE 1
#define FALSE 0

typedef struct
{
    int positionX;
    int positionY;
    int led_state;
    int button_pressed;
} Cursor;

static Cursor my_cursor = {0, 4, 1, FALSE};
static image current_image;

/*
    Reception de l'image
    Format identique à celui fait pour les emoji
*/
void receiver_task(int dummy)
{
    byte buf[RADIO_PACKET];
    int n;

    while (1)
    {
        n = radio_receive(buf);
        if (n == IMAGE_SIZE)
        {
            // On passe en mode affichage
            my_cursor.button_pressed = FALSE;
            printf("Reception de l'image \n");
            display_show((void *)buf);
        }
    }
}

// Change l'etat de la led
void switch_led()
{
    if (my_cursor.led_state == 1)
    {
        my_cursor.led_state = 0;
    }
    else
    {
        my_cursor.led_state = 1;
    }
}

// Allume ou eteint la led en fonction de l'etat courant
void update_led_state(int current_positionX, int current_positionY)
{
    if (my_cursor.led_state == 1)
    {
        image_unset(current_positionX, current_positionY, current_image);
    }
    else
    {
        image_set(current_positionX, current_positionY, current_image);
    }
}

/*
    Reste bloque dans la boucle tant que le bouton est appuye
    Calcule le temps total d'appui sur le bouton
*/
unsigned debounce(int buttonType)
{
    int current_state = 0;
    int time_state = timer_now();
    while (!current_state)
    {
        current_state = gpio_in(buttonType);
        timer_delay(20);
        if (current_state != 0)
        {
            current_state = gpio_in(buttonType);
        }
    }
    return timer_now() - time_state;
}

// Permet de faire le tour de la matrice sans depasser les limites
int decrement_modulo(int x, int mod)
{
    x--;
    if (x < 0)
    {
        x += mod;
    }
    return x;
}

// Envoi du dessin
void sender_task(int dummy)
{
    gpio_connect(BUTTON_A);
    gpio_connect(BUTTON_B);
    unsigned time_state;
    while (1)
    {

        // Appui d'au moins un bouton
        if (gpio_in(BUTTON_A) != 1 || gpio_in(BUTTON_B) != 1)
        {
            /*
                Appui initial sur un bouton
                Active le mode dessin et activation de la led a la position initiale
            */
            if (!my_cursor.button_pressed)
            {
                my_cursor.button_pressed = TRUE;
                image_clear(current_image);
                my_cursor.positionX = 0;
                my_cursor.positionY = 4;
                timer_delay(150);
            }
            else
            {
                // Attente du debounce pour les valeurs des entrees
                timer_delay(40);
                // Envoi de l'image (bouton A et B appuyes)
                if (gpio_in(BUTTON_A) == 0 && gpio_in(BUTTON_B) == 0)
                {

                    update_led_state(my_cursor.positionX, my_cursor.positionY);
                    radio_send((void *)current_image, sizeof(current_image));

                    // Attente que les deux boutons ne soient plus actives
                    while (gpio_in(BUTTON_A) == 0 || gpio_in(BUTTON_B) == 0)
                    {
                        timer_delay(10);
                    }
                }
                // Bouton A
                else if (gpio_in(BUTTON_A) == 0 && gpio_in(BUTTON_B) == 1)
                {
                    // Calcul du temps appuye sur le bouton A
                    time_state = debounce(BUTTON_A);
                    /*
                    Appui plus d'une seconde -> La led change d'etat et on passe a la led suivante
                    Sinon on passe juste a la led suivante
                    */
                    if (time_state > 1000)
                    {
                        switch_led();
                    }
                    my_cursor.positionY = decrement_modulo(my_cursor.positionY, 5);
                }
                // Bouton B
                else if (gpio_in(BUTTON_A) == 1 && gpio_in(BUTTON_B) == 0)
                {
                    /*
                   Appui plus d'une seconde -> La led change d'etat et on passe a la led suivante
                   Sinon on passe juste a la led suivante
                   */
                    time_state = debounce(BUTTON_B);
                    if (time_state > 1000)
                    {
                        switch_led();
                    }
                    my_cursor.positionX = (my_cursor.positionX + 1) % 5;
                }
            }
            timer_delay(150);
        }
    }
}

// Tache d'affichage de la led courante
void led_task(int dummy)
{
    int led_display = FALSE;
    int current_positionX = my_cursor.positionX;
    int current_positionY = my_cursor.positionY;
    while (1)
    {
        if (my_cursor.button_pressed)
        {
            // Faire clignoter la led si on reste sur le meme emplacement
            if (current_positionX == my_cursor.positionX && current_positionY == my_cursor.positionY)
            {
                if (led_display)
                {
                    // Desactive la led en position (positionX,positionY) de l'image
                    image_unset(my_cursor.positionX, my_cursor.positionY, current_image);
                }
                else
                {
                    // Active la led en position (positionX,positionY) de l'image
                    image_set(current_positionX, current_positionY, current_image);
                }
                led_display = !led_display;
                timer_delay(300);
            }
            // Sinon on change l'etat de la led courante
            else
            {
                update_led_state(current_positionX, current_positionY);
                // On met a jour la position de notre led et on regarde l'etat de celle-ci avant modification
                current_positionX = my_cursor.positionX;
                current_positionY = my_cursor.positionY;
                my_cursor.led_state = state_pixel(current_positionX, current_positionY, current_image);
            }
            display_show(current_image);
        }
        timer_delay(100);
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
    start("Led", led_task, 0, STACK);
}