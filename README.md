# Inquest Chipset — Fedora / Linux

Portare locală a utilitarelor Inquest Chipset pentru **Guild Wars 2 + Nexus prin Steam/Proton pe Fedora Linux**.

Repository-ul conține sursele, scripturile Linux de compilare și instalare, sunetul folosit de addon și dependențele necesare. Fișierele originale Blish HUD (`PocketUtilities.dll`, PDB, configurații .NET), proiectele Visual Studio, executabilele și datele personale de test sunt excluse.

**Nexus rulează în procesul Windows al GW2 prin Proton.** Din acest motiv, compilarea pe Fedora produce un DLL Windows x64. Acest DLL este necesar pentru funcționare, dar este generat local și ignorat de Git; nu este inclus în repository. Nu există instalator sau procedură de instalare pentru Windows în acest proiect.

## Cerințe

- Fedora Linux cu Steam, Guild Wars 2 și Proton configurate.
- Nexus deja instalat în directorul GW2.
- Python 3 și Bash.
- Zig 0.14.1 în `PATH`, sau locația sa în variabila `ZIG`.

## Compilare și instalare

Închide GW2 înainte de instalare.

```bash
git clone https://github.com/insightfulrival/inquest-chipset-fedora.git
cd inquest-chipset-fedora
./build.sh
python3 install.py
```

Pentru Zig într-un alt director:

```bash
ZIG=/cale/catre/zig ./build.sh
```

Instalatorul folosește implicit `~/.local/share/Steam/steamapps/common/Guild Wars 2`. Pentru altă bibliotecă Steam, inclusiv Steam Flatpak:

```bash
python3 install.py --game "/cale/catre/steamapps/common/Guild Wars 2"
```

Instalatorul copiază `dist/InquestChipset.dll` și sunetul în `addons`, păstrează o copie a configurației Nexus și activează încărcarea automată. La următoarea pornire a GW2, addonul va fi încărcat. Clicurile se activează manual.

## Utilizare

| Scurtătură implicită | Acțiune |
| --- | --- |
| `,` ținut apăsat | Dublu clic repetat la cursor, la 50 ms |
| `[` | Dialog pentru intervalul de 0,25–300 secunde; repetare la poziția aleasă; apăsarea din nou oprește |
| `Ctrl+Space` | Dodge-jump |
| `Ctrl+Shift+I` | Setările addonului |

Schimbă scurtăturile în **Nexus → Keybinds**: `INQUEST_HOLD`, `INQUEST_TOGGLE`, `INQUEST_DODGEJUMP`, `INQUEST_SETTINGS`. Pe alte aranjamente de tastatură simbolul tastei `[` poate fi diferit. Setările apar și în opțiunile addonului din Nexus.

Dodge-jump folosește legăturile jocului cunoscute de Nexus, cu fallback implicit la `V` / `Space`. În setări poți dezactiva „Use Nexus game bindings” și schimba codurile tastelor fallback.

Clicurile se opresc în combat și la ieșirea din gameplay. Chatul, harta, mișcarea camerei, tastele modificatoare și pierderea focusului suspendă inputul. Modul periodic păstrează timpul rămas la pauză. Dodge-jump este blocat pe mount, în chat și pe hartă. „Stop all clicking” oprește clicurile.

Setările sunt salvate în `addons/InquestChipset/settings.ini`. Pentru dezactivare, oprește addonul din Nexus. Pentru dezinstalare, închide GW2 și șterge numai `addons/InquestChipset.dll`; setările pot fi păstrate.

## Validare și limite

Compilare verificată pe Fedora cu Zig 0.14.1. DLL-ul a trecut un test de încărcare cu Wine 11 din Proton Experimental: exportul `GetAddonDef`, API Nexus v6, interfața, cele patru scurtături, blocarea inputului în afara gameplay-ului și descărcarea/reîncărcarea. Acesta este un test izolat; clicurile și dodge-jump nu au fost încă verificate pe un personaj în GW2.

Interfața este refăcută cu ImGui. Este folosit un sunet original cu volum configurabil, în locul alegerii aleatoare dintre trei sunete.

## Proveniență

Portare neoficială. Autorul modulului original: **Nekres**.

- [Modulul Blish original](https://github.com/agaertner/bhm-pocket-utilities), sursă consultată la commit `cf8f2306d7996ac242cd1aef7a5fd99444607137`.
- [Nexus API](https://github.com/RaidcoreGG/Nexus-API), commit `9b2c53df86c00db6495642bfcff2d0611bd957ef`; licență în `vendor/Nexus.LICENSE`.
- [Mumble API](https://github.com/RaidcoreGG/RCGG-lib-mumble-api), commit `8c93788c3042dd6b401c0a93d17e34e1d4414e9a`; licență în `vendor/Mumble.LICENSE`.
- [ImGui Nexus 1.80](https://github.com/RaidcoreGG/imgui), commit `58075c4414b985b352d10718b02a8c43f25efd7c`; licență în `vendor/imgui/LICENSE.txt`.

DLL-ul Blish a fost inspectat static cu REA 6.7.0. SHA-256 original: `f93ce0399c13f647c43fc1a51e5b92f5790b96f9e6ed932741fbab94594a805b`. Este .NET Framework 4.7.2 / AnyCPU, dependent de Blish HUD; integrarea a fost rescrisă pentru Nexus API v6. Fișierul original și rapoartele locale nu sunt publicate aici.
