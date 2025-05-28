#include "file_reader.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define GET_BIT(value, bit_pos) (((value) >> (bit_pos)) & 1)

struct disk_t* disk_open_from_file(const char* nazwa_pliku) {
    if (!nazwa_pliku) {
        errno = EFAULT;
        return NULL;
    }

    struct disk_t* dysk = malloc(sizeof(struct disk_t));
    if (!dysk) {
        errno = ENOMEM;
        return NULL;
    }

    dysk->uchwyt_pliku = fopen(nazwa_pliku, "rb");
    if (!dysk->uchwyt_pliku) {
        free(dysk);
        errno = ENOENT;
        return NULL;
    }

    if (fseek(dysk->uchwyt_pliku, 0, SEEK_END) != 0) {
        fclose(dysk->uchwyt_pliku);
        free(dysk);
        errno = EIO;
        return NULL;
    }

    long rozmiar_pliku = ftell(dysk->uchwyt_pliku);
    if (rozmiar_pliku == -1) {
        fclose(dysk->uchwyt_pliku);
        free(dysk);
        errno = EIO;
        return NULL;
    }
    dysk->rozmiar_pliku = (uint64_t)rozmiar_pliku;


    if(fseek(dysk->uchwyt_pliku, 0, SEEK_SET) != 0) {
        fclose(dysk->uchwyt_pliku);
        free(dysk);
        errno = EIO;
        return NULL;
    }

    dysk->liczba_sektorow = dysk->rozmiar_pliku / 512;

    return dysk;
}

int disk_read(struct disk_t* pdysk, int32_t pierwszy_sektor, void* bufor, int32_t sektor_do_odczytu) {
    if (!pdysk || !bufor) {
        errno = EFAULT;
        return -1;
    }

    if (pierwszy_sektor + sektor_do_odczytu > (int32_t)pdysk->liczba_sektorow) {
        errno = ERANGE;
        return -1;
    }

    uint8_t* sprawdz = (uint8_t*)bufor;

    for (int32_t i = 0; i < sektor_do_odczytu; i++) {
        if (fseek(pdysk->uchwyt_pliku, (pierwszy_sektor + i) * 512, SEEK_SET) != 0 ||
            fread(sprawdz, 1, 512, pdysk->uchwyt_pliku) != 512)
        {
            if(feof(pdysk->uchwyt_pliku)){

                fseek(pdysk->uchwyt_pliku, 0, SEEK_SET);
            } else {
                fseek(pdysk->uchwyt_pliku, 0, SEEK_SET);
            }
            errno = EIO;

            return -1;
        }
        sprawdz += 512;
    }

    if(fseek(pdysk->uchwyt_pliku, 0, SEEK_SET) != 0){
        errno = EIO;
        return -1;
    }

    return sektor_do_odczytu;
}

int disk_close(struct disk_t* dysk) {
    if (!dysk) {
        errno = EFAULT;
        return -1;
    }
    if (!dysk->uchwyt_pliku) {
        free(dysk);
        errno = EINVAL;
        return -1;
    }
    if (fclose(dysk->uchwyt_pliku) != 0) {
        free(dysk);
        errno = EIO;
        return -1;
    }
    free(dysk);
    return 0;
}

int odczytaj_sektory(void *pdysk, uint32_t pierwszy_sektor, void *bufor, int32_t liczba_sektorow) {
    if (!pdysk || !bufor) {
        errno = EFAULT;
        return -1;
    }
    if (disk_read(pdysk, pierwszy_sektor, bufor, liczba_sektorow) != liczba_sektorow) {
        return -1;
    }
    return 0;
}
void* bezpieczny_malloc(size_t rozmiar) {
    void* sprawdz = malloc(rozmiar);
    if (!sprawdz) {
        errno = ENOMEM;
    } else {
        memset(sprawdz, 0, rozmiar);
    }
    return sprawdz;
}
void bezpieczny_free(void *sprawdz) {
    if (sprawdz != NULL) {
        free(sprawdz);
    }
}
struct volume_t* fat_open(struct disk_t* pdysk, uint32_t pierwszy_sektor) {
    if (!pdysk) {
        errno = EFAULT;
        return NULL;
    }

    struct volume_t* wolumin = NULL;
    unsigned char* bufor = NULL;
    struct RekordRozruchowy* rekord_rozruchowy = NULL;
    uint16_t* tablica_fat = NULL;
    uint8_t* katalog_glowny = NULL;
    struct WpisKataloguGlownego** wpisy_katalogu = NULL;
    uint64_t liczba_wpisow = 0;
    bool sukces = true;

    bufor = (unsigned char*)bezpieczny_malloc(512);
    if (sukces && !bufor) {
        errno = ENOMEM;
        sukces = false;
    }


    if (sukces && odczytaj_sektory(pdysk, pierwszy_sektor, bufor, 1) != 0) {
        errno = EIO;
        sukces = false;
    }

    if (sukces) {
        rekord_rozruchowy = (struct RekordRozruchowy*)bufor;
        if (rekord_rozruchowy->sygnatura_rozruchu != 0x29 ||
            rekord_rozruchowy->sygnatura_sektora_rozruchowego[0] != 0x55 ||
            rekord_rozruchowy->sygnatura_sektora_rozruchowego[1] != 0xAA) {
            errno = EINVAL;
            sukces = false;
        }
    }

    if (sukces) {
        wolumin = (struct volume_t*)bezpieczny_malloc(sizeof(struct volume_t));
        if (!wolumin) {
            errno = ENOMEM;
            sukces = false;
        }
    }

    if (sukces) {
        tablica_fat = (uint16_t*)bezpieczny_malloc((uint64_t)rekord_rozruchowy->sektorow_na_tablice_fat * 512);
        if (!tablica_fat) {
            errno = ENOMEM;
            sukces = false;
        }
    }


    if (sukces) {
        uint32_t pierwszy_sektor_fat = rekord_rozruchowy->sektory_zarezerwowane;
        uint32_t liczba_sektorow_fat = rekord_rozruchowy->sektorow_na_tablice_fat;
        if (odczytaj_sektory(pdysk, pierwszy_sektor_fat, tablica_fat, liczba_sektorow_fat) != 0) {
            errno = EIO;
            sukces = false;
        }
    }

    size_t rozmiar_katalogu_bajt = 0;
    if(sukces){
        rozmiar_katalogu_bajt = (uint64_t)rekord_rozruchowy->liczba_wpisow_katalogu_glownego * sizeof(struct WpisKataloguGlownego);
        katalog_glowny = (uint8_t*)bezpieczny_malloc(rozmiar_katalogu_bajt);
        if (!katalog_glowny) {
            errno = ENOMEM;
            sukces = false;
        }
    }
    uint32_t pierwszy_sektor_katalogu = 0;
    uint32_t rozmiar_katalogu_sektory = 0;

    if(sukces){
        pierwszy_sektor_katalogu = rekord_rozruchowy->sektory_zarezerwowane + (uint64_t)rekord_rozruchowy->liczba_tablic_fat * rekord_rozruchowy->sektorow_na_tablice_fat;
        rozmiar_katalogu_sektory = (rozmiar_katalogu_bajt + 512 - 1) / 512;

        if (odczytaj_sektory(pdysk, pierwszy_sektor_katalogu, katalog_glowny, rozmiar_katalogu_sektory) != 0) {
            errno = EIO;
            sukces = false;
        }
    }

    if(sukces) {
        uint64_t pojemnosc_wpisow = 100;
        wpisy_katalogu = (struct WpisKataloguGlownego**)bezpieczny_malloc(pojemnosc_wpisow * sizeof(struct WpisKataloguGlownego*));
        if(!wpisy_katalogu) {
            errno = ENOMEM;
            sukces = false;
        }

        struct WpisKataloguGlownego* wpis = (struct WpisKataloguGlownego*)katalog_glowny;
        for (uint16_t i = 0; sukces && i < rekord_rozruchowy->liczba_wpisow_katalogu_glownego; i++, wpis++) {
            if (wpis->nazwa_pliku[0] == 0) break;
            if (wpis->nazwa_pliku[0] == 0xE5 || (wpis->atrybuty & 0x0F) == 0x0F) continue;

            if (liczba_wpisow >= pojemnosc_wpisow) {
                struct WpisKataloguGlownego** new_wpisy_katalogu = (struct WpisKataloguGlownego**)realloc(wpisy_katalogu, (pojemnosc_wpisow + 100) * sizeof(struct WpisKataloguGlownego*));
                if (!new_wpisy_katalogu) {
                    errno = ENOMEM;
                    sukces = false;
                    break;
                }
                wpisy_katalogu = new_wpisy_katalogu;
                pojemnosc_wpisow += 100;
            }

            wpisy_katalogu[liczba_wpisow] = wpis;
            liczba_wpisow++;
        }
    }

    if (sukces)
    {
        uint64_t offset_do_katalogu = (uint64_t)rekord_rozruchowy->liczba_tablic_fat * rekord_rozruchowy->sektorow_na_tablice_fat;
        *wolumin = (struct volume_t){
                .tablica_fat = tablica_fat,
                .uchwyt_dysku = pdysk,
                .dane_katalogu_glownego = katalog_glowny,
                .rekord_rozruchowy = rekord_rozruchowy,
                .sektor_startowy_fat = rekord_rozruchowy->sektory_zarezerwowane,
                .rozmiar_klastra_bajt = (uint64_t)rekord_rozruchowy->sektorow_na_klaster * 512,
                .sektory_na_klaster = rekord_rozruchowy->sektorow_na_klaster,
                .sektor_startowy_danych = rekord_rozruchowy->sektory_zarezerwowane + rozmiar_katalogu_sektory + offset_do_katalogu,
                .liczba_wpisow_katalogu = liczba_wpisow,
                .wpisy_katalogu_glownego = wpisy_katalogu
        };
    }

    bezpieczny_free(bufor);

    if (!sukces) {
        bezpieczny_free(wolumin);
        bezpieczny_free(tablica_fat);
        bezpieczny_free(katalog_glowny);
        bezpieczny_free(wpisy_katalogu);
        return NULL;
    }

    return wolumin;
}

int fat_close(struct volume_t* pwolumin) {
    if (!pwolumin) {
        errno = EFAULT;
        return -1;
    }

    bezpieczny_free(pwolumin->tablica_fat);
    pwolumin->tablica_fat = NULL;

    bezpieczny_free(pwolumin->dane_katalogu_glownego);
    pwolumin->dane_katalogu_glownego = NULL;

    bezpieczny_free(pwolumin->wpisy_katalogu_glownego);
    pwolumin->wpisy_katalogu_glownego = NULL;

    bezpieczny_free(pwolumin);
    return 0;
}

void jaka_nazwa_pliku(struct WpisKataloguGlownego* wpis, char* nazwa_pliku) {
    uint16_t dlugosc_nazwy = 0;
    uint16_t dlugosc_rozszerzenia = 0;
    const char* wskaznik_nazwy = (const char*)wpis->nazwa_pliku;
    char* wskaznik_spacji = strchr(wskaznik_nazwy, ' ');
    if (wskaznik_spacji) {
        dlugosc_nazwy = wskaznik_spacji - wskaznik_nazwy;
    } else {
        dlugosc_nazwy = strnlen(wskaznik_nazwy, 8);
    }
    memcpy(nazwa_pliku, wskaznik_nazwy, dlugosc_nazwy);
    nazwa_pliku[dlugosc_nazwy] = '\0';

    if (wpis->rozszerzenie_pliku[0] != ' ') {
        const char* wskaznik_rozszerzenia = (const char*)wpis->rozszerzenie_pliku;
        wskaznik_spacji = strchr(wskaznik_rozszerzenia, ' ');
        if (wskaznik_spacji) {
            dlugosc_rozszerzenia = wskaznik_spacji - wskaznik_rozszerzenia;
        } else {
            dlugosc_rozszerzenia = strnlen(wskaznik_rozszerzenia, 3);
        }
        strncat(nazwa_pliku, ".", 2);
        strncat(nazwa_pliku, wskaznik_rozszerzenia, dlugosc_rozszerzenia);
    }
}
void zwolnij_liste_klastrow(struct Klaster* pierwszy_klaster) {
    for (struct Klaster* aktualny = pierwszy_klaster; aktualny != NULL; ) {
        struct Klaster* nastepny = aktualny->nastepny_klaster;
        bezpieczny_free(aktualny);
        aktualny = nastepny;
    }
}
bool czy_wpis_jest_katalogiem(const struct WpisKataloguGlownego* wpis) {
    return (wpis->atrybuty & 0x18) != 0;
}
struct WpisKataloguGlownego* znajdz_wpis_katalogu(struct volume_t* pwolumin, const char* nazwa_pliku) {
    struct WpisKataloguGlownego* aktualny_wpis;
    char nazwa_pliku_z_wpisu[15];

    for (uint16_t i = 0; i < pwolumin->liczba_wpisow_katalogu;i++) {
        aktualny_wpis = pwolumin->wpisy_katalogu_glownego[i];
        jaka_nazwa_pliku(aktualny_wpis, nazwa_pliku_z_wpisu);

        if (strcmp(nazwa_pliku_z_wpisu, nazwa_pliku) == 0) {
            return aktualny_wpis;
        }
    }
    return NULL;
}
struct Klaster* utworz_klaster(struct volume_t* pwolumin, uint16_t numer_klastra) {
    struct Klaster* klaster = bezpieczny_malloc(sizeof(struct Klaster));
    if (!klaster){
        return NULL;
    }
    *klaster = (struct Klaster) {
            .numer_klastra = numer_klastra,
            .sektor_startowy_klastra = ((numer_klastra - 2) * pwolumin->sektory_na_klaster) + pwolumin->sektor_startowy_danych,
            .nastepny_klaster = NULL
    };
    return klaster;
}
struct Klaster* zbuduj_liste_klastrow(struct volume_t* pwolumin, uint16_t pierwszy_klaster_niski) {
    struct Klaster* pierwszy_klaster = utworz_klaster(pwolumin, pierwszy_klaster_niski);
    if (!pierwszy_klaster) {
        return NULL;
    }

    struct Klaster* aktualny_klaster = pierwszy_klaster;

    for (uint16_t i = pierwszy_klaster_niski; i < 0x0FFF8; ) {
        i = pwolumin->tablica_fat[i];

        if (i >= 0x0FFF8) {
            break;
        }

        struct Klaster* nowy_klaster = utworz_klaster(pwolumin, i);
        if (!nowy_klaster) {
            zwolnij_liste_klastrow(pierwszy_klaster);
            return NULL;
        }

        aktualny_klaster->nastepny_klaster = nowy_klaster;
        aktualny_klaster = nowy_klaster;
    }

    return pierwszy_klaster;
}
struct file_t* file_open(struct volume_t* wolumin, const char* nazwa_pliku) {
    if (!wolumin || !nazwa_pliku) {
        errno = EFAULT;
        return NULL;
    }

    struct WpisKataloguGlownego* wpis_katalogu = znajdz_wpis_katalogu(wolumin, nazwa_pliku);
    if (!wpis_katalogu) {
        errno = ENOENT;
        return NULL;
    }

    if (czy_wpis_jest_katalogiem(wpis_katalogu)) {
        errno = EISDIR;
        return NULL;
    }

    struct file_t* plik = (struct file_t*)bezpieczny_malloc(sizeof(struct file_t));
    if (!plik) {
        errno = ENOMEM;
        return NULL;
    }

    struct Klaster* pierwszy_klaster = zbuduj_liste_klastrow(wolumin, wpis_katalogu->pierwszy_klaster_niski);
    if (!pierwszy_klaster) {
        bezpieczny_free(plik);
        errno = ENOMEM;
        return NULL;
    }

    memset(plik, 0, sizeof(struct file_t));

    plik->atrybuty_pliku = wpis_katalogu->atrybuty;
    plik->rozmiar_pliku = wpis_katalogu->rozmiar_pliku;
    plik->aktualna_pozycja = 0;
    plik->uchwyt_woluminu = wolumin;
    plik->pierwszy_klaster = pierwszy_klaster;

    if (memcpy(plik->nazwa_pliku, wpis_katalogu->nazwa_pliku, sizeof(plik->nazwa_pliku)) == NULL)
    {
        bezpieczny_free(plik);
        errno = EFAULT;
        return NULL;

    }
    return plik;
}

int file_close(struct file_t* strumien) {
    if (!strumien) {
        errno = EFAULT;
        return -1;
    }
    zwolnij_liste_klastrow(strumien->pierwszy_klaster);
    bezpieczny_free(strumien);
    return 0;
}

uint16_t oblicz_przesuniecie_w_klastrze(struct file_t* strumien, struct Klaster* aktualny_klaster) {
    size_t numer_aktualnego_klastra = 0;
    size_t rozmiar_klastra = strumien->uchwyt_woluminu->rozmiar_klastra_bajt;

    for (struct Klaster* tmp_klaster = strumien->pierwszy_klaster; tmp_klaster != NULL;
         tmp_klaster = tmp_klaster->nastepny_klaster, numer_aktualnego_klastra++) {
        if (tmp_klaster == aktualny_klaster) {
            break;
        }
    }
    size_t przesuniecie_w_klastrze = strumien->aktualna_pozycja - (numer_aktualnego_klastra * rozmiar_klastra);

    return przesuniecie_w_klastrze;
}
uint16_t oblicz_bajt_kop(struct file_t* strum, size_t bajty_czyt, size_t bajty_razem, size_t przes_klast) {
    size_t bajty_klast = strum->uchwyt_woluminu->rozmiar_klastra_bajt;
    size_t bajty_kop = bajty_razem - bajty_czyt;

    if (bajty_klast > przes_klast) {
        bajty_klast -= przes_klast;
    } else {
        bajty_klast = 0;
    }

    bajty_kop = MIN(bajty_kop, bajty_klast);

    size_t bajty_pliku = strum->rozmiar_pliku - strum->aktualna_pozycja;
    if (strum->aktualna_pozycja < strum->rozmiar_pliku) {
        bajty_kop = MIN(bajty_kop, bajty_pliku);
    }

    return (uint16_t)bajty_kop;
}
uint16_t czytaj_dane_z_klastra(struct file_t* strumien, void* wskaznik, uint8_t* bufor, size_t bajtow_przeczytanych, size_t bajtow_do_skopiowania, size_t przesuniecie_klastra) {
    if (bajtow_do_skopiowania == 0 || strumien->aktualna_pozycja >= strumien->rozmiar_pliku) {
        return bajtow_przeczytanych;
    }

    uint8_t* docelowy = (uint8_t*)wskaznik + bajtow_przeczytanych;
    uint8_t* zrodlowy = bufor + przesuniecie_klastra;

    memcpy(docelowy, zrodlowy, bajtow_do_skopiowania);

    strumien->aktualna_pozycja += bajtow_do_skopiowania;
    bajtow_przeczytanych += bajtow_do_skopiowania;

    return bajtow_przeczytanych;
}
size_t file_read(void* wskaznik, size_t wielkosc, size_t nmemb, struct file_t* strumien) {
    if (!wskaznik || !strumien) {
        errno = EFAULT;
        return -1;
    }
    if (strumien->aktualna_pozycja >= strumien->rozmiar_pliku) {
        return 0;
    }
    uint8_t* buf_kl = bezpieczny_malloc(strumien->uchwyt_woluminu->rozmiar_klastra_bajt);
    uint16_t odcz_b = 0;
    uint16_t do_odcz_b = wielkosc * nmemb;

    if (!buf_kl) {
        errno = ENOMEM;
        return -1;
    }

    struct Klaster* kl = strumien->pierwszy_klaster;

    for (; odcz_b < do_odcz_b;) {
        if (!kl) {
            if(strumien->aktualna_pozycja < strumien->rozmiar_pliku) {
                errno = ENXIO;
                bezpieczny_free(buf_kl);
                return -1;
            }
            break;
        }
        if (disk_read(strumien->uchwyt_woluminu->uchwyt_dysku, kl->sektor_startowy_klastra, buf_kl, strumien->uchwyt_woluminu->sektory_na_klaster) == -1) {
            bezpieczny_free(buf_kl);
            return -1;
        }
        uint16_t prz_kl = oblicz_przesuniecie_w_klastrze(strumien, kl);

        uint16_t do_kop_b = oblicz_bajt_kop(strumien, odcz_b, do_odcz_b, prz_kl);

        odcz_b = czytaj_dane_z_klastra(strumien, wskaznik, buf_kl, odcz_b, do_kop_b, prz_kl);

        kl = kl->nastepny_klaster;
    }
    bezpieczny_free(buf_kl);
    return odcz_b / wielkosc;
}

int32_t file_seek(struct file_t* strumien, int32_t offset, int whence) {
    if (!strumien) {
        errno = EFAULT;
        return -1;
    }

    uint32_t nowa_poz;
    switch (whence) {
        case SEEK_SET:
            if (offset < 0) {
                errno = EINVAL;
                return -1;
            }
            nowa_poz = offset;
            break;
        case SEEK_CUR:
            if ((int64_t)strumien->aktualna_pozycja + offset < 0) {
                errno = ENXIO;
                return -1;
            }

            nowa_poz = (int64_t)strumien->aktualna_pozycja + offset;
            break;
        case SEEK_END:
            if ((int64_t)strumien->rozmiar_pliku + offset < 0) {
                errno = ENXIO;
                return -1;
            }
            nowa_poz = strumien->rozmiar_pliku + offset;
            break;
        default:
            errno = EINVAL;
            return -1;
    }

    if (nowa_poz > strumien->rozmiar_pliku) {
        errno = ENXIO;
        return -1;
    }

    strumien->aktualna_pozycja = nowa_poz;
    return nowa_poz;
}

struct dir_t* utworz_katalog(struct volume_t* pwolumin, struct WpisKataloguGlownego* wpisy[], int liczba_wpisow) {
    struct dir_t* katalog = bezpieczny_malloc(sizeof(struct dir_t));
    if (!katalog) {
        return NULL;
    }
    *katalog = (struct dir_t) {
            .uchwyt_woluminu = pwolumin,
            .aktualny_indeks_wpisu = 0,
            .wpisy_katalogu = wpisy,
            .liczba_wpisow = liczba_wpisow
    };
    return katalog;
}
struct dir_t* dir_open(struct volume_t* pwolumin, const char* sciezka) {
    if (!pwolumin || !sciezka) {
        errno = EFAULT;
        return NULL;
    }
    if (strcmp(sciezka, "\\") == 0) {
        struct dir_t* katalog = utworz_katalog(pwolumin, (struct WpisKataloguGlownego **) (struct WpisKataloguGlownego *) pwolumin->wpisy_katalogu_glownego, pwolumin->liczba_wpisow_katalogu);
        if (!katalog) {
            errno = ENOMEM;
            return NULL;
        }
        return katalog;
    }

    struct WpisKataloguGlownego* wpis = znajdz_wpis_katalogu(pwolumin, sciezka);
    if (!wpis) {
        errno = ENOENT;
        return NULL;
    }

    if (!czy_wpis_jest_katalogiem(wpis)) {
        errno = ENOTDIR;
        return NULL;
    }
    struct dir_t* katalog = utworz_katalog(pwolumin, (struct WpisKataloguGlownego **) (struct WpisKataloguGlownego *) pwolumin->wpisy_katalogu_glownego, pwolumin->liczba_wpisow_katalogu);
    if (!katalog) {
        errno = ENOMEM;
        return NULL;
    }
    return katalog;
}

int dir_read(struct dir_t* pkatalog, struct dir_entry_t* pwpis) {
    if (!pkatalog || !pwpis) {
        errno = EFAULT;
        return -1;
    }
    if (!pkatalog->wpisy_katalogu) {
        errno = EIO;
        return -1;
    }
    if (pkatalog->aktualny_indeks_wpisu >= pkatalog->liczba_wpisow) {
        return 1;
    }
    struct WpisKataloguGlownego* wpis_katalogu_glownego = pkatalog->wpisy_katalogu[pkatalog->aktualny_indeks_wpisu];

    if (!wpis_katalogu_glownego) {
        errno = EIO;
        return -1;
    }

    struct dir_entry_t sprawdz = {
            .rozmiar = wpis_katalogu_glownego->rozmiar_pliku,
            .czy_zarchiwizowany = GET_BIT(wpis_katalogu_glownego->atrybuty, 5),
            .czy_katalog = czy_wpis_jest_katalogiem(wpis_katalogu_glownego),
            .czy_tylko_do_odczytu = GET_BIT(wpis_katalogu_glownego->atrybuty, 0),
            .czy_ukryty = GET_BIT(wpis_katalogu_glownego->atrybuty, 1),
            .czy_systemowy = GET_BIT(wpis_katalogu_glownego->atrybuty, 2)
    };
    jaka_nazwa_pliku(wpis_katalogu_glownego, sprawdz.name);
    *pwpis = sprawdz;

    pkatalog->aktualny_indeks_wpisu++;
    return 0;
}

int dir_close(struct dir_t* pkatalog) {
    if (!pkatalog) {
        errno = EFAULT;
        return -1;
    }

    bezpieczny_free(pkatalog);
    return 0;
}
