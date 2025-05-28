# Czytnik Systemu Plików FAT16

## Opis Projektu

Projekt ten realizuje implementację parsera dla kontenerów plikowych zapisanych w formacie systemu plików FAT16. Głównym celem jest umożliwienie operacji wyłącznie odczytu danych z obrazu dysku sformatowanego w tym systemie. Architektura rozwiązania opiera się na modułowym podejściu, gdzie poszczególne warstwy abstrakcji pozwalają na interakcję z systemem plików na różnych poziomach szczegółowości.

### Dostęp do Urządzenia Blokowego

Kluczowym elementem systemu jest zaimplementowane API do obsługi urządzenia blokowego. W kontekście tego projektu, urządzenie blokowe jest reprezentowane przez plik na dysku twardym (obraz systemu plików). Dedykowane funkcje pozwalają na otwieranie takiego "dysku", czytanie określonej liczby sektorów (bloków o rozmiarze 512 bajtów) z zadanej pozycji oraz zamykanie urządzenia. Ta warstwa abstrakcji jest fundamentalna, ponieważ ukrywa plikowy charakter nośnika i prezentuje go jako sekwencję bloków, co jest typowe dla interakcji z fizycznymi urządzeniami pamięci masowej.

### Obsługa Woluminu FAT16

Na bazie API urządzenia blokowego, zaimplementowano funkcjonalność otwierania i zamykania woluminów FAT16. Proces otwierania woluminu obejmuje wczytanie i weryfikację kluczowych struktur systemu plików, przede wszystkim supersektora (Boot Sector). Sprawdzana jest poprawność sygnatur oraz podstawowych parametrów geometrycznych woluminu, aby upewnić się, że mamy do czynienia z prawidłowym obrazem FAT16. Informacje odczytane z supersektora są następnie wykorzystywane do interpretacji pozostałych części systemu plików, takich jak tablica alokacji plików (FAT) oraz katalog główny.

### Operacje na Plikach i Katalogach

Projekt dostarcza również zestaw funkcji API umożliwiających wykonywanie podstawowych operacji na plikach i katalogach znajdujących się na woluminie FAT16. Dostępne są funkcje pozwalające na otwieranie plików po ich nazwie, czytanie danych z otwartych plików, przeszukiwanie (zmianę aktualnej pozycji odczytu w pliku – seek) oraz zamykanie plików. Podobnie, zaimplementowano mechanizmy do otwierania katalogów, iteracyjnego odczytywania kolejnych wpisów katalogowych (informacji o plikach i podkatalogach) oraz zamykania katalogów.

### Ograniczenia i Standardy

Zgodnie z wymaganiami projektowymi na ocenę 3.0, funkcjonalność operacji na plikach i katalogach jest ograniczona wyłącznie do obsługi katalogu głównego. Oznacza to, że operacje na podkatalogach nie są implementowane ani testowane w tej wersji.

Wszystkie funkcje API zostały zaprojektowane tak, aby sygnalizować błędy w sposób zgodny ze standardem POSIX. W przypadku wystąpienia błędu, odpowiedni kod błędu jest ustawiany w globalnej zmiennej `errno`, co pozwala na standardową obsługę sytuacji wyjątkowych przez kod wywołujący.

### Struktury Danych i Plik Nagłówkowy

Definicje wszystkich niezbędnych struktur danych (takich jak `disk_t` dla dysku, `volume_t` dla woluminu, `file_t` dla pliku, `dir_t` dla katalogu oraz `dir_entry_t` dla wpisu katalogowego) oraz prototypy wszystkich publicznych funkcji API znajdują się w pliku nagłówkowym `file_reader.h`. Zapewnia to spójny interfejs dla użytkownika biblioteki.

Celem nadrzędnym projektu jest stworzenie solidnego, niskopoziomowego czytnika systemu plików FAT16. Taka implementacja może stanowić wartościową bazę do dalszego rozwoju i implementacji bardziej zaawansowanych funkcjonalności, takich jak obsługa podkatalogów, operacje zapisu czy wsparcie dla innych wariantów systemu FAT.
