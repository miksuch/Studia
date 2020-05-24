#include "utils.h"
char prompt[MTXT_MAX], txtbuf[BUF_MAX];
int msqid;
void send_msg( char *txt, char action, int lp )
{
    struct msgbuf m;
    if( txt != NULL && action != 'R' )
        strcpy( m.mtxt, txt );
    m.mtype = getpid();
    m.special_char = action;
    m.special_int = lp;
    if ( msgsnd( msqid, (struct msgbuf *)&m, sizeof(m), 0) == -1 )
    {
        perror("msgsnd");
        exit(EXIT_FAILURE);
    }
}
void leave()
{
    printf("Zamykanie...\n");
    send_msg( NULL, 'R', 0 );
    exit(EXIT_SUCCESS);
}
void SIGINT_handler( int x )
{
    leave();
}
int create_puzzle_client()
{
    sprintf( prompt, "Podaj hasło w postaci: [nnnn] ( gdzie n jest cyfrą z przedziału 1 do 6 )\n");
    if( write( fileno(stdout), prompt, strlen(prompt) ) < (int)strlen(prompt) )
    {
        perror("write");
        exit(EXIT_FAILURE);
    }
    if( read( fileno( stdin ), txtbuf, BUF_MAX ) < 0 )
    {
        perror("read");
        exit(EXIT_FAILURE);
    }
    txtbuf[BUF_MAX-1] = '\0';
    send_msg( txtbuf, 'C', 0 );
    return 0;
}
void list_puzzles_client()
{
    send_msg( NULL, 'L', 0 );
    struct msgbuf m;
    int end_of_list = 0, i, k;
    while( end_of_list != 1 )
    {
        if ( msgrcv(msqid, (struct msgbuf *)&m, sizeof(m), getpid(), 0) == -1 )
        {
            perror("msgrcv");
            exit(EXIT_FAILURE);
        }
        if( m.special_int >= 0 )
        {
            for( i = 0, k = 0; i < m.special_int; ++i )
            {
                if( m.mtxt[i] != '*' )
                {
                    prompt[k] = m.mtxt[i];
                    ++k;
                }
            }
            if( write( fileno(stdout), prompt, k ) < k )
            {
                perror("write");
                exit(EXIT_FAILURE);
            }
        }
        else end_of_list = 1;
    }
}
void check_for_replies()
{
    struct msgbuf m;
    if( msgrcv(msqid, (struct msgbuf *)&m, sizeof(m), getpid(), 0) == -1 );
    else if( m.mtxt != NULL && (int)strlen(m.mtxt) == m.special_int )
        printf("%s", m.mtxt );
    return;
}
void get_game_reply()
{
    struct msgbuf m;
    int msg_received = 0;
    while( !msg_received )
    {
        if ( msgrcv(msqid, (struct msgbuf *)&m, sizeof(m), getpid(), 0) == -1 )
        {
            perror("msgrcv");
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("%s", m.mtxt );
            msg_received = 1;
        }
    }
}
void play()
{
    int exit_flag = 0;
    long lp;
    char *tmp, option;
    sprintf( prompt, "Sposób gry:\n[lp] [nnnn]\tlp - numer zagadki, n - cyfra z zakresu 1 do 6\nq Q\tCofnij do głównego menu.\n");
    write( fileno(stdout), prompt, strlen(prompt) );
    while( exit_flag != 1 )
    {
        for( int i = 0; i < BUF_MAX; ++i )
            txtbuf[i]='\0';
        if( fgets( txtbuf, BUF_MAX, stdin ) == NULL )
        {
            perror("fgets");
            exit(EXIT_FAILURE);
        }
        if( strlen(txtbuf) == 2 && ( txtbuf[0] == 'q' || txtbuf[0] == 'Q' ) )
            option = txtbuf[0];
        else
        {
            lp = strtol( txtbuf, &tmp, 10 );
            if( lp == LONG_MIN || lp == LONG_MAX )
                option = '#';
            else if( strlen(tmp) == 6 && tmp[0] == ' ' ) // zagadka + spacja i enter
            {
                for( int i = 1; i < 6; ++i )
                    txtbuf[i-1] = tmp[i];
                txtbuf[5] = '\0';
                option = 'P';
            }
            else option = '#';
        }
        switch( option )
        {
        case 'P':
            send_msg( txtbuf, 'P', lp );
            get_game_reply();
            break;
        case 'q':
        case 'Q':
            exit_flag = 1;
            help( 0 );
            break;
        case '#':
        default:
            write( fileno(stdout), prompt, strlen(prompt) );

            break;
        }
    }
}
int main( int argc, char *argv[] )
{
    sig_handler();
    if( argc != 3 )
    {
        printf("Użycie: %s [ścieżka do pliku] [nazwa gracza]\n", argv[0] );
        exit(EXIT_FAILURE);
    }
    else if( strlen(argv[2]) > PLAYERNAME_MAX )
    {
        printf("Podana nazwa gracza jest zbyt długa. Maksymalna ilość znaków: %d\n", PLAYERNAME_MAX-1 );
        exit(EXIT_FAILURE);
    }
    char playername[PLAYERNAME_MAX], option; // fgets zawsze zapisze w ostatnim polu tablicy '\0'
    strcpy( playername, argv[2] );
    int exit_flag = 0;
    key_t key;
    if ((key = ftok(argv[1], 'M')) == -1)
    {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    if ((msqid = msgget(key, 0644)) == -1)
    {
        perror("msgget");
        exit(EXIT_FAILURE);
    }
    send_msg( playername, 'N', 0 );
    start_message( 0 );
    while( exit_flag != 1 )
    {
        for( int i = 0; i < BUF_MAX; ++i )
            txtbuf[i]='\0';
        if( fgets( txtbuf, BUF_MAX, stdin ) != NULL )
        {
            if( strlen(txtbuf) == 2 )
                option = txtbuf[0];
            else
                option = '#';
        }
        else
        {
            perror("fgets");
            exit(EXIT_FAILURE);
        }

        switch( option )
        {
        case 'c':
        case 'C':
            create_puzzle_client();
            check_for_replies();
            printf("\n");
            break;
        case 'l':
        case 'L':
            list_puzzles_client();
            printf("\n");
            break;
        case 'p':
        case 'P':
            play();
            break;
        case 'h':
        case 'H':
            help( 0 );
            break;
        case 'd':
        case 'D':
            send_msg( NULL, 'D', 0 );
            break;
        case 'q':
        case 'Q':
            exit_flag = 1;
            send_msg( NULL, 'R', 0 );
            break;
        case '#':
        default:
            printf("Podano nieprawidłową opcję. Podaj \"h\" lub \"H\" by uzyskać więcej informacji.\n");
            break;
        }
    }
    return 0;
}

