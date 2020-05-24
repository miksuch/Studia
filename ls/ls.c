
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <unistd.h>
#include <time.h>

#define MAX_PATH 4097
#define MAX_FILENAME 260

/* UZYTE STRUKTURY*/

struct flag_set
{
    int a,r,l,h,t,exit;
};
struct FILE_INFO
{
    struct stat st;
    char filename[MAX_FILENAME];
    double h_size;
    char h_unit;
};

/* FUNKCJE POMOCNICZE */

int numlen( double a )
{
    int w=1;
    unsigned long x = a;
    while( x != 0 )
    {
        x /= 10;
        ++w;
    }
    return w;
}
void clear_str( char *s, int size )
{
    for( int i = 0; i < size || s[i] == '\0' ; ++i )
        s[i] = '\0';
}
int no_of_files( char *path, int flag_a )
{
    DIR *directory = NULL;
    if ( ( directory = opendir(path) ) == NULL )
    {
        perror(path);
        return -1;
    }
    struct dirent *d;
    int number = 0;
    while( ( d = readdir(directory) ) != NULL  )
    {
        if( d->d_name[0] == '.' && !flag_a )
            continue;
        ++number;
    }
    closedir(directory);
    return number;
}
int no_of_directories( char *path )
{
    DIR *directory = NULL;
    if ( ( directory = opendir(path) ) == NULL )
    {
        perror(path);
        return -1;
    }
    struct dirent *d;
    struct stat st;
    char tmp_path[MAX_PATH];
    int number = 0;
    while( ( d = readdir(directory) ) != NULL  )
    {
        if( strcmp( d->d_name, "." ) == 0 || strcmp( d->d_name, ".." ) == 0 )
            continue;
        clear_str( tmp_path, MAX_PATH );
        sprintf( tmp_path, "%s/%s", path, d->d_name );
        if( stat( tmp_path, &st ) == -1 )
        {
            printf("Funkcja stat() nie powiodła się dla: %s ; Błąd: %s\n", tmp_path, strerror(errno) );
            errno=0;
            continue;
        }
        if( S_ISDIR( st.st_mode ) )
            ++number;
    }
    closedir(directory);
    return number;
}
void type_and_perms(int mode, char *str )
{

    switch ( mode & S_IFMT)
    {
    case S_IFBLK:
        str[0] = 'b';
        break;
    case S_IFCHR:
        str[0] = 'c';
        break;
    case S_IFDIR:
        str[0] = 'd';
        break;
    case S_IFIFO:
        str[0] = 'p';
        break;
    case S_IFLNK:
        str[0] = 'l';
        break;
    case S_IFREG:
        break;
    case S_IFSOCK:
        str[0] = 's';
        break;
    }
    if(mode & S_IRUSR)
        str[1] = 'r';
    if(mode & S_IWUSR)
        str[2] = 'w';
    if(mode & S_IXUSR)
        str[3] = 'x';
    if(mode & S_IRGRP)
        str[4] = 'r';
    if(mode & S_IWGRP)
        str[5] = 'w';
    if(mode & S_IXGRP)
        str[6] = 'x';
    if(mode & S_IROTH)
        str[7] = 'r';
    if(mode & S_IWOTH)
        str[8] = 'w';
    if(mode & S_IXOTH)
        str[9] = 'x';
    str[10] = '\0';
}
int time_compare(const void *l, const void *r)
{
    unsigned long ltime = (unsigned long)((struct FILE_INFO*)l)->st.st_atime;
    unsigned long rtime = (unsigned long)((struct FILE_INFO*)r)->st.st_atime;
    if( ltime < rtime )
        return 1;
    if( ltime == rtime )
        return 0;
    else return -1;
}
void help()
{
    printf( "Składnia: ls [OPCJA]... [PLIK]...\nWypisanie informacji o PLIKACH (domyślnie w katalogu bieżącym). Sortowane alfabetyczne, jeżeli nie jest podana opcja -t lub -T.\n\n");
    printf( "Obslugiwane opcje:\n");
    printf( "-a -a\tPokazuje również pliki zaczynające się na \".\".\n");
    printf( "-h -H\tZ opcją -l lub -L podaje rozmiary w formacie czytelnym dla ludzi ( np. 1K 234M 2G ).\n");
    printf( "-l -L\tWypisuje szczegółowe informacje o plikach.\n");
    printf( "-t -T\tSortowanie wg czasu dostępu, najmniej odległy na początku.\n");
}
void version()
{
    printf("ls - wersja wlasna 2.0\n\nAutor: Mikolaj Suchodolski\n");
}

/* FUNKCJE GLOWNE */

void list(char *path, struct flag_set *flags )
{
    DIR *directory = NULL;
    if ( ( directory = opendir(path) ) == NULL )
    {
        perror(path);
        return;
    }
    printf("%s:\n", path );
    /* uzywane zmienne */
    int n = no_of_files( path, flags->a );
    if( n < 0 )
        return;
    struct FILE_INFO file_tab[n];
    struct dirent *dp = NULL;
    char tmp_path[MAX_PATH];
    unsigned int i = 0, k = 0;

    /* dla opcji -l */
    int field_width[] = {0,0,0,0}, tmp[] = {0,0,0,0};
    /* dla opcji -h */
    char units[] = { '#', 'K', 'M', 'G' };



    /* przegladam i zapisuje informacje o plikach w podanym katalogu */
    while( ( dp = readdir(directory) ) != NULL )
    {
        if( dp->d_name[0] == '.' && !flags->a )
            continue;
        clear_str( tmp_path, MAX_PATH );
        clear_str( file_tab[i].filename, MAX_FILENAME );
        strcpy( file_tab[i].filename, dp->d_name);
        sprintf( tmp_path, "%s/%s", path, dp->d_name );
        if( stat( tmp_path, &file_tab[i].st ) == -1 )
        {
            printf("Funkcja stat() nie powiodła się dla: %s ; Błąd: %s\n", tmp_path, strerror(errno) );
            errno=0;
            continue;
        }
        else ++k;
        ++i;
    }
    if( k != n ){ perror("Błąd podczas zbierania informacji o plikach"); return; }

    /* sortowanie po czasie dostepu */
    if( flags->t )
        qsort(file_tab,n,sizeof(struct FILE_INFO),time_compare);

    /* zamienianie rozmiaru pliku w strukturze stat na czytelny dla ludzi */
    if( flags->h )
    {
        for( i = 0; i < n; ++i )
        {
            file_tab[i].h_size = (long)file_tab[i].st.st_size;
            if( flags->h )
            {
                for( k = 0; file_tab[i].h_size / 1024.0 > 1 && k < sizeof(units)/sizeof(units[0]) ; ++k )
                {
                    file_tab[i].h_size =  (double)file_tab[i].h_size / 1024.0;
                }

                file_tab[i].h_unit = units[k];
            }
        }
    }

    /* obliczam szerokosci kolumn dla odpowiedniego wyswietlania */
    if( flags->l )
    {
        for( i = 0; i < n ; ++i )
        {
            tmp[0] = numlen(file_tab[i].st.st_nlink);
            if( tmp[0] > field_width[0] )
                field_width[0] = tmp[0];

            tmp[1] = strlen(getpwuid(file_tab[i].st.st_uid)->pw_name);
            if( tmp[1] > field_width[1] )
                field_width[1] = tmp[1];

            tmp[2] = strlen(getgrgid(file_tab[i].st.st_gid)->gr_name);
            if( tmp[2] > field_width[2] )
                field_width[2] = tmp[2];

            if( flags-> h )
            {
                tmp[3] = numlen( file_tab[i].h_size );
                if( tmp[3] > field_width[3] )
                    field_width[3] = tmp[3];
            }
            else
            {
                tmp[3] = numlen( file_tab[i].st.st_size);
                if( tmp[3] > field_width[3] )
                    field_width[3] = tmp[3];
            }
        }
    }

    /* wyswietlanie informacji */
    for( i = 0; i < n; ++i )
    {
        if( flags->l )
        {
            /* typ pliku i uprawnienia*/
            char str[]="----------";
            type_and_perms( file_tab[i].st.st_mode, str );
            printf("%s ", str );

            /* liczba polaczen */
            printf("%*d ", field_width[0], file_tab[i].st.st_nlink );

            /* nazwa wlasciciela */
            printf("%-*s ", field_width[1], getpwuid(file_tab[i].st.st_uid)->pw_name);

            /* nazwa grupy */
            printf("%-*s", field_width[2],getgrgid(file_tab[i].st.st_gid)->gr_name);

            /* rozmiar w bajtach */
            if( flags->h )
            {
                if( file_tab[i].h_unit != '#' )
                    printf("%*.1lf%c ", field_width[3]+2, file_tab[i].h_size, file_tab[i].h_unit );
                else
                    printf("%*.lf ", field_width[3]+3, file_tab[i].h_size );

            }
            else printf("%*.1lu ", field_width[3], file_tab[i].st.st_size );

            /* czas ostatniej modyfikacji */
            struct tm * time = localtime(&file_tab[i].st.st_mtime);
            printf("%d-%02d-%02d %02d:%02d ", time->tm_year + 1900,
                   time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min);
        }
        /* nazwa pliku */
        printf("%s \n", file_tab[i].filename );
    }
    printf("\n");
    closedir( directory );

    /* wypisywanie katalogow w rekurencji */
    if( flags->r )
    {
        if ( ( directory = opendir(path) ) == NULL )
        {
            perror(path);
            return;
        }
        n = no_of_directories( path );
        struct stat st;
        struct FILE_INFO file_tab2[n];
        i = 0;
        k = 0;
        while( ( dp = readdir(directory) ) != NULL )
        {
            if( strcmp( dp->d_name, "." ) == 0 || strcmp( dp->d_name, ".." ) == 0 )
                continue;
            clear_str( tmp_path, MAX_PATH );
            clear_str( file_tab2[i].filename, MAX_FILENAME );
            sprintf( tmp_path, "%s/%s", path, dp->d_name );
            if( stat( tmp_path, &st ) == -1 )
            {
                printf("Funkcja stat() nie powiodła się dla: %s ; Błąd: %s\n", tmp_path, strerror(errno) );
                errno=0;
                continue;
            }
            if( S_ISDIR( st.st_mode ) )
            {
                strcpy( file_tab2[i].filename, tmp_path );
                file_tab2[i].st = st;
                ++k;
                ++i;
            }

        }
        if( k != n ){ perror("Błąd podczas zbierania informacji o katalogach"); return; }
        else
        {
            for( int i = 0; i < n; ++i )
                list( file_tab2[i].filename, flags );
        }
        closedir( directory );
    }
}
void check_for_options( struct flag_set *f, int *argc, char *argv[] )
{
    for( int i = 1; i < *argc; ++i )
    {
        if( argv[i][0] == '-' )
        {
            if( strcmp( argv[i], "--help") == 0 )
            {
                help();
                f->exit = 1;
                return;
            }
            else if( strcmp( argv[i], "--version") == 0 )
            {
                version();
                f->exit = 1;
                return;
            }
            else
            {
                for( int k = 1; k < strlen( argv[i] ); ++k )
                {
                    if( argv[i][k] == 'l' || argv[i][k] == 'L' )
                        f->l = 1;
                    else if( argv[i][k] == 't' || argv[i][k] == 'T' )
                        f->t = 1;
                    else if( argv[i][k] == 'h' || argv[i][k] == 'H' )
                        f->h = 1;
                    else if( argv[i][k] == 'a' || argv[i][k] == 'A' )
                        f->a = 1;
                    else if( argv[i][k] == 'r' || argv[i][k] == 'R' )
                        f->r = 1;
                    else
                    {
                        f->exit = 1;
                        printf("Niepoprawna opcja: %s\nUżyj opcji --help dla uzyskania informacji.\n", argv[i] );
                        return;
                    }
                }
            }
        }
    }
    /* ignoruje opcje -h gdy opcja -l nie jest podana */
    if( !f->l && f->h )
        f->h = 0;
}
int main(int argc, char *argv[] )
{
    char *path = NULL;
    int n=0;
    struct flag_set flags = {.a=0,.r=0,.l=0,.t=0,.h=0,.exit=0 };
    check_for_options( &flags, &argc, argv );
    if( flags.exit == 0 )
    {
        for( int i = 1; i < argc; ++i )
        {
            if( argv[i][0] != '-' )
            {
                ++n;
                path = argv[i];
                list( path, &flags );
            }
        }
        if( n == 0 )
            list( ".", &flags );
    }
    return 0;
}


