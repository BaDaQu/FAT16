#include "file_reader.h"

int main() {
    const char* nazwa_pliku_dysku = "dysk.img";
    struct disk_t* dysk = disk_open_from_file(nazwa_pliku_dysku);
    if (!dysk) {
        perror("disk_open_from_file failed");
        return 1;
    }
    printf("Dysk otwarty pomyślnie, rozmiar: %lu bajtów, sektorów: %lu\n", dysk->rozmiar_pliku, dysk->liczba_sektorow);

    unsigned char sektor_bufor[512];
    int odczytane_sektory = disk_read(dysk, 0, sektor_bufor, 1);
    if (odczytane_sektory != 1) {
        perror("disk_read failed");
        disk_close(dysk);
        return 1;
    }
    printf("Odczytano sektor z dysku.\n");

    struct volume_t* wolumin = fat_open(dysk, 0);
    if (!wolumin) {
        perror("fat_open failed");
        disk_close(dysk);
        return 1;
    }
    printf("Wolumin FAT otwarty pomyślnie.\n");

    const char* nazwa_pliku = "FILE1   TXT";
    struct file_t* plik = file_open(wolumin, nazwa_pliku);
    if (!plik) {
        perror("file_open failed");
        fat_close(wolumin);
        disk_close(dysk);
        return 1;
    }
    printf("Plik %s otwarty pomyślnie, rozmiar: %u bajtów.\n", nazwa_pliku, plik->rozmiar_pliku);

    unsigned char plik_bufor[100];
    size_t przeczytane_bajty = file_read(plik_bufor, 1, 10, plik);
    if (przeczytane_bajty != 10) {
        perror("file_read failed");
        file_close(plik);
        fat_close(wolumin);
        disk_close(dysk);
        return 1;
    }
    printf("Przeczytano %zu bajtów z pliku.\n", przeczytane_bajty);

    int32_t pozycja = file_seek(plik, 5, SEEK_SET);
    if(pozycja != 5)
    {
        perror("file_seek failed");
        file_close(plik);
        fat_close(wolumin);
        disk_close(dysk);
        return 1;
    }
    printf("Przesunięto pozycję w pliku na: %d.\n", pozycja);
    przeczytane_bajty = file_read(plik_bufor, 1, 5, plik);
    if (przeczytane_bajty != 5) {
        perror("file_read failed");
        file_close(plik);
        fat_close(wolumin);
        disk_close(dysk);
        return 1;
    }
    printf("Przeczytano %zu bajtów z pliku.\n", przeczytane_bajty);
    file_close(plik);

    struct dir_t* katalog = dir_open(wolumin, "\\");
    if (!katalog) {
        perror("dir_open failed");
        fat_close(wolumin);
        disk_close(dysk);
        return 1;
    }
    printf("Katalog otwarty pomyślnie.\n");

    struct dir_entry_t wpis_katalogu;
    int wynik_odczytu;
    int liczba_wpisow = 0;
    while((wynik_odczytu = dir_read(katalog, &wpis_katalogu)) == 0)
    {
        printf("Nazwa: %s, Rozmiar: %u, Katalog: %s\n", wpis_katalogu.name, wpis_katalogu.rozmiar, wpis_katalogu.czy_katalog ? "Tak" : "Nie");
        liczba_wpisow++;
    }

    if (wynik_odczytu == -1) {
        perror("dir_read failed");
        dir_close(katalog);
        fat_close(wolumin);
        disk_close(dysk);
        return 1;
    }

    printf("Odczytano %d wpisów z katalogu.\n", liczba_wpisow);

    dir_close(katalog);

    if (fat_close(wolumin) != 0) {
        perror("fat_close failed");
        disk_close(dysk);
        return 1;
    }
    printf("Wolumin FAT zamknięty.\n");

    if (disk_close(dysk) != 0) {
        perror("disk_close failed");
        return 1;
    }
    printf("Dysk zamknięty.\n");

    printf("Wszystkie testy zakończone pomyślnie!\n");
    return 0;
}
