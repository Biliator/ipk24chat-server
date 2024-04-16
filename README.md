# IPK server pro chatovací aplikaci

## 1. Úvod

IPK server pro chatovací aplikace umožňuje uživatel se připojit na server a pomocí něj posílat zprávy jiným uživatelům a přijímat zprávy. Uživatel začíná přihlášením a po úspěšné autorizaci, je připojen do výchozího kanálu (default), kde odtud může komunikovat s dalšími účastníky. Program byl vyvíjen ve WSL a ne všechny knihovny musí být na jiných distribucí Linuxu dostupné. Např. na Merlinovi mi nešel Makefile, ale na Virtuálce, která byla dodáná k projektu, mi to šlo.

### make a spuštění

krok 1.
```make```

krok 2.
```./ipk24chat-server -l IPv4 -p PORT -d [TIME IN MS] -r [NUMBER] -h```

-d a -r nepovinný a pouze u udp -h je nápověda

## 2. Teorie

V této části budou vysvětleny základní principy a koncepty, které jsou nezbytné pro porozumění fungování aplikace.

### UDP
UDP je zkratka pro User Datagram Protocol a je to jednoduchý, bezspojkový, nespolehlivý protokol vrstvy transportu v síťové architektuře TCP/IP. UDP přenáší datagramy nezávisle na sobě, což znamená, že každý datagram může cestovat jinou cestou a dorazit v jiném pořadí než byl odeslán. Nezajišťuje žádnou formu potvrzení doručení nebo opakování přenosu dat, což znamená, že není zaručeno, že data dorazí v pořádku, ani že dorazí vůbec.

### TCP
TCP je zkratka pro Transmission Control Protocol a je to spojově orientovaný, spolehlivý protokol vrstvy transportu v síťové architektuře TCP/IP. TCP zajišťuje spolehlivý přenos dat tím, že kontroluje přenos dat mezi odesílatelem a příjemcem, potvrzuje doručení dat a řeší problémy, jako jsou ztráty paketů, opoždění a duplicity. Pro dosažení spolehlivého doručení TCP využívá mechanismy jako je potvrzování doručení, opakování přenosu, řízení toku a řízení zahlcení.

## 3. Architektura

## 4. Testování
Tato část dokumentace obsahuje informace o testování aplikace. Zahrnuje popis testovací metodiky, prostředí testování, provedené testy a výsledky.

### TCP testování

### UDP testování

## Bibliografie
User Datagram Protocol. (srpen, 1980). J. Postel.
https://datatracker.ietf.org/doc/html/rfc768

Datatracker, Transmission Control Protocol (TCP). (srpen, 2022). Wesley Eddy.
https://datatracker.ietf.org/doc/html/rfc9293

Beej's Guide to Network Programming. (8. dubna, 2023). Brian “Beej Jorgensen” Hall.
https://beej.us/guide/bgnet/html/

NESFIT/IPK-Projekty. (2024).
https://git.fit.vutbr.cz/NESFIT/IPK-Projects-2024/src/branch/master/Stubs/cpp

The GNU Netcat project. (1.listopadu 2006). Giovanni Giacobbi.
https://netcat.sourceforge.net/