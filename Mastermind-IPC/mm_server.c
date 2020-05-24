#include "utils.h"
char prompt[MTXT_MAX], txtbuf[BUF_MAX], data[MTXT_MAX];
int lp, msqid, fd;
struct mm_session *sessions[PLAYERS_MAX];
void leave()
{
    msgctl(msqid, IPC_RMID, NULL );
    close(fd);
    exit(EXIT_SUCCESS);
}
void SIGINT_handler( int x )
{
    printf("Zamykanie...\n");
    leave();
}
void send_reply( char *txt, long recipient, int special_i )
{
    struct msgbuf m;
    if( txt != NULL )
        strcpy( m.mtxt, txt );
    m.mtype = recipient;
    m.special_char = 'S';
    m.special_int = special_i;
    if ( msgsnd( msqid, (struct msgbuf *)&m, sizeof(m), 0) == -1)
    {
        perror("msgsnd");
        exit(EXIT_FAILURE);
    }
}
int count_lines()
{
    char c;
    int lines = 0;
    if( lseek( fd, 0, SEEK_SET ) < 0 )
    {
        perror("lseek");
        exit(EXIT_FAILURE);
    }
    while( read( fd, &c, 1 ) > 0 )
    {
        if( c == '\n' )
            ++lines;
    }
    return lines;
}
void init_sessions()
{
    for( int i = 0; i < PLAYERS_MAX; ++i )
        sessions[i] = NULL;
}
void register_player( struct msgbuf *m )
{
    for( int i = 0; i < PLAYERS_MAX; ++i )
    {
        if( sessions[i] == NULL )
        {
            sessions[i] = malloc( sizeof(struct mm_session));
            sessions[i]->playerid = m->mtype;
            strcpy( sessions[i]->playername, m->mtxt );
            for( int k = 0; k < GAMES_MAX; ++k )
            {
                sessions[i]->puzzle_lp[k] = 0;
                sessions[i]->turns_left[k] = 0;
            }
            return;
        }
    }
    printf("Brak miejsca dla nowych graczy!\n");
    sprintf(prompt,"Serwer obsługuje już maksymalną ilość graczy proszę spróbować później i uruchomić grę ponownie.\n");
    send_reply( prompt, m->mtype, m->special_int );
    return;
}
void remove_player( long id ) // id < 0 to remove all
{
    int remove_all;
    if( id < 0 )
        remove_all = 1;
    else remove_all = 0;
    for( int i = 0; i < PLAYERS_MAX; ++i )
        if( ( sessions[i] != NULL && sessions[i]->playerid == id ) || remove_all )
        {
            free( sessions[i] );
            sessions[i] = NULL;
            if( !remove_all )
                return;
        }
}
void show_sessions() // debug
{
    for( int i = 0; i < PLAYERS_MAX; ++i )
    {
        if( sessions[i] != NULL )
            printf("session %d, player = %s\n", i, sessions[i]->playername );
    }
    printf("\n");
}
int get_line_offset( int n ) // zwraca nr bajtu który jest początkiem n-tej lini w pliku
{
    char c;
    int lines = 0, offset = 0;
    if( lseek( fd, 0, SEEK_SET ) < 0 )
        return -1;
    while( read( fd, &c, 1 ) > 0 )
    {
        if( n - 1 == lines )
            return offset;
        if( n > 0 )
            ++offset;
        if( c == '\n' )
            ++lines;
    }
    if( n > 0 )
        perror("get_line_offset");
    return -1;
}
int get_session_id( int playerid )
{
    for( int i = 0; i < PLAYERS_MAX; ++i )
        if( sessions[i] != NULL && sessions[i]->playerid == playerid ) // sesja gracza
            return i;
    return -1;
}
char* get_playername( long id )
{
    for( int i = 0; i < PLAYERS_MAX; ++i )
        if( sessions[i] != NULL && id == sessions[i]->playerid )
            return sessions[i]->playername;
    return NULL;
}
int get_open_game_slot( int session_id, int puzzle )
{
    int k = -1;
    for( int i = 0; i < GAMES_MAX; ++i )
    {
        if( sessions[session_id]->puzzle_lp[i] == puzzle )
            return i;
        if( sessions[session_id]->puzzle_lp[i] == 0 )
        {
            sessions[session_id]->turns_left[i] = 12;
            k = i;
        }
    }
    return k;
}
void close_game( int session_id, int game_num )
{
    sessions[session_id]->puzzle_lp[game_num] = 0;
}
int add_puzzle( char *puzzle, char *playername )
{
    if( playername == NULL )
        return -1;
    sprintf( data, "%03d\t%s\t%s\n", ++lp, puzzle, playername );
    if( lseek( fd, 0, SEEK_END ) < 0 )
        return -1;
    if( write( fd, data, strlen(data) ) <= 0 )
        return -1;
    return 1;
}
int isValidPuzzle( char *puzzle, int begin, int end )
{
    if( end - begin != 5 )
        return -1;
    puzzle[--end]='\0'; // obcinam enter;
    for( int i = begin; i < end; ++i )
    {
        if( puzzle[i] < '1' || puzzle[i] > '6' )
            return -1;
    }
    return 1;
}
void create_puzzle( char *txtbuf, long player_id )
{
    if( isValidPuzzle( txtbuf, 0, strlen(txtbuf) ) == 1 )
    {
        char *author = get_playername( player_id );
        if( add_puzzle( txtbuf, author ) < 0 )
        {
            sprintf( prompt, "Wystąpił problem podczas dodawania zagadki. Zagadka nie została dodana.\n" );
            send_reply( prompt, player_id, strlen(prompt) );
            return;
        }
        else
        {
            sprintf( prompt, "Zagadka została dodana pomyślnie z numerem %d !\n", lp );
            send_reply( prompt, player_id, strlen(prompt) );
            return;
        }
    }
    else
    {
        sprintf( prompt, "Podana zagadka %s nie jest poprawna.\n", txtbuf );
        send_reply( prompt, player_id, strlen(prompt) );
        return;
    }
}
void list_puzzles( long recipient )
{
    int n, i, puzzle_flag = 1;
    if( lseek( fd, 0, SEEK_SET ) < 0 )
    {
        printf("list_puzzles lseek nie powiódł się\n");
        sprintf( prompt, "Wystąpił problem po stronie serwera. Zagadki nie będą wylistowane.\n");
        send_reply( prompt, recipient, strlen(prompt) );
        send_reply( NULL, recipient, -1 );
        return;
    }
    sprintf( prompt, "\nLp.:\tAutor:\n" );
    send_reply( prompt, recipient, strlen(prompt) );
    while( ( n = read( fd, data, MTXT_MAX ) ) > 0 )
    {
        for( i = 0; i < n; ++i )
        {
            if( data[i] == '\t' )
            {
                if( puzzle_flag == 1 )
                    data[i] = '*';
                puzzle_flag *= (-1);
                continue;
            }
            if( puzzle_flag == -1 )
            {
                data[i] = '*';
            }
        }
        send_reply( data, recipient, n );
    }
    send_reply( NULL, recipient, -1 );
    return;
}
void checkout_answer( struct msgbuf* m )
{
    if( isValidPuzzle( m->mtxt, 0, strlen(m->mtxt) ) == 1 && m->special_int <= lp )
    {
        int session_id = get_session_id( m->mtype );
        if( session_id < 0 )
        {
            printf("Gracz o nr %ld nie został znaleziony\n", m->mtype );
            sprintf( prompt, "Nie znaleziono Ciebie na liście graczy, proszę uruchomić grę ponownie.\n");
            send_reply( prompt, m->mtype, strlen(prompt) );
            return;
        }
        int game_num = get_open_game_slot( session_id, m->special_int );
        if( game_num < 0 )
        {
            sprintf( prompt, "Nie możesz grać w więcej niż %d gier w tym samym czasie.\n", GAMES_MAX );
            send_reply( prompt, m->mtype, strlen(prompt) );
            return;
        }
        else
        {
            sessions[session_id]->puzzle_lp[game_num]=m->special_int;
            int offset = get_line_offset( m->special_int );
            if( offset < 0 )
            {
                printf("Nie znaleziono offsetu linii nr %d\n", m->special_int );
                sprintf( prompt, "Wystąpił problem podczas weryfikacji zagadki proszę spróbować ponownie.\n" );
                send_reply( prompt, m->mtype, m->special_int );
                return;
            }
            lseek( fd, offset + 4, SEEK_SET );
            read( fd, txtbuf, 4 );
            char tmp_answer[5]="0000";
            for( int i = 0; i < 4; ++i )
            {
                for( int k = 0; k < 4; ++k )
                {
                    if( m->mtxt[i] == txtbuf[k] )
                    {
                        if( i == k )
                        {
                            tmp_answer[i] = '2';
                            break;
                        }
                        else if( tmp_answer[i] != '2' )
                            tmp_answer[i] = '1';
                    }
                }
            }
            sessions[session_id]->turns_left[game_num] -= 1;
            m->special_int = sessions[session_id]->turns_left[game_num];
            if( strcmp("2222", tmp_answer ) == 0 )
            {
                sprintf( prompt, "Gratulacje %s ! Rozwiązałeś zagadkę nr %d.\n", sessions[session_id]->playername, sessions[session_id]->puzzle_lp[game_num] );
                close_game( session_id, game_num );
            }
            else if( m->special_int == 0 )
            {
                sprintf( prompt, "Odpowiedź: %s\tWykorzystałeś wszystkie próby i nie udało Ci się rozwiązać zagadki nr %d, spróbuj ponownie.\n", tmp_answer, sessions[session_id]->puzzle_lp[game_num] );
                close_game( session_id, game_num );
            }
            else sprintf( prompt, "Zagadka nr %d - Odpowiedź: %s Pozostało prób: %d\n", sessions[session_id]->puzzle_lp[game_num], tmp_answer, m->special_int );

            send_reply( prompt, m->mtype, strlen(prompt) );
            return;
        }
    }
    else
    {
        sprintf( prompt, "Podano niedozwoloną odpowiedź lub zagadka o tym numerze nie istnieje.\n");
        send_reply( prompt, m->mtype, strlen(prompt) );
        return;
    }
}

int main( int argc, char *argv[] )
{
    sig_handler();
    atexit( leave );
    if( argc != 2 )
    {
        printf("Użycie: %s [ścieżka do pliku]\n", argv[0] );
        exit(EXIT_FAILURE);
    }
    struct msgbuf m;
    key_t key;
    init_sessions( );
    if( (fd = open("puzzle.data", O_RDWR | O_CREAT, 0660 )) == -1 )
    {
        perror("Unable to create or open puzzle.data file");
        exit(EXIT_FAILURE);
    }
    lp = count_lines(); // -1 dla policzenia wszystkich
    if ((key = ftok(argv[1], 'M')) == -1)
    {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    if ((msqid = msgget(key, 0644 | IPC_CREAT)) == -1)
    {
        perror("msgget");
        exit(EXIT_FAILURE);
    }

    start_message( 1 );
    while( 1 )
    {
        if ( msgrcv(msqid, (struct msgbuf *)&m, sizeof(m), 0, 0) == -1 )
        {
            perror("msgrcv");
            exit(EXIT_FAILURE);
        }
        if( m.special_char == 'S' ) // odebrano wiadomość do klienta
        {
            if( get_session_id(m.mtype) < 0 )
                remove_player( m.mtype );
            else send_reply( m.mtxt, m.mtype, m.special_int );
        }
        else
        {
            switch( m.special_char )
            {
            case 'D': // debug
                show_sessions();
                break;
            case 'N': // new player
                register_player( &m );
                break;
            case 'C': // create
                create_puzzle( m.mtxt, m.mtype );
                break;
            case 'L': // list
                list_puzzles( m.mtype );
                break;
            case 'P': // play
                checkout_answer( &m );
                break;
            case 'R': // player left
                remove_player( m.mtype );
                break;
            default:
                printf("Otrzymano wiadomość o niezdefiniowanej odpowiedzi: %c\n", m.special_char );
            }
        }
    }
    return 0;
}
