# TFTP Klient + Server

## Autor

Kryštof Paulík
(xpauli08)

## Datum vytvoření

20. listopadu 2023

## Popis

Programy `tftp-server` a `tftp-client` jsou implementace serveru a klienta pro přenos souborů pomocí protokolu TFTP (Trivial File Transfer Protocol). Byly vytvořeny v souladu se speficikací TFTP dle [RFC1350](https://datatracker.ietf.org/doc/html/rfc1350). Program `tftp-server` navíc podporuje následující rozšíření:

- TFTP Option Extension - [RFC2347](https://datatracker.ietf.org/doc/html/rfc2347)
- TFTP Blocksize Option - [RFC2348](https://datatracker.ietf.org/doc/html/rfc2348)
- TFTP Timeout Interval and Transfer Size Options - [RFC2349](https://datatracker.ietf.org/doc/html/rfc2349)

### Spuštění programů

#### tftp-client

Komunikuje se vzdáleným serverem pro přenos souborů. Spouští se s následujícími argumenty:

    tftp-client -h hostname [-p port] [-f filepath] -t dest_filepath

- -h IP adresa/doménový název vzdáleného serveru
- -p port vzdáleného serveru
  pokud není specifikován předpokládá se výchozí dle specifikace
- -f cesta ke stahovanému souboru na serveru (download)
  pokud není specifikován používá se obsah stdin (upload)
- -t cesta, pod kterou bude soubor na vzdáleném serveru/lokálně uložen

#### tftp-server

Přijímá příchozí spojení žádající o přenos souborů. Spouští se s následujícími argumenty:

    tftp-server [-p port] root_dirpath

- -p místní port, na kterém bude server očekávat příchozí spojení
- cesta k adresáři, pod kterým se budou ukládat příchozí soubory

### Známá omezení programů

#### tftp-client

- klient nemá implementovanou podporu timeout

#### tftp-server

- když po odeslání OACK packetu server neobdrží ACK packet, neposílá OACK packet opakovaně (v této části není implementován timeout)

- server nemá implementovanou podporu módu netascii

### Seznam odevzdaných souborů

- _src/_ (složka obsahující zdrojové a hlavičkové soubory)
  - _tftp-client.c_ (zdrojový soubor klientské aplikace)
  - _tftp-server.c_ (zdrojový soubor serverové aplikace)
  - _tftp-functions.c_ (zdrojový soubor obsahující definice společných funkcí)
  - _tftp-functions.h_ (hlavičkový soubor obsahující mimo jiné konstanty a deklarace funkcí)
- _bin/_ (složka, kde budou uloženy binární soubory vytvořené pomocí Makefile)
- _Makefile_ (soubor sloužící k překladu zdrojových souborů na soubory binární)
- _README_ (tento soubor)
- _manual.pdf_ (soubor obsahující dokumentaci k projektu)

### Příklady spuštění

#### tftp-server

Nastavení root directory na `../slozka` a naslouchá na portu `2000`

    tftp-server ../slozka -p 2000

Nastavení root directory na `../slozka`, port neuveden, proto bude server naslouchat na defaultním portu `69`

    tftp-server ../slozka

#### tftp-client

Upload na server s doménovým jménem `localhost`, port není uveden, takže se předpokládá defaultní port `69`, soubor bude uložen v root directory pod `novy-obrazek.png`. Soubor je čten ze stdin.

    tftp-client -h localhost -t novy-obrazek.png < obrazek.png

Download ze serveru s IPv4 adresou `127.0.0.1`, port `2000`, cesta ke stahovanému souboru je `stahovany-obrazek.png`, soubor bude na straně klienta uložen pod `stazeny-obrazek.png`.

    tftp-client -h 127.0.0.1 -p 2000 -t stahovany-obrazek.png -f stazeny-obrazek.png
