#include "login.h"
#include <stdio.h>
#include <unistd.h>
#include <termios.h>

void loadingBar() {
    printf("\nLogging into Warehouse Management System...\n\n");
    for (int i = 0; i <= 100; i += 5) {
        printf("\rLoading: [");
        int j;
        for (j = 0; j < i/5; j++) printf("#");
        for (; j < 20; j++) printf(".");
        printf("] %d%%", i);
        fflush(stdout);
        usleep(100000);
    }
    printf("\n\nAccess Granted!\n");
}

int my_getch(void) {
    struct termios oldt, newt;
    int ch;
    
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    
    return ch;
}

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}
