#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <limits.h>
#define BUF_MAX 16
#define PLAYERS_MAX 64
#define PLAYERNAME_MAX 33
#define MTXT_MAX 128
#define GAMES_MAX 8
struct msgbuf {
    long mtype; // zawsze odbiorca/nadawca
    char mtxt[MTXT_MAX];
    char special_char;
    int special_int;
};
struct mm_session{
long playerid;
char playername[PLAYERNAME_MAX];
int turns_left[GAMES_MAX];
int puzzle_lp[GAMES_MAX];
};
void SIGINT_handler( int x );
int sig_handler()
{
    sigset_t iset;
    struct sigaction act;

    sigemptyset(&iset);
    act.sa_handler = &SIGINT_handler;
    act.sa_mask = iset;
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);
    return 1;
}
void help( int server )
{
    printf("Możliwe opcje:\n");
    if( !server )
        printf("p P\tGraj.\n");
    printf( "c C\tStwórz nową zagadkę.\n");
    printf( "l L\tWypisz istniejące zagadki.\n");
    printf( "h H\tPokaż możliwe opcje.\n");
    printf( "q Q\tWyjdź.\n\n");
}
void start_message( int server )
{
    printf("Mastermind IPC ");
    if( server )
        printf("Serwer ");
    printf("- Mikołaj Suchodolski\n\n");
    if( !server )
        help( server );
}

