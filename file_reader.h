#ifndef BADAQU_FAT16
#define BADAQU_FAT16

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

// STRUKTURY DANYCH

//DYSK
struct disk_t {
    FILE* uchwyt_pliku;                                        // Uchwyt do otwartego pliku
    uint64_t rozmiar_pliku;                                    // Długość pliku w bajtach
    uint64_t liczba_sektorow;                                  // Liczba sektorów na dysku
};

//REKORDU ROZRUCHOWY
struct RekordRozruchowy {
    uint8_t instrukcja_skoku[3];                               // Instrukcja skoku
    uint8_t nazwa_oem[8];                                      // Nazwa OEM
    uint16_t bajtow_na_sektor;                                 // Bajtów na sektor (512)
    uint8_t sektorow_na_klaster;                               // Sektorów na klaster
    uint16_t sektory_zarezerwowane;                            // Liczba sektorów zarezerwowanych
    uint8_t liczba_tablic_fat;                                 // Liczba tablic FAT
    uint16_t liczba_wpisow_katalogu_glownego;                  // Liczba wpisów w katalogu głównym
    uint16_t calkowita_liczba_sektorow_16bit;                  // Całkowita liczba sektorów (16-bit)
    uint8_t deskryptor_nosnika;                                // Deskryptor nośnika
    uint16_t sektorow_na_tablice_fat;                          // Sektorów na tablicę FAT
    uint16_t sektorow_na_sciezke;                              // Sektorów na ścieżkę
    uint16_t liczba_glowic;                                    // Liczba głowic
    uint32_t ukrytych_sektorow;                                // Ukrytych sektorów
    uint32_t calkowita_liczba_sektorow_32bit;                  // Całkowita liczba sektorów (32-bit)
    uint8_t numer_napedu;                                      // Numer napędu
    uint8_t zarezerwowane2;                                    // Rezerwowe
    uint8_t sygnatura_rozruchu;                                // Sygnatura rozruchu
    uint32_t id_woluminu;                                      // Identyfikator woluminu
    uint8_t etykieta_woluminu[11];                             // Etykieta woluminu
    uint8_t typ_systemu_plikow[8];                             // Typ systemu plików
    uint8_t kod_rozruchowy[448];                               // Kod rozruchowy
    uint8_t sygnatura_sektora_rozruchowego[2];                 // Sygnatura sektora rozruchowego
} __attribute__((packed));

// STRUKTURA WPISU KATALOGU GŁÓWNEGO
struct WpisKataloguGlownego {
    uint8_t nazwa_pliku[8];                                    // Nazwa pliku
    uint8_t rozszerzenie_pliku[3];                             // Rozszerzenie pliku
    uint8_t atrybuty;                                          // Atrybuty pliku
    uint8_t zarezerwowane1;                                    // Rezerwowe
    uint8_t czas_utworzenia_dziesiatki_sekund;                 // Dziesiętne części sekundy utworzenia
    uint16_t czas_utworzenia;                                  // Czas utworzenia
    uint16_t data_utworzenia;                                  // Data utworzenia
    uint16_t data_ostatniego_dostepu;                          // Data ostatniego dostępu
    uint16_t zarezerwowane2;                                   // Rezerwowe
    uint16_t czas_ostatniej_modyfikacji;                       // Czas ostatniej modyfikacji
    uint16_t data_ostatniej_modyfikacji;                       // Data ostatniej modyfikacji
    uint16_t pierwszy_klaster_niski;                           // Numer pierwszego klastra (niski)
    uint32_t rozmiar_pliku;                                    // Rozmiar pliku
};

// STRUKTURA WOLUMINU
struct volume_t {
    uint32_t rozmiar_klastra_bajt;                             // Bajtów na klaster
    uint32_t sektor_startowy_danych;                           // Sektor startowy danych
    struct WpisKataloguGlownego** wpisy_katalogu_glownego;     // Tablica wpisów katalogu głównego
    uint32_t sektor_startowy_fat;                              // Sektor startowy FAT
    struct disk_t* uchwyt_dysku;                               // Uchwyt do dysku
    uint8_t* dane_katalogu_glownego;                           // Dane katalogu głównego
    uint32_t sektory_na_klaster;                               // Sektorów na klaster
    uint16_t* tablica_fat;                                     // Tablica FAT
    struct RekordRozruchowy* rekord_rozruchowy;                // Rekord rozruchowy
    uint64_t liczba_wpisow_katalogu;                           // Liczba wpisów w katalogu głównym
};

// STRUKTURA KLASTRA
struct Klaster {
    uint32_t sektor_startowy_klastra;                          // Sektor startowy klastra
    struct Klaster* nastepny_klaster;                          // Wskaźnik na następny klaster
    uint32_t numer_klastra;                                    // Numer klastra
};

// STRUKTURA PLIKU
struct file_t {
    char nazwa_pliku[8];                                       // Nazwa pliku
    uint32_t rozmiar_pliku;                                    // Rozmiar pliku
    struct Klaster* pierwszy_klaster;                          // Pierwszy klaster pliku
    uint32_t aktualna_pozycja;                                 // Aktualna pozycja w pliku
    uint16_t atrybuty_pliku;                                   // Atrybuty pliku
    struct volume_t* uchwyt_woluminu;                          // Uchwyt do woluminu
};

// STRUKTURA KATALOGU
struct dir_t {
    uint16_t liczba_wpisow;                                    // Liczba wpisów w katalogu
    struct volume_t* uchwyt_woluminu;                          // Uchwyt do woluminu
    uint16_t aktualny_indeks_wpisu;                            // Aktualny indeks wpisu
    struct WpisKataloguGlownego** wpisy_katalogu;              // Tablica wpisów katalogu
};

// STRUKTURA WPISU KATALOGU
struct dir_entry_t {
    char name[9];                                             // Nazwa pliku lub katalogu
    uint32_t rozmiar;                                          // Rozmiar pliku lub katalogu
    bool czy_zarchiwizowany;                                   // Czy zarchiwizowany
    bool czy_katalog;                                          // Czy katalog
    bool czy_tylko_do_odczytu;                                 // Czy tylko do odczytu
    bool czy_ukryty;                                           // Czy ukryty
    bool czy_systemowy;                                        // Czy systemowy
};

// OPERACJE

//OPERACJE POMOCNICZE
int odczytaj_sektory(void *pdysk, uint32_t pierwszy_sektor, void *bufor, int32_t liczba_sektorow);
void* bezpieczny_malloc(size_t rozmiar);
void bezpieczny_free(void *sprawdz);
void jaka_nazwa_pliku(struct WpisKataloguGlownego* wpis, char* nazwa_pliku);
void zwolnij_liste_klastrow(struct Klaster* pierwszy_klaster);
bool czy_wpis_jest_katalogiem(const struct WpisKataloguGlownego* wpis);
struct WpisKataloguGlownego* znajdz_wpis_katalogu(struct volume_t* pwolumin, const char* nazwa_pliku);
struct Klaster* utworz_klaster(struct volume_t* pwolumin, uint16_t numer_klastra);
struct Klaster* zbuduj_liste_klastrow(struct volume_t* pwolumin, uint16_t pierwszy_klaster_niski);
uint16_t oblicz_przesuniecie_w_klastrze(struct file_t* strumien, struct Klaster* aktualny_klaster);
uint16_t oblicz_bajt_kop(struct file_t* strum, size_t bajty_czyt, size_t bajty_razem, size_t przes_klast);
uint16_t czytaj_dane_z_klastra(struct file_t* strumien, void* wskaznik, uint8_t* bufor, size_t bajtow_przeczytanych, size_t bajtow_do_skopiowania, size_t przesuniecie_klastra);
struct dir_t* utworz_katalog(struct volume_t* pwolumin, struct WpisKataloguGlownego* wpisy[], int liczba_wpisow);
// OPERACJE DYSKU
// Funkcja przyjmuje nazwę pliku z obrazem urządzenia blokowego, o długości bloku równej 512 bajtów, na blok i otwiera go.
struct disk_t* disk_open_from_file(const char* nazwa_pliku);
// Funkcja wczytuje sectors_to_read bloków, zaczynając od bloku first_sector. Źródłem jest urządzenie pdisk a wczytane bloki zapisywane są do bufora buffer.
int disk_read(struct disk_t* pdysk, int32_t pierwszy_sektor, void* bufor, int32_t sektor_do_odczytu);
// Funkcja zamyka urządzenie pdisk i zwalnia pamięć struktury.
int disk_close(struct disk_t* dysk);

// OPERACJE FAT
// Funkcja otwiera i sprawdza wolumin FAT, dostępny na urządzeniu pdisk, zaczynając od sektora first_sector.
struct volume_t* fat_open(struct disk_t* pdysk, uint32_t pierwszy_sektor);
// Funkcja zamyka wolumin pvolume i zwalnia pamięć struktury.
int fat_close(struct volume_t* pwolumin);

// OPERACJE PLIKU
// Funkcja otwiera plik file_name.
struct file_t* file_open(struct volume_t* pwolumin, const char* nazwa_pliku);
// Funkcja zamyka plik stream i zwalnia pamięć struktury.
int file_close(struct file_t* strumien);
// Funkcja file_read odczytuje nmemb elementów danych, każdy o długości size bajtów. Odczyt danych rozpoczyna się na pozycji oraz z pliku, danego wskaźnikiem stream.
size_t file_read(void* wskaznik, size_t wielkosc, size_t nmemb, struct file_t* strumien);
// Funkcja ustawia pozycję w pliku stream.
int32_t file_seek(struct file_t* strumien, int32_t offset, int whence);

// OPERACJE KATALOGU
// Funkcja otwiera katalog, dany ścieżką dir_path.
struct dir_t* dir_open(struct volume_t* pwolumin, const char* sciezka);
// Funkcja dir_read() zapisuje informacje o następnym poprawnym wpisie w katalogu pdir do struktury informacyjnej dir_entry_t, dostarczonej wskaźnikiem pentry.
int dir_read(struct dir_t* pkatalog, struct dir_entry_t* pwpis);
// Funkcja zamyka katalog pdir i zwalnia pamięć struktury.
int dir_close(struct dir_t* pkatalog);




#endif // BADAQU_FAT16
