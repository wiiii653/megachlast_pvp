# Donacje — megaklaster (adresy krypto)

> Dokument gotowy do implementacji. Adresy są **publiczne** (bezpieczne do publikacji).
> ⚠️ To jest ZESTAW ADRESÓW. **NIGDY nie umieszczaj w nim / nie loguj frazy seed** — seed pozostaje poza systemem, zapisany przez właściciela na papierze.

---

## 1. Główna lista donacji (do wyświetlenia graczom)

- **Bitcoin (BTC)**
  `13A3DTPUUMq3hgBSJ5R5N4Sz9CVL2v4AY1`

- **Ethereum (ETH)**
  `0xac43ea88a19ee3d214b25cb2aeac5a156bbd7652`
  *(Ten adres przyjmuje również ERC-20: USDT-ERC20, USDC oraz tokeny na łańcuchach EVM — ETH, Polygon, BSC — ten sam klucz/address.)*

- **Tether (USDT) — sieć TRON (TRC-20)**
  `TEoNMfDBS86nAuFbBgpAxBVqKizzcpHF5t`
  *(Tylko TRON! Nie mieszać z ERC-20.)*

- **Litecoin (LTC)**
  `LMMgp5Esoiy8if7G4v51UgBu5BJY1axAEW`

- **XRP (Ripple)**
  `rQnFKC1eRLvviR8sqXG1GzvJZAg6nmr8co`

- **Solana (SOL)**
  `HGkhk5mKH2LMAVRCfc49Nk535Py8iGKSMYvTg2Rv1dJb`

- **Dogecoin (DOGE)**
  `D7Gq67sh2UdMzrbhdN5GkRHjk6fZG1WAXc`

## 2. Opcjonalne (tylko jeśli chcesz je pokazać)

- **TRON (TRX)**
  `TKbrWfhD1S8WDyaQKo5CYy7rMgEN9RKgsn`
- **Monero (XMR)** — *pełny adres do uzupełnienia przy implementacji (zrzut miał skrót: `47tULZwd8JB3u83V…`)*

---

## 3. Notatki sieciowe dla implementującego

- **USDT TRC-20 ≠ USDT ERC-20.** Jeśli gracz wyśle USDT po sieci Ethereum, trafi na adres ETH `0xac…` (też Twój — ale to inna sieć). Przy wyborze sieci pokazuj wyraźnie: **TRON → `TEoNM…`**, **Ethereum → `0xac…`**.
- **EVM-owe łańcuchy** (ETH/Polygon/BSC/MATIC/USDC.e) — wszystkie korzystają z adresu `0xac43ea88a19ee3d214b25cb2aeac5a156bbd7652` (ten sam klucz w łańcuchach EVM).
- **Fee (przy wysyłaniu z portfela, nie przy odbiorze):** na Tronie potrzeba małej rezerwy **TRX** (lub gas-free tryb Guardy); na Polygon — **POL**.
- **Odbiór zawsze darmowy.** Donacje nie wymagają po stronie donatora niczego ponad właściwą sieć.

## 4. Wytyczne implementacji (dla agenta)

- Przyjmuj przede wszystkim: **USDT TRC-20**, **BTC**, **LTC**, **XRP**, **SOL**, **DOGE**.
- Pokaż przy każdej monetzie: **ticker + nazwa sieci**, nie sam adres.
- **Nigdy nie loguj / nie wyświetlaj frazy seed ani kluczy prywatnych.** Adresy w tym pliku są publiczne i przeznaczone do publikacji.
- Test: przy wdrożeniu wyślij na każdy adres **symboliczną kwotę** i potwierdź, że dochodzi (test przed publicznym udostępnieniem).

---

*Utworzono: 2026-10-02. Adresy z Guarda (jeden seed HD). Backup (12 słów) w posiadaniu właściciela.*